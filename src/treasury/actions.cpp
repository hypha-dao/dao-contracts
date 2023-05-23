#include <dao.hpp>

#include <algorithm>
#include <numeric>

#include <member.hpp>
#include <document_graph/edge.hpp>

#include <eosio/action.hpp>

#include <treasury/treasury.hpp>
#include <treasury/redemption.hpp>
#include <treasury/balance.hpp>
#include <treasury/payment.hpp>
#include <treasury/common.hpp>

namespace hypha {

#ifdef USE_TREASURY

using treasury::Treasury;
using treasury::TreasuryData;
using treasury::Redemption;
using treasury::RedemptionData;
using treasury::Balance;
using treasury::TrsyPayment;
using treasury::TrsyPaymentData;

namespace trsycommon = treasury::common;

static TrsyPayment createTrsyPayment(dao& dao, const name& treasurer, std::optional<uint64_t> treasuryID, const dao::RedemptionInfo& payment, bool isMsig) {

  EOS_CHECK(
    payment.amount.amount > 0,
    "Amount must be greater to 0"
  )

  //Verify the redemption id is valid by creating a Redemption object with it
  Redemption redemption(dao, payment.redemption_id);

  auto treasury = redemption.getTreasury();
  treasury.checkTreasurerAuth();

  EOS_CHECK(
    !treasuryID.has_value() || *treasuryID == treasury.getId(),
    "Redemption must belong to the provided DAO Treasury"
  )

  const auto& amountRequested = redemption.getAmountRequested();

  {
    auto amountDue = amountRequested - redemption.getAmountPaid();
    //Check the payment amount is less or equal to the amount due
    EOS_CHECK(
      payment.amount <= amountDue,
      to_str(
        "Redemption amount must be less than amount due. Original requested amount: ", amountRequested,
        "; Paid amount: ", redemption.getAmountPaid(),
        ". The remaining amount due is: ", amountDue,
        " and you attempted to create a new payment for: ", payment.amount
      )
    );
  }

  //Update amount paid
  redemption.setAmountPaid(redemption.getAmountPaid() + payment.amount);
  redemption.update();
  
  TrsyPayment trsyPayment(dao, treasury.getId(), payment.redemption_id, TrsyPaymentData {
    .creator = treasurer,
    .amount_paid = payment.amount,
    .notes = std::move(payment.notes),
    .state = isMsig ? trsycommon::payment_state::PENDING : trsycommon::payment_state::NONE //Specific flag for non msig payments
  });

  return trsyPayment;
}

ACTION dao::newmsigpay(name treasurer, uint64_t treasury_id, std::vector<RedemptionInfo>& payments, const std::vector<eosio::permission_level>& signers)
{
  require_auth(treasurer);

  EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");
 
  //TODO: Should we notify the requestor?
  auto dhoSettings = getSettingsDocument();
  auto nextProposalNameInt = dhoSettings->getSettingOrDefault("next_proposal_name_int", int64_t(1'000'000));
  
  name proposalName = name(uint64_t(nextProposalNameInt));

  dhoSettings->setSetting(Content{"next_proposal_name_int", nextProposalNameInt + 1});

  //Msig Document to store proposal name
  ContentGroups msigInfoCgs{
    ContentGroup{
      Content {CONTENT_GROUP_LABEL, DETAILS},
      Content {"proposal_name", proposalName},
      Content {"treasury_id", int64_t(treasury_id)},
      Content {"state", trsycommon::payment_state::PENDING}
    },
    ContentGroup {
      Content {CONTENT_GROUP_LABEL, SYSTEM},
      Content {TYPE, "msig.info"_n},
      Content {NODE_LABEL, "Multisig Info"}
    }
  };

  Document msigInfoDoc(get_self(), get_self(), msigInfoCgs);

  auto daoId = Treasury(*this, treasury_id).getDaoID();

  auto daoSettings = getSettingsDocument(daoId);

  auto treasuryAccount = daoSettings->getOrFail<name>("treasury_account");

  auto nativeToken = dhoSettings->getOrFail<asset>("native_token");

  eosio::transaction trx;

  trx.expiration = eosio::current_time_point() + eosio::days(5);

  //Hardcode for now the price
  constexpr auto NATIVE_TO_USD_RATIO = 1.25;

  for (auto& redemptionInfo : payments) {
    auto trsyPay = createTrsyPayment(*this, treasurer, treasury_id, redemptionInfo, true);

    Edge(get_self(), get_self(), trsyPay.getId(), msigInfoDoc.getID(), "msiginfo"_n);
    Edge(get_self(), get_self(), msigInfoDoc.getID(), trsyPay.getId(), trsycommon::links::PAYMENT);

    auto calculatedNative = int64_t(normalizeToken(redemptionInfo.amount) / NATIVE_TO_USD_RATIO);

    std::string notes;

    //Notes structure receiver;redemption_id;notes
    notes = to_str(
      redemptionInfo.receiver, ";", 
      redemptionInfo.redemption_id, ";", 
      redemptionInfo.notes.empty() ? "Redemption payment" : redemptionInfo.notes
    );

    trx.actions.push_back(eosio::action(
      eosio::permission_level{treasuryAccount, "active"_n},
      "eosio.token"_n,
      "transfer"_n,
      std::tuple(treasuryAccount, get_self(), denormalizeToken(calculatedNative, nativeToken), notes)
    ));
  }

  //Setup single multisig transaction for all redemptions (might want to create 1 per redemptio or make it optional)
  eosio::action(
    eosio::permission_level{get_self(), "active"_n},
    "eosio.msig"_n,
    "propose"_n,
    std::tuple(get_self(), proposalName, signers, trx)
  ).send();
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

    verifyDaoType(dao_id);

    checkAdminsAuth(dao_id);

    Treasury trsy(*this, TreasuryData {
        .dao = static_cast<int64_t>(dao_id)
    });
}

ACTION dao::redeem(uint64_t dao_id, name requestor, const asset& amount)
{
  TRACE_FUNCTION();

  EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

  verifyDaoType(dao_id);

  require_auth(requestor);

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

  createTrsyPayment(
    *this, 
    treasurer, 
    std::nullopt, 
    RedemptionInfo{ .amount = amount, .receiver = name(), .notes = notes, .redemption_id = redemption_id }, 
    false
  );

  //TODO: Should we notify the requestor?
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

#endif

void dao::onCashTokenTransfer(const name& from, const name& to, const asset& quantity, const string& memo)
{
#ifndef USE_TREASURY
  EOS_CHECK(
    false,
    "Treasury Module is not enabled"
  );
#else
  EOS_CHECK(quantity.amount > 0, "quantity must be > 0");
  EOS_CHECK(quantity.is_valid(), "quantity invalid");

  //Since the symbols must be unique for each Cash token we can use the 
  //raw value as the edge name
  name lookupEdgeName = name(quantity.symbol.raw());

  //This would be a very weird scenario where the symbol raw value
  //equals the edge name 'dao' which would cause unexpected behaviour
  if (lookupEdgeName == common::DAO) {
    // auto settings = getSettingsDocument();
    // settings->setSetting(
    //   "errors", 
    //   Content{ "cash_critital_error", to_str("Symbol raw value colapses with 'dao' edge name:", quantity) }
    // );
    EOS_CHECK(
      false, 
      to_str("Symbol raw value colapses with 'dao' edge name:", quantity)
    )

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
#endif
}

}