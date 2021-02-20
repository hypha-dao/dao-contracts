#include <document_graph/content_wrapper.hpp>
#include <document_graph/util.hpp>
#include <proposals/proposal.hpp>
#include <proposals/proposal_factory.hpp>
#include <payers/payer_factory.hpp>
#include <payers/payer.hpp>

#include <dao.hpp>
#include <common.hpp>
#include <util.hpp>
#include <member.hpp>
#include <period.hpp>
#include <assignment.hpp>

namespace hypha
{
   void dao::propose(const name &proposer,
                     const name &proposal_type,
                     ContentGroups &content_groups)
   {
      eosio::check(!isPaused(), "Contract is paused for maintenance. Please try again later.");

      std::unique_ptr<Proposal> proposal = std::unique_ptr<Proposal>(ProposalFactory::Factory(*this, proposal_type));
      proposal->propose(proposer, content_groups);
   }

   void dao::closedocprop(const checksum256 &proposal_hash)
   {
      eosio::check(!isPaused(), "Contract is paused for maintenance. Please try again later.");

      Document docprop(get_self(), proposal_hash);
      name proposal_type = docprop.getContentWrapper().getOrFail(SYSTEM, TYPE)->getAs<eosio::name>();

      Proposal *proposal = ProposalFactory::Factory(*this, proposal_type);
      proposal->close(docprop);
   }

   void dao::claimnextper(const eosio::checksum256 &assignment_hash)
   {
      eosio::check(!isPaused(), "Contract is paused for maintenance. Please try again later.");

      Assignment assignment(this, assignment_hash);
      eosio::name assignee = assignment.getAssignee().getAccount();

      // assignee must still be a DHO member
      eosio::check(Member::isMember(get_self(), assignee), "assignee must be a current member to claim pay: " + assignee.to_string());

      std::optional<Period> periodToClaim = assignment.getNextClaimablePeriod();
      eosio::check(periodToClaim != std::nullopt, "All available periods for this assignment have been claimed: " + readableHash(assignment_hash));

      // require_auth(assignee);
      eosio::check(has_auth(assignee) || has_auth(get_self()), "only assignee or " + get_self().to_string() + " can claim pay");

      // Valid claim identified - start process
      // process this claim
      Edge::write(get_self(), get_self(), assignment.getHash(), periodToClaim.value().getHash(), common::CLAIMED);
      int64_t assignmentApprovedDateSec = assignment.getApprovedTime().sec_since_epoch();
      int64_t periodStartSec = periodToClaim.value().getStartTime().sec_since_epoch();
      int64_t periodEndSec = periodToClaim.value().next().getEndTime().sec_since_epoch();

      // Pro-rate the payment if the assignment was created during the period being claimed
      float first_phase_ratio_calc = 1; // pro-rate based on elapsed % of the first phase
      if (assignmentApprovedDateSec > periodStartSec)
      {
         int64_t elapsed_sec = periodEndSec - assignmentApprovedDateSec;
         int64_t period_sec = periodEndSec - periodStartSec;
         first_phase_ratio_calc = (float)elapsed_sec / (float)period_sec;
      }
      eosio::check(first_phase_ratio_calc <= 1, "fatal error: first_phase_ratio_calc is greater than 1: " + std::to_string(first_phase_ratio_calc));

      // These values are calculated when the assignment is proposed, so simply pro-rate them if/as needed
      // If there is an explicit INSTANT SEEDS amount, support sending it
      asset husd = adjustAsset(assignment.getSalaryAmount(&common::S_HUSD), first_phase_ratio_calc);
      asset hvoice = adjustAsset(assignment.getSalaryAmount(&common::S_HVOICE), first_phase_ratio_calc);
      asset hypha = adjustAsset(assignment.getSalaryAmount(&common::S_HYPHA), first_phase_ratio_calc);

      string memo = "Payment for assignment " + readableHash(assignment.getHash()) + "; Period: " + readableHash(periodToClaim.value().getHash());

      // creating a single struct improves performance for table queries here
      AssetBatch ab{};
      ab.hypha = hypha;
      ab.voice = hvoice;
      ab.husd = husd;

      ab = applyBadgeCoefficients(periodToClaim.value(), assignee, ab);

      makePayment(periodToClaim.value().getHash(), assignee, ab.hypha, memo, eosio::name{0});
      makePayment(periodToClaim.value().getHash(), assignee, ab.voice, memo, eosio::name{0});
      makePayment(periodToClaim.value().getHash(), assignee, ab.husd, memo, eosio::name{0});
   }

