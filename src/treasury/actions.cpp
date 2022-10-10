#include <dao.hpp>

#include <algorithm>
#include <numeric>

#include <member.hpp>
#include <document_graph/edge.hpp>

#include <treasury/treasury.hpp>
#include <treasury/redemption.hpp>
#include <treasury/balance.hpp>
#include <treasury/payment.hpp>
#include <treasury/attestation.hpp>
#include <treasury/common.hpp>

namespace hypha {

using treasury::Treasury;
using treasury::TreasuryData;
using treasury::Redemption;
using treasury::RedemptionData;
using treasury::Balance;
using treasury::TrsyPayment;
using treasury::TrsyPaymentData;
using treasury::Attestation;
using treasury::AttestationData;

namespace trsycommon = treasury::common;


void verifyAttestations(
  const std::vector<Attestation>& attestations,
  Settings* treasurySettings,
  TrsyPayment& payment,
  Redemption& redemption
) {

  //Check if we can confirm the payment
  if (attestations.size() >= treasurySettings->getOrFail<int64_t>(trsycommon::fields::THRESHOLD))
  {
    payment.setIsConfirmed(true);
    payment.setConfirmedDate(eosio::current_time_point());
    payment.update();
    redemption.setAmountPaid(redemption.getAmountPaid() + payment.getAmountPaid());
    redemption.update();
    //TODO: Notify when payment is confirmed (?)
  }
}

ACTION dao::addtreasurer(uint64_t treasury_id, name treasurer)
{
    TRACE_FUNCTION();
    EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

    Treasury treasury(*this, treasury_id);

    auto daoID = treasury.getDaoID();

    checkAdminsAuth(daoID);

    auto mem = getOrCreateMember(treasurer);

    mem.checkMembershipOrEnroll(daoID);

    treasury.addTreasurer(mem.getID());
}

ACTION dao::remtreasurer(uint64_t treasury_id, name treasurer)
{
    TRACE_FUNCTION();
    EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

    Treasury treasury(*this, treasury_id);

    auto daoID = treasury.getDaoID();

    checkAdminsAuth(daoID);

    treasury.removeTreasurer(getMemberID(treasurer));
}

ACTION dao::createtrsy(uint64_t dao_id)
{
    TRACE_FUNCTION();
    EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

    checkAdminsAuth(dao_id);

    Treasury trsy(*this, TreasuryData {
        .dao = static_cast<int64_t>(dao_id)
    });
}

ACTION dao::redeem(uint64_t dao_id, name requestor, const asset& amount)
{
  TRACE_FUNCTION();

  require_auth(requestor);

  EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

  auto treasury = Treasury::getFromDaoID(*this, dao_id);

  auto treasurySettings = getSettingsDocument(treasury.getId());

  //Check if redeptions are enabled
  EOS_CHECK(
    treasurySettings->getOrFail<int64_t>(trsycommon::fields::REDEMPTIONS_ENABLED) == 1,
    "Redemptions are not available at this moment; please try later."
  )

  Balance bal = Balance::getOrCreate(*this, dao_id, getMemberID(requestor));

  //If balance is not greater or equal to amount, we will get overdrawn balance error
  bal.substract(amount);

  bal.update();
  
  //Create redemption entry
  Redemption redemption(*this, treasury.getId(), RedemptionData {
    .requestor = requestor,
    .amount_requested = amount,
    .amount_paid = asset(0, amount.symbol)
  });
}

ACTION dao::newpayment(name treasurer, uint64_t redemption_id, const asset& amount, string notes)
{
  TRACE_FUNCTION();
  require_auth(treasurer);

  EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

  //Verify the redemption id is valid by creating a Redemption object with it
  Redemption redemption(*this, redemption_id);

  auto treasury = redemption.getTreasury();
  treasury.checkTreasurerAuth();

  const auto& amountRequested = redemption.getAmountRequested();

  {
    auto amountDue = amountRequested - redemption.getAmountPaid();
    //Check the payment amount is less or equal to the requested amount
    EOS_CHECK(
      amount <= amountDue,
      to_str(
        "Redemption amount must be less than amount due. Original requested amount: ", amountRequested,
        "; Paid amount: ", redemption.getAmountPaid(),
        ". The remaining amount due is: ", amountDue,
        " and you attempted to create a new payment for: ", amount
      )
    );
  }

  //Check if the current payments sum less than the required amount
  {
    auto payments = redemption.getPayments();
    auto totalPayAmount = std::accumulate(payments.begin(), 
                                          payments.end(), 
                                          asset(0, amount.symbol), 
                                          [](asset& prev, TrsyPayment& current){
                                            return prev + current.getAmountPaid();
                                          });

    auto amountDue = amountRequested - totalPayAmount;
    EOS_CHECK(
      amount <= amountDue,
      to_str("Accumulated amount of existing confirmed & unconfirmed payments is ", totalPayAmount,
                   ", redemption amount is ", amountRequested, 
                   ", so new payments amount must be less or equal to ", amountDue, " but you submitted ", amount)
    )
  }

  TrsyPayment payment(*this, treasury.getId(), redemption_id, TrsyPaymentData {
    .creator = treasurer,
    .amount_paid = amount,
    .confirmed_date = eosio::time_point(),
    .is_confirmed = false,
    .notes = std::move(notes)
  });

  Attestation attestation(*this, payment.getId(), AttestationData {
    .treasurer = treasurer
  });
 
  verifyAttestations(
    { std::move(attestation) }, //Empty array since this is the first attestation
    getSettingsDocument(treasury.getId()),
    payment,
    redemption
  );

  //TODO: Should we notify the requestor?
}

ACTION dao::attestpay(name treasurer, uint64_t payment_id, const asset& amount, const std::optional<std::string>& notes)
{
  TRACE_FUNCTION();
  require_auth(treasurer);

  EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

  TrsyPayment payment(*this, payment_id);

  auto treasury = payment.getTreasury();

  treasury.checkTreasurerAuth();

  EOS_CHECK(
    !payment.getIsConfirmed(),
    "Cannot attest to a payment that has already been confirmed"
  );

  //Verify that the redeption document is valid by fetching it
  auto redemption = payment.getRedemption();

  EOS_CHECK(
    payment.getAmountPaid() == amount,
    "amount does not match amount on payment you are attesting to."
  )

  auto attestations = payment.getAttestations();

  EOS_CHECK(
    std::none_of(attestations.begin(), attestations.end(), [&treasurer](Attestation& attest){
      return attest.getTreasurer() == treasurer;
    }),
    to_str("Treasurer ", treasurer, " has already attested to this payment")
  );

  if (notes.has_value()) {
    payment.setNotes(notes.value());
    payment.update();
  }

  Attestation attestation(*this, payment.getId(), AttestationData {
    .treasurer = treasurer
  });

  attestations.emplace_back(std::move(attestation));

  verifyAttestations(
    attestations,
    getSettingsDocument(treasury.getId()),
    payment,
    redemption
  );
}

ACTION dao::remattestpay(name treasurer, uint64_t payment_id)
{
  TRACE_FUNCTION();
  require_auth(treasurer);

  EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

  TrsyPayment payment(*this, payment_id);

  auto treasury = payment.getTreasury();

  treasury.checkTreasurerAuth();

  EOS_CHECK(
    !payment.getIsConfirmed(),
    "Cannot remove attestation from a payment that has already been confirmed"
  );

  auto attestations = payment.getAttestations();

  auto attestIt = std::find_if(attestations.begin(), attestations.end(), [&treasurer](Attestation& attest){
    return attest.getTreasurer() == treasurer;
  });

  EOS_CHECK(
    attestIt != attestations.end(),
    to_str("Treasurer ", treasurer, " hasn't attested this payment")
  );

  attestIt->remove();

  //Remove payment if this is the only attestation
  if (attestations.size() == 1) {
    getGraph().eraseDocument(payment.getId());
  }
}

ACTION dao::setpaynotes(uint64_t payment_id, string notes)
{
  TRACE_FUNCTION();

  EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

  TrsyPayment payment(*this, payment_id);

  auto treasury = payment.getTreasury();

  treasury.checkTreasurerAuth();

  payment.setNotes(notes);
  payment.update();
}

ACTION dao::setrsysttngs(uint64_t treasury_id, const std::map<std::string, Content::FlexValue>& kvs, std::optional<std::string> group)
{
  TRACE_FUNCTION();

  EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

  Treasury treasury(*this, treasury_id);

  treasury.checkTreasurerAuth();

  auto settings = getSettingsDocument(treasury_id);

  settings->setSettings(group.value_or(SETTINGS), kvs);
}

void dao::onCashTokenTransfer(const name& from, const name& to, const asset& quantity, const string& memo)
{
  EOS_CHECK(quantity.amount > 0, "quantity must be > 0");
  EOS_CHECK(quantity.is_valid(), "quantity invalid");

  //Since the symbols must be unique for each Cash token we can use the 
  //raw value as the edge name
  name lookupEdgeName = name(quantity.symbol.raw());

  //This would be a very weird scenario where the symbol raw value
  //equals the edge name 'dao' which would cause unexpected behaviour
  if (lookupEdgeName == common::DAO) {
    auto settings = getSettingsDocument();
    settings->setSetting(
      "errors", 
      Content{ "cash_critital_error", to_str("Symbol raw value colapses with 'dao' edge name:", quantity) }
    );
    return;
  }

  uint64_t daoID = 0;

  auto rootID = getRootID();
  //Fast lookup
  if (auto [exists, edge] = Edge::getIfExists(get_self(), rootID, lookupEdgeName); exists) {
    daoID = edge.getToNode();
  }
  //We have to find which DAO this token belongs to (if any)
  else {
    auto daoEdges = getGraph().getEdgesFrom(rootID, common::DAO);

    for (auto& edge : daoEdges) {
      auto daoSettings = getSettingsDocument(edge.getToNode());

      if (daoSettings->getOrFail<asset>(common::PEG_TOKEN).symbol == quantity.symbol) {
        //If we find it, let's create a lookup edge for the next time we get this symbol
        daoID = edge.getToNode();
        Edge(get_self(), get_self(), rootID, daoID, lookupEdgeName);
        break;
      }
    }
  }

  EOS_CHECK(
    daoID != 0,
    "No DAO uses the transfered token"
  );

  EOS_CHECK(
    Member::isMember(*this, daoID, from),
    "Sender is not a member of the DAO"
  )

  auto member = Member(*this, getMemberID(from));

  Balance memberBal = Balance::getOrCreate(*this, daoID, member.getID());

  memberBal.add(quantity);

  memberBal.update();
}

}