   asset dao::getProRatedAsset(ContentWrapper *assignment, const symbol &symbol, const string &key, const float &proration)
   {
      asset assetToPay = asset{0, symbol};
      if (auto [idx, assetContent] = assignment->get(DETAILS, key); assetContent)
      {
         eosio::check(std::holds_alternative<eosio::asset>(assetContent->value), "fatal error: expected token type must be an asset value type: " + assetContent->label);
         assetToPay = std::get<eosio::asset>(assetContent->value);
      }
      return adjustAsset(assetToPay, proration);
   }

   std::vector<Document> dao::getCurrentBadges(Period &period, const name &member)
   {
      std::vector<Document> current_badges;
      std::vector<Edge> badge_assignment_edges = m_documentGraph.getEdgesFrom(Member::calcHash(member), common::ASSIGN_BADGE);
      for (Edge e : badge_assignment_edges)
      {
         Document badgeAssignmentDoc(get_self(), e.getToNode());
         auto badgeAssignment = badgeAssignmentDoc.getContentWrapper();
         Edge badge_edge = Edge::get(get_self(), badgeAssignmentDoc.getHash(), common::BADGE_NAME);
         Document badge(get_self(), badge_edge.getToNode());
         current_badges.push_back(badge);

         // TODO: exclude badges that are no longer active
         // Period startPeriod(this, badgeAssignment.getOrFail(DETAILS, START_PERIOD)->getAs<eosio::checksum256>());
         // int64_t periodCount = badgeAssignment.getOrFail(DETAILS, PERIOD_COUNT)->getAs<int64_t>();
         // int64_t counter = 0;
         // std::optional<Period> seeker = std::optional<Period>{startPeriod};
         // while (seeker.has_value() && counter <= periodCount)
         // {
         //    eosio::print (" counter : " + std::to_string(counter) + "\n");
         //    seeker = seeker.value().next();
         //    counter++;
         // }
         // std::optional<eosio::time_point> periodEndTime = seeker.value().getEndTime();
         // eosio::check(periodEndTime != std::nullopt, "End of calendar has been reached. Contact administrator to add more time periods.");
         // int64_t badgeAssignmentExpiration = periodEndTime.value().sec_since_epoch();
         // if (badgeAssignmentExpiration > eosio::current_time_point().sec_since_epoch())
         // {

         // }
      }
      return current_badges;
   }

   eosio::asset dao::applyCoefficient(ContentWrapper &badge, const eosio::asset &base, const std::string &key)
   {
      if (auto [idx, coefficient] = badge.get(DETAILS, key); coefficient)
      {
         if (std::holds_alternative<std::monostate>(coefficient->value))
         {
            return asset{0, base.symbol};
         }

         eosio::check(std::holds_alternative<int64_t>(coefficient->value), "fatal error: coefficient must be an int64_t type: key: " + key);

         float coeff_float = (float)((float)std::get<int64_t>(coefficient->value) / (float)10000);
         float adjustment = (float)coeff_float - (float)1;
         return adjustAsset(base, adjustment);
      }
      return asset{0, base.symbol};
   }

   dao::AssetBatch dao::applyBadgeCoefficients(Period &period, const eosio::name &member, dao::AssetBatch &ab)
   {
      // get list of badges
      auto badges = getCurrentBadges(period, member);
      AssetBatch applied_assets = ab;

      // for each badge, apply appropriate coefficients
      for (auto &badge : badges)
      {
         ContentWrapper badgeCW = badge.getContentWrapper();
         applied_assets.hypha = applied_assets.hypha + applyCoefficient(badgeCW, ab.hypha, HYPHA_COEFFICIENT);
         applied_assets.husd = applied_assets.husd + applyCoefficient(badgeCW, ab.husd, HUSD_COEFFICIENT);
         applied_assets.voice = applied_assets.voice + applyCoefficient(badgeCW, ab.voice, HVOICE_COEFFICIENT);
      }

      return applied_assets;
   }

   void dao::makePayment(const eosio::checksum256 &fromNode,
                         const eosio::name &recipient,
                         const eosio::asset &quantity,
                         const string &memo,
                         const eosio::name &paymentType)
   {
      // nothing to do if quantity is zero of symbol is USD, a known placeholder
      if (quantity.amount == 0 || quantity.symbol == common::S_USD)
      {
         return;
      }

      std::unique_ptr<Payer> payer = std::unique_ptr<Payer>(PayerFactory::Factory(*this, quantity.symbol, paymentType));
      Document paymentReceipt = payer->pay(recipient, quantity, memo);
      Edge::write(get_self(), get_self(), fromNode, paymentReceipt.getHash(), common::PAYMENT);
      Edge::write(get_self(), get_self(), Member::calcHash(recipient), paymentReceipt.getHash(), common::PAID);
   }

   void dao::apply(const eosio::name &applicant, const std::string &content)
   {
      require_auth(applicant);
      Member member(get_self(), applicant, applicant);
      member.apply(getRoot(get_self()), content);
   }

   void dao::enroll(const eosio::name &enroller, const eosio::name &applicant, const std::string &content)
   {
      require_auth(enroller);
      Member member = Member::get(get_self(), applicant);
      member.enroll(enroller, content);
   }

   bool dao::isPaused() { return false; }

   Document dao::getSettingsDocument()
   {
      auto root = getRoot(get_self());
      auto edges = m_documentGraph.getEdgesFromOrFail(root, common::SETTINGS_EDGE);
      eosio::check(edges.size() == 1, "There should only exists only 1 settings edge from root node");
      return Document(get_self(), edges[0].to_node);
   }

   void dao::setsetting(const string &key, const Content::FlexValue &value)
   {
      require_auth(get_self());
      setSetting(key, value);
   }

   void dao::setSetting(const string &key, const Content::FlexValue &value)
   {
      auto document = getSettingsDocument();
      auto oldHash = document.getHash();
      auto settingContent = Content(key, value);
      auto updateDateContent = Content(UPDATED_DATE, eosio::current_time_point());

      ContentWrapper cw = document.getContentWrapper();
      ContentGroup *settings = cw.getGroupOrFail("settings");

      ContentWrapper::insertOrReplace(*settings, settingContent);
      ContentWrapper::insertOrReplace(*settings, updateDateContent);

      m_documentGraph.updateDocument(get_self(), oldHash, document.getContentGroups());
   }

   void dao::remsetting(const string &key)
   {
      require_auth(get_self());
      removeSetting(key);
   }

   void dao::removeSetting(const string &key)
   {
      auto document = getSettingsDocument();
      auto oldHash = document.getHash();
      auto contentGroups = document.getContentGroups();
      auto &settingsGroup = contentGroups[0];

      auto isKey = [&key](auto &c) {
         return c.label == key;
      };

      //First let's check if key exists
      auto contentItr = std::find_if(settingsGroup.begin(), settingsGroup.end(), isKey);
      if (contentItr != settingsGroup.end())
      {
         settingsGroup.erase(contentItr);
         auto updateDateContent = Content(UPDATED_DATE, eosio::current_time_point());
         ContentWrapper::insertOrReplace(settingsGroup, updateDateContent);
         m_documentGraph.updateDocument(get_self(), oldHash, std::move(contentGroups));
      }
      //Should we assert if setting doesn't exits ?
      eosio::check(false, "The specified setting does not exist: " + key);
   }

   void dao::addperiod(const eosio::checksum256 &predecessor, const eosio::time_point &start_time, const string &label)
   {
      require_auth(get_self());

      Period newPeriod(this, start_time, label);

      // check that predecessor exists
      Document previous(get_self(), predecessor);
      ContentWrapper contentWrapper = previous.getContentWrapper();

      // if it's a period, make sure the start time comes before
      auto [idx, predecessorType] = contentWrapper.get(SYSTEM, TYPE);
      if (predecessorType && predecessorType->getAs<eosio::name>() == common::PERIOD)
      {
         eosio::check(contentWrapper.getOrFail(DETAILS, START_TIME)->getAs<eosio::time_point>().sec_since_epoch() <
                          start_time.sec_since_epoch(),
                      "start_time of period predecessor must be before the new period's start_time");

         // predecessor is a period, so use "next" edge
         Edge::write(get_self(), get_self(), predecessor, newPeriod.getHash(), common::NEXT);
      }
      else
      {
         // predecessor is not a period, so use "start" edge
         Edge::write(get_self(), get_self(), predecessor, newPeriod.getHash(), common::START);
      }
   }

   // void dao::nbadge(const name &owner, const ContentGroups &contentGroups)
   // {
   //    eosio::require_auth(get_self());

   //    Migration migration(this);
   //    migration.createBadge(owner, contentGroups);
   // }

   // void dao::nbadass(const name &owner, const ContentGroups &contentGroups)
   // {
   //    eosio::require_auth(get_self());

   //    Migration migration(this);
   //    migration.createBadgeAssignment(owner, contentGroups);
   // }

   // void dao::nbadprop(const name &owner, const ContentGroups &contentGroups)
   // {
   //    eosio::require_auth(get_self());

   //    Migration migration(this);
   //    migration.createBadgeAssignmentProposal(owner, contentGroups);
   // }

   void dao::updatedoc(const eosio::checksum256 hash, const name &updater, const string &group, const string &key, const Content::FlexValue &value)
   {
      eosio::require_auth(get_self());

      Document document(get_self(), hash);
      auto oldHash = document.getHash();

      // the ContentWrapper is used to access the document's data
      ContentWrapper cw = document.getContentWrapper();

      // update the indicated group to the new value
      auto [idx, contentGroup] = cw.getGroupOrCreate(group);
      auto content = Content(key, value);
      ContentWrapper::insertOrReplace(*contentGroup, content);
      m_documentGraph.updateDocument(updater, oldHash, document.getContentGroups());
   }

   void dao::createroot(const std::string &notes)
   {
      require_auth(get_self());

      Document rootDoc(get_self(), get_self(), getRootContent(get_self()));

      // Create the settings document as well and add an edge to it
      ContentGroups settingCgs{
          ContentGroup{
              Content(CONTENT_GROUP_LABEL, SETTINGS),
              Content(ROOT_NODE, readableHash(rootDoc.getHash()))},
          ContentGroup{
              Content(CONTENT_GROUP_LABEL, SYSTEM),
              Content(TYPE, common::SETTINGS_EDGE),
              Content(NODE_LABEL, "Settings")}};

      Document settingsDoc(get_self(), get_self(), std::move(settingCgs));
      Edge::write(get_self(), get_self(), rootDoc.getHash(), settingsDoc.getHash(), common::SETTINGS_EDGE);
   }

   void dao::erasedoc(const checksum256 &hash)
   {
      require_auth(get_self());

      DocumentGraph dg(get_self());
      dg.eraseDocument(hash);
   }

   void dao::killedge(const uint64_t id)
   {
      require_auth(get_self());
      Edge::edge_table e_t(get_self(), get_self().value);
      auto itr = e_t.find(id);
      e_t.erase(itr);
   }

   void dao::setalert(const eosio::name &level, const std::string &content)
   {
      auto [exists, edge] = Edge::getIfExists(get_self(), getRoot(get_self()), common::ALERT);
      if (exists)
      {
         remalert("removing alert to replace with new alert");
      }

      Document alert(get_self(), get_self(),
                     ContentGroups{
                         ContentGroup{
                             Content(CONTENT_GROUP_LABEL, DETAILS),
                             Content(LEVEL, level),
                             Content(CONTENT, content)},
                         ContentGroup{
                             Content(CONTENT_GROUP_LABEL, SYSTEM),
                             Content(TYPE, common::ALERT),
                             Content(NODE_LABEL, "Alert : " + level.to_string())}});

      Edge::write(get_self(), get_self(), getRoot(get_self()), alert.getHash(), common::ALERT);
   }

   void dao::remalert(const string &notes)
   {
      Edge alertEdge = Edge::get(get_self(), getRoot(get_self()), common::ALERT);
      Document alert(get_self(), alertEdge.getToNode());
      getGraph().eraseDocument(alert.getHash());
   }

   DocumentGraph &dao::getGraph()
   {
      return m_documentGraph;
   }
} // namespace hypha