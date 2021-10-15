#include <algorithm>

#include <document_graph/content_wrapper.hpp>
#include <document_graph/util.hpp>
#include <proposals/proposal.hpp>
#include <proposals/proposal_factory.hpp>
#include <payers/payer_factory.hpp>
#include <payers/payer.hpp>
#include <logger/logger.hpp>

#include <dao.hpp>
#include <common.hpp>
#include <util.hpp>
#include <member.hpp>
#include <period.hpp>
#include <assignment.hpp>
#include <time_share.hpp>

namespace hypha
{
   void dao::fix (const checksum256 &hash)
   {
      eosio::check(!isPaused(), "Contract is paused for maintenance. Please try again later.");
      Assignment assignment(this, hash);
      Edge roleToAssignmentEdge = Edge::getTo(get_self(), hash, common::ASSIGNMENT);
      Edge::getOrNew(get_self(), get_self(), hash, roleToAssignmentEdge.getFromNode(), common::ROLE_NAME);
   }

   

   ACTION 
   dao::fixassigns(std::vector<eosio::checksum256>& hashes)
   {
     const size_t maxHashesPerAction = 2;

     EOS_CHECK(
       hashes.size() <= maxHashesPerAction,
       to_str("Too many hashes: ", hashes.size(), " max: ", maxHashesPerAction)
     )

     for (size_t i = 0; i < hashes.size(); ++i) {
       Assignment assign(this, hashes[i]);
       auto cw = assign.getContentWrapper();
       if (cw.getOrFail(DETAILS, common::STATE)->getAs<std::string>() == common::STATE_APPROVED && 
           !cw.exists(DETAILS, common::APPROVED_DEFERRED)) {
         auto details = cw.getGroupOrFail(DETAILS);
         ContentWrapper::insertOrReplace(*details, Content{
           common::APPROVED_DEFERRED,
           cw.getOrFail(DETAILS, DEFERRED)->getAs<int64_t>()
         });
         m_documentGraph.updateDocument(assign.getCreator(), assign.getHash(), cw.getContentGroups());
       }
     }
   }

   void dao::propose(const name &proposer,
                     const name &proposal_type,
                     ContentGroups &content_groups)
   {
      TRACE_FUNCTION()
      EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

      std::unique_ptr<Proposal> proposal = std::unique_ptr<Proposal>(ProposalFactory::Factory(*this, proposal_type));
      proposal->propose(proposer, content_groups);
   }

   void dao::vote(const name &voter, const checksum256 &proposal_hash, string &vote, string notes)
   {
      TRACE_FUNCTION()
      EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");
      Document docprop(get_self(), proposal_hash);
      name proposal_type = docprop.getContentWrapper().getOrFail(SYSTEM, TYPE)->getAs<eosio::name>();

      Proposal *proposal = ProposalFactory::Factory(*this, proposal_type);
      proposal->vote(voter, vote, docprop, notes);
   }

   void dao::closedocprop(const checksum256 &proposal_hash)
   {
      TRACE_FUNCTION()
      EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

      Document docprop(get_self(), proposal_hash);
      name proposal_type = docprop.getContentWrapper().getOrFail(SYSTEM, TYPE)->getAs<eosio::name>();

      Proposal *proposal = ProposalFactory::Factory(*this, proposal_type);
      proposal->close(docprop);
   }

   void dao::proposeextend(const checksum256 &assignment_hash, const int64_t additional_periods)
   {
      TRACE_FUNCTION()
      EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

      Assignment assignment(this, assignment_hash);

      // only the assignee can submit an extension proposal
      eosio::name assignee = assignment.getAssignee().getAccount();
      eosio::require_auth(assignee);
      EOS_CHECK(Member::isMember(get_self(), assignee), "assignee must be a current member to request an extension: " + assignee.to_string());

      eosio::print("\nproposer is: " + assignee.to_string() + "\n");
      // construct ContentGroups to call propose
      auto contentGroups = ContentGroups{
          ContentGroup{
              Content(CONTENT_GROUP_LABEL, DETAILS),
              Content(PERIOD_COUNT, assignment.getPeriodCount() + additional_periods),
              Content(TITLE, std::string("Assignment Extension Proposal")),
              Content(ORIGINAL_DOCUMENT, assignment.getHash())}};

      // propose the extension
      std::unique_ptr<Proposal> proposal = std::unique_ptr<Proposal>(ProposalFactory::Factory(*this, common::EXTENSION));
      proposal->propose(assignee, contentGroups);
   }

   void dao::withdraw(name owner, eosio::checksum256 hash) 
   {  
     TRACE_FUNCTION()
     EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");
     
     Assignment assignment(this, hash);

     eosio::name assignee = assignment.getAssignee().getAccount();

     EOS_CHECK(
       assignee == owner, 
       to_str("Only the member [", assignee.to_string() ,"] can withdraw the assignment [", hash, "]")
     );

     eosio::require_auth(owner);

     auto cw = assignment.getContentWrapper();

     auto state = cw.getOrFail(DETAILS, common::STATE)->getAs<string>();

     EOS_CHECK(
       state != common::STATE_WITHDRAWED && 
       state != common::STATE_SUSPENDED &&
       state != common::STATE_EXPIRED &&
       state != common::STATE_REJECTED,
       to_str("Cannot withdraw ", state, " assignments")
     )

     auto originalPeriods = assignment.getPeriodCount();

     auto startPeriod = assignment.getStartPeriod();

     //Calculate the number of periods since start period to the current period
     auto currentPeriod = startPeriod.getPeriodUntil(eosio::current_time_point());
    
     auto periodsToCurrent = startPeriod.getPeriodCountTo(currentPeriod);

     periodsToCurrent = std::max(periodsToCurrent, int64_t(0)) + 1;

     EOS_CHECK(
       originalPeriods >= periodsToCurrent,
       to_str("Withdrawal of expired assignment: ", hash, " is not allowed")
     );

     auto detailsGroup = cw.getGroupOrFail(DETAILS);
 
     ContentWrapper::insertOrReplace(*detailsGroup, Content { PERIOD_COUNT, periodsToCurrent });
 
     ContentWrapper::insertOrReplace(*detailsGroup, Content { common::STATE, common::STATE_WITHDRAWED });
 
     hash = m_documentGraph.updateDocument(assignment.getCreator(), 
                                           hash, 
                                           cw.getContentGroups()).getHash();
 
     assignment = Assignment(this, hash);
     
     //This has to be the last step, since adjustcmtmnt could change the assignment hash
     //to old assignments
     ContentGroups adjust {
       ContentGroup {
         Content { ASSIGNMENT_STRING, hash },
         Content { NEW_TIME_SHARE, 0 },
         Content { common::ADJUST_MODIFIER, common::MOD_WITHDRAW },
       }
     };

    
    adjustcmtmnt(owner, adjust);
   }

   void dao::suspend(name proposer, eosio::checksum256 hash, string reason)
   {
     TRACE_FUNCTION()
     EOS_CHECK(
       !isPaused(), 
       "Contract is paused for maintenance. Please try again later."
     );

     //TODO-J: Refactor Multi-tenant
     EOS_CHECK(
       Member::isMember(get_self(), proposer), 
       to_str("Only members are allowed to propose suspensions")
     );

     eosio::require_auth(proposer);

     ContentGroups cgs = {
       ContentGroup {
          Content { CONTENT_GROUP_LABEL, DETAILS },
          Content { DESCRIPTION, std::move(reason) },
          Content { ORIGINAL_DOCUMENT, hash },
       },
     };

     propose(proposer, common::SUSPEND, cgs);
   }

   void dao::claimnextper(const eosio::checksum256 &assignment_hash)
   {
      TRACE_FUNCTION()

      EOS_CHECK(
        !isPaused(), 
        "Contract is paused for maintenance. Please try again later."
      );

      Assignment assignment(this, assignment_hash);

      eosio::name assignee = assignment.getAssignee().getAccount();

      // assignee must still be a DHO member
      EOS_CHECK(Member::isMember(get_self(), assignee), "assignee must be a current member to claim pay: " + assignee.to_string());

      std::optional<Period> periodToClaim = assignment.getNextClaimablePeriod();
      EOS_CHECK(periodToClaim != std::nullopt, "All available periods for this assignment have been claimed: " + readableHash(assignment_hash));

      // require_auth(assignee);
      EOS_CHECK(has_auth(assignee) || has_auth(get_self()), "only assignee or " + get_self().to_string() + " can claim pay");

      // Valid claim identified - start process
      // process this claim
      Edge::write(get_self(), get_self(), assignment.getHash(), periodToClaim.value().getHash(), common::CLAIMED);
      int64_t periodStartSec = periodToClaim.value().getStartTime().sec_since_epoch();
      int64_t periodEndSec = periodToClaim.value().getEndTime().sec_since_epoch();

      // Pro-rate the payment if the assignment was created during the period being claimed
      //float first_phase_ratio_calc = 1.f; // pro-rate based on elapsed % of the first phase

      //EOS_CHECK(first_phase_ratio_calc <= 1, "fatal error: first_phase_ratio_calc is greater than 1: " + std::to_string(first_phase_ratio_calc));

      asset deferredSeeds;
      asset husd;
      asset hvoice;
      asset hypha;

      {
         const int64_t initTimeShare = assignment.getInitialTimeShare()
                                           .getContentWrapper()
                                           .getOrFail(DETAILS, TIME_SHARE)
                                           ->getAs<int64_t>();

         TimeShare current = assignment.getCurrentTimeShare();

         //Initialize nextOpt with current in order to have a valid initial timeShare
         auto nextOpt = std::optional<TimeShare>{current};

         int64_t currentTimeSec = periodStartSec;

         std::optional<TimeShare> lastUsedTimeShare;

         while (nextOpt)
         {

            ContentWrapper nextWrapper = nextOpt->getContentWrapper();

            const int64_t timeShare = nextWrapper.getOrFail(DETAILS, TIME_SHARE)->getAs<int64_t>();
            const int64_t startDateSec = nextWrapper.getOrFail(DETAILS, TIME_SHARE_START_DATE)->getAs<time_point>().sec_since_epoch();

            //If the time share doesn't belong to the claim period we
            //finish pro-rating
            if (periodEndSec - startDateSec <= 0)
            {
               break;
            }

            int64_t remainingTimeSec;

            lastUsedTimeShare = std::move(nextOpt.value());

            //It's possible that time share was set on previous periods,
            //if so we should use period start date as the base date
            const int64_t baseDateSec = std::max(periodStartSec, startDateSec);

            //Check if there is another timeshare
            if ((nextOpt = lastUsedTimeShare->getNext(get_self())))
            {
               const time_point commingStartDate = nextOpt->getContentWrapper().getOrFail(DETAILS, TIME_SHARE_START_DATE)->getAs<time_point>();
               const int64_t commingStartDateSec = commingStartDate.sec_since_epoch();
               //Check if the time share belongs to the same peroid
               if (commingStartDateSec >= periodEndSec)
               {
                  remainingTimeSec = periodEndSec - baseDateSec;
               }
               else
               {
                  remainingTimeSec = commingStartDateSec - baseDateSec;
               }
            }
            else
            {
               remainingTimeSec = periodEndSec - baseDateSec;
            }

            const int64_t fullPeriodSec = periodEndSec - periodStartSec;

            //Time share could only represent a portion of the whole period
            float relativeDuration = static_cast<float>(remainingTimeSec) / static_cast<float>(fullPeriodSec);
            float relativeCommitment = static_cast<float>(timeShare) / static_cast<float>(initTimeShare);
            float commitmentMultiplier = relativeDuration * relativeCommitment;

            //Accumlate each of the currencies with the time share multiplier
            //  deferredSeeds = (deferredSeeds.is_valid() ? deferredSeeds : eosio::asset{0, common::S_SEEDS}) +
            //  adjustAsset(assignment.getSalaryAmount(&common::S_SEEDS, &periodToClaim.value()), first_phase_ratio_calc * commitmentMultiplier);

            // These values are calculated when the assignment is proposed, so simply pro-rate them if/as needed
            // If there is an explicit INSTANT SEEDS amount, support sending it
            husd = (husd.is_valid() ? husd : eosio::asset{0, common::S_HUSD}) +
                   adjustAsset(assignment.getSalaryAmount(&common::S_HUSD), commitmentMultiplier);

            hvoice = (hvoice.is_valid() ? hvoice : eosio::asset{0, common::S_HVOICE}) +
                     adjustAsset(assignment.getSalaryAmount(&common::S_HVOICE), commitmentMultiplier);

            hypha = (hypha.is_valid() ? hypha : eosio::asset{0, common::S_HYPHA}) +
                    adjustAsset(assignment.getSalaryAmount(&common::S_HYPHA), commitmentMultiplier);
         }

         //If the last used time share is different from current time share
         //let's update the edge
         if (lastUsedTimeShare->getHash() != current.getHash())
         {
            Edge::get(get_self(), assignment.getHash(), common::CURRENT_TIME_SHARE).erase();
            Edge::write(get_self(), get_self(), assignment.getHash(), lastUsedTimeShare->getHash(), common::CURRENT_TIME_SHARE);
         }
      }

      // EOS_CHECK(deferredSeeds.is_valid(), "fatal error: SEEDS has to be a valid asset");
      EOS_CHECK(husd.is_valid(), "fatal error: HUSD has to be a valid asset");
      EOS_CHECK(hvoice.is_valid(), "fatal error: HVOICE has to be a valid asset");
      EOS_CHECK(hypha.is_valid(), "fatal error: HYPHA has to be a valid asset");

      string assignmentNodeLabel = "";
      if (auto [idx, assignmentLabel] = assignment.getContentWrapper().get(SYSTEM, NODE_LABEL); assignmentLabel)
      {
         EOS_CHECK(std::holds_alternative<std::string>(assignmentLabel->value), "fatal error: assignment content item type is expected to be a string: " + assignmentLabel->label);
         assignmentNodeLabel = std::get<std::string>(assignmentLabel->value);
      }

      //If node_label is not present for any reason fallback to the assignment hash
      if (assignmentNodeLabel.empty()) {
        assignmentNodeLabel = to_str(assignment.getHash());
      }

      string memo = assignmentNodeLabel + ", period: " + periodToClaim.value().getNodeLabel();      

      // string memo = "[assignment_label:" + assignmentNodeLabel + ",period_label:" + periodToClaim.value().getNodeLabel()
      //          + ",assignment_hash:" + readableHash(assignment.getHash()) + ",period_hash:" + readableHash(periodToClaim.value().getHash()) + "]";
      
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
      TRACE_FUNCTION()
      asset assetToPay = asset{0, symbol};
      if (auto [idx, assetContent] = assignment->get(DETAILS, key); assetContent)
      {
         EOS_CHECK(std::holds_alternative<eosio::asset>(assetContent->value), "fatal error: expected token type must be an asset value type: " + assetContent->label);
         assetToPay = std::get<eosio::asset>(assetContent->value);
      }
      return adjustAsset(assetToPay, proration);
   }

   std::vector<Document> dao::getCurrentBadges(Period &period, const name &member)
   {
      TRACE_FUNCTION()
      std::vector<Document> current_badges;
      std::vector<Edge> badge_assignment_edges = m_documentGraph.getEdgesFrom(Member::calcHash(member), common::ASSIGN_BADGE);
      for (Edge e : badge_assignment_edges)
      {
        Document badgeAssignmentDoc(get_self(), e.getToNode());
        Edge badge_edge = Edge::get(get_self(), badgeAssignmentDoc.getHash(), common::BADGE_NAME);
        
        //Verify badge still exists
        EOS_CHECK(
          Document::exists(get_self(), badge_edge.getToNode()),
          to_str("Badge document doesn't exits for badge assignment:", 
                 badgeAssignmentDoc.getHash(),
                 " badge:", badge_edge.getToNode())
        )

        auto badgeAssignment = badgeAssignmentDoc.getContentWrapper();
        Document badge(get_self(), badge_edge.getToNode());
        
        //Check if badge assignment is old (start_period was stored as an integer)
        //If the type of start_period is no checksum then let's skip this badge assignment
        Content* startPeriodContent = badgeAssignment.getOrFail(DETAILS, START_PERIOD);
        if (!std::holds_alternative<eosio::checksum256>(startPeriodContent->value)) {
          continue;
        }

        Period startPeriod(this, startPeriodContent->getAs<eosio::checksum256>());
        int64_t periodCount = badgeAssignment.getOrFail(DETAILS, PERIOD_COUNT)->getAs<int64_t>();
        auto endPeriod = startPeriod.getNthPeriodAfter(periodCount);
        
        int64_t badgeAssignmentStart = startPeriod.getStartTime().sec_since_epoch();
        int64_t badgeAssignmentExpiration = endPeriod.getStartTime().sec_since_epoch();

        auto periodStartSecs = period.getStartTime().sec_since_epoch();
        //Badge expiration should be compared against the claimed period instead
        //of the current time, point since it could happen that the assignment is
        //beign claimed after the badge assignment expired.
        if (badgeAssignmentStart <= periodStartSecs &&
            periodStartSecs < badgeAssignmentExpiration) {
          current_badges.push_back(badge);
        }
      }
      
      return current_badges;
   }

   eosio::asset dao::applyCoefficient(ContentWrapper &badge, const eosio::asset &base, const std::string &key)
   {
      TRACE_FUNCTION()
      if (auto [idx, coefficient] = badge.get(DETAILS, key); coefficient)
      {
         if (std::holds_alternative<std::monostate>(coefficient->value))
         {
            return asset{0, base.symbol};
         }

         EOS_CHECK(std::holds_alternative<int64_t>(coefficient->value), "fatal error: coefficient must be an int64_t type: key: " + key);

         float coeff_float = (float)((float)std::get<int64_t>(coefficient->value) / (float)10000);
         float adjustment = (float)coeff_float - (float)1;
         return adjustAsset(base, adjustment);
      }
      return asset{0, base.symbol};
   }

   dao::AssetBatch dao::applyBadgeCoefficients(Period &period, const eosio::name &member, dao::AssetBatch &ab)
   {
      TRACE_FUNCTION()
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
      TRACE_FUNCTION()
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
      TRACE_FUNCTION()
      require_auth(applicant);
      Member member(*this, applicant, applicant);
      member.apply(getRoot(get_self()), content);
   }

   void dao::enroll(const eosio::name &enroller, const eosio::name &applicant, const std::string &content)
   {
      TRACE_FUNCTION()
      require_auth(enroller);
      Member member = Member::get(*this, applicant);
      member.enroll(enroller, content);
   }

   bool dao::isPaused() { return false; }

   Document dao::getSettingsDocument()
   {
      TRACE_FUNCTION()
      auto root = getRoot(get_self());
      auto edges = m_documentGraph.getEdgesFromOrFail(root, common::SETTINGS_EDGE);
      EOS_CHECK(edges.size() == 1, "There should only exists only 1 settings edge from root node");
      return Document(get_self(), edges[0].to_node);
   }

   void dao::setsetting(const string &key, const Content::FlexValue &value)
   {
      TRACE_FUNCTION()
      require_auth(get_self());
      setSetting(key, value);
   }

   void dao::setSetting(const string &key, const Content::FlexValue &value)
   {
      TRACE_FUNCTION()
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
      TRACE_FUNCTION()
      require_auth(get_self());
      removeSetting(key);
   }

   void dao::removeSetting(const string &key)
   {
      TRACE_FUNCTION()
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
      EOS_CHECK(false, "The specified setting does not exist: " + key);
   }

   void dao::addperiod(const eosio::checksum256 &predecessor, const eosio::time_point &start_time, const string &label)
   {
      TRACE_FUNCTION()
      require_auth(get_self());

      Period newPeriod(this, start_time, label);

      // check that predecessor exists
      Document previous(get_self(), predecessor);
      ContentWrapper contentWrapper = previous.getContentWrapper();

      // if it's a period, make sure the start time comes before
      auto [idx, predecessorType] = contentWrapper.get(SYSTEM, TYPE);
      if (predecessorType && predecessorType->getAs<eosio::name>() == common::PERIOD)
      {
         EOS_CHECK(contentWrapper.getOrFail(DETAILS, START_TIME)->getAs<eosio::time_point>().sec_since_epoch() <
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
      TRACE_FUNCTION()
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
      TRACE_FUNCTION()
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
      TRACE_FUNCTION()
      require_auth(get_self());

      DocumentGraph dg(get_self());
      dg.eraseDocument(hash);
   }

   void dao::newedge(name &creator, const checksum256 &from_node, const checksum256 &to_node, const name &edge_name)
   {
      TRACE_FUNCTION()
      require_auth(get_self());
      Edge edge(get_self(), creator, from_node, to_node, edge_name);
   }

   void dao::killedge(const uint64_t id)
   {
      TRACE_FUNCTION()
      require_auth(get_self());
      Edge::edge_table e_t(get_self(), get_self().value);
      auto itr = e_t.find(id);
      e_t.erase(itr);
   }

   void dao::setalert(const eosio::name &level, const std::string &content)
   {
      TRACE_FUNCTION()
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
      TRACE_FUNCTION()
      Edge alertEdge = Edge::get(get_self(), getRoot(get_self()), common::ALERT);
      Document alert(get_self(), alertEdge.getToNode());
      getGraph().eraseDocument(alert.getHash());
   }

   DocumentGraph &dao::getGraph()
   {
      return m_documentGraph;
   }

   /**
  * Info Structure
  * 
  * ContentGroups
  * [
  *   Group Assignment 0 Details: [
  *     assignment: [checksum256]
  *     new_time_share_x100: [int64_t] min_time_share_x100 <= new_time_share_x100 <= time_share_x100
  *     modifier: [withdraw]
  *   ],
  *   Group Assignment 1 Details: [
  *     assignment: [checksum256]
  *     new_time_share_x100: [int64_t] min_time_share_x100 <= new_time_share_x100 <= time_share_x100
  *   ],
  *   Group Assignment N Details: [
  *     assignment: [checksum256]
  *     new_time_share_x100: [int64_t] min_time_share_x100 <= new_time_share_x100 <= time_share_x100
  *   ]
  * ]
  */
   ACTION dao::adjustcmtmnt(name issuer, ContentGroups &adjust_info)
   {
      TRACE_FUNCTION()
      require_auth(issuer);

      ContentWrapper cw(adjust_info);

      for (size_t i = 0; i < adjust_info.size(); ++i)
      {

         Assignment assignment = Assignment(this,
                                            cw.getOrFail(i, "assignment").second->getAs<checksum256>());

         EOS_CHECK(assignment.getAssignee().getAccount() == issuer,
                      "Only the owner of the assignment can adjust it");

         int64_t newTimeShare = cw.getOrFail(i, NEW_TIME_SHARE).second->getAs<int64_t>();

         auto [modifierIdx, modifier] = cw.get(i, common::ADJUST_MODIFIER);

         string_view modstr = modifier ? string_view{modifier->getAs<string>()} : string_view{};

         auto [idx, startDate] = cw.get(i, TIME_SHARE_START_DATE);

         auto fixedStartDate = startDate ? std::optional<time_point>{startDate->getAs<time_point>()} : std::nullopt;

         auto assignmentCW = assignment.getContentWrapper();

         if (modstr.empty()) {
           
           auto lastPeriod = assignment.getLastPeriod();
           auto assignmentExpirationTime = lastPeriod.getEndTime();

           EOS_CHECK(
             assignmentExpirationTime.sec_since_epoch() > eosio::current_time_point().sec_since_epoch(),
             to_str("Cannot adjust expired assignment: ", assignment.getHash(), " last period: ", lastPeriod.getHash())
           )

           auto state = assignmentCW.getOrFail(DETAILS, common::STATE)->getAs<string>();

           EOS_CHECK(
             state != common::STATE_WITHDRAWED && 
             state != common::STATE_SUSPENDED &&
             state != common::STATE_EXPIRED &&
             state != common::STATE_REJECTED,
             to_str("Cannot adjust commitment for ", state, " assignments")
           )
         }
        
         modifyCommitment(assignment, 
                          newTimeShare, 
                          fixedStartDate,  
                          modstr);
      }
   }

  ACTION dao::adjustdeferr(name issuer, checksum256 assignment_hash, int64_t new_deferred_perc_x100)
  {
    TRACE_FUNCTION()

    require_auth(issuer);

    Assignment assignment = Assignment(this, assignment_hash);

    EOS_CHECK(
      assignment.getAssignee().getAccount() == issuer,
      "Only the owner of the assignment can modify it"
    );

    auto cw = assignment.getContentWrapper();

    auto [detailsIdx, detailsGroup] = cw.getGroup(DETAILS);

    EOS_CHECK(
      detailsGroup != nullptr,
      "Missing details group from assignment"
    );

    auto approvedDeferredPerc = cw.getOrFail(detailsIdx, common::APPROVED_DEFERRED)
                                  .second->getAs<int64_t>();

    EOS_CHECK(
      new_deferred_perc_x100 >= approvedDeferredPerc,
      to_str("New percentage has to be greater or equal to approved percentage: ", approvedDeferredPerc)
    )

    const int64_t UPPER_LIMIT = 100;

    EOS_CHECK(
      approvedDeferredPerc <= new_deferred_perc_x100 && new_deferred_perc_x100 <= UPPER_LIMIT,
      to_str("New percentage is out of valid range [", 
             approvedDeferredPerc, " - ", UPPER_LIMIT, "]:", new_deferred_perc_x100)
    )

    asset usdPerPeriod = cw.getOrFail(detailsIdx, USD_SALARY_PER_PERIOD)
                           .second->getAs<eosio::asset>();

    int64_t initialTimeshare = assignment.getInitialTimeShare()
                                         .getContentWrapper()
                                         .getOrFail(DETAILS, TIME_SHARE)->getAs<int64_t>();

    auto usdPerPeriodCommitmentAdjusted = adjustAsset(usdPerPeriod, static_cast<float>(initialTimeshare) / 100.f);

    auto deferred = new_deferred_perc_x100 / 100.f;
          
    auto husdVal = adjustAsset(usdPerPeriodCommitmentAdjusted, 1.f - deferred);
    husdVal.symbol = common::S_HUSD;
    
    cw.insertOrReplace(*detailsGroup, Content{
      DEFERRED,
      new_deferred_perc_x100
    });

    auto hyphaUsdVal = getSettingOrFail<eosio::asset>(common::HYPHA_USD_VALUE);

    EOS_CHECK(
      hyphaUsdVal.symbol.precision() == 4,
      util::to_str("Expected HYPHA_USD_VALUE precision to be 4, but got:", hyphaUsdVal.symbol.precision())
    )

    auto hyphaVal = adjustAsset(usdPerPeriodCommitmentAdjusted, deferred);

    hyphaVal.set_amount(hyphaVal.amount / (hyphaUsdVal.amount * 0.0001));
    hyphaVal.symbol = common::S_HYPHA;

    cw.insertOrReplace(*detailsGroup, Content{
      HYPHA_SALARY_PER_PERIOD, 
      hyphaVal
    });

    cw.insertOrReplace(*detailsGroup, Content{
      HUSD_SALARY_PER_PERIOD,
      husdVal
    });

    m_documentGraph.updateDocument(issuer, assignment_hash, cw.getContentGroups());
  }

   void dao::modifyCommitment(Assignment& assignment, int64_t commitment, std::optional<eosio::time_point> fixedStartDate, std::string_view modifier)
   {
      TRACE_FUNCTION()
      
      ContentWrapper assignmentCW = assignment.getContentWrapper();

      //Should use the role edge instead of the role content item
      //in case role document is modified (causes it's hash to change)
      auto assignmentToRoleEdge = m_documentGraph.getEdgesFrom(assignment.getHash(), common::ROLE_NAME);
      
      EOS_CHECK(
        !assignmentToRoleEdge.empty(),
        to_str("Missing 'role' edge from assignment: ", assignment.getHash())
      )

      Document roleDocument(get_self(), assignmentToRoleEdge.at(0).getToNode());
      auto role = roleDocument.getContentWrapper();

      //Check min_time_share_x100 <= new_time_share_x100 <= time_share_x100
      int64_t originalTimeShare = assignmentCW.getOrFail(DETAILS, TIME_SHARE)->getAs<int64_t>();
      int64_t minTimeShare = 0;
              
      EOS_CHECK(
        commitment >= minTimeShare,
        to_str(NEW_TIME_SHARE, " must be greater than or equal to: ", minTimeShare, " You submitted: ", commitment)
      );

      EOS_CHECK(
        commitment <= originalTimeShare,
        to_str(NEW_TIME_SHARE, " must be less than or equal to original (approved) time_share_x100: ", originalTimeShare, " You submitted: ", commitment)
      );

      //Update lasttimeshare
      Edge lastTimeShareEdge = Edge::get(get_self(), assignment.getHash(), common::LAST_TIME_SHARE);

      time_point startDate = eosio::current_time_point();

      TimeShare lastTimeShare(get_self(), lastTimeShareEdge.getToNode());

      if (fixedStartDate)
      {
        time_point lastStartDate = lastTimeShare.getContentWrapper()
                                                .getOrFail(DETAILS, TIME_SHARE_START_DATE)
                                                ->getAs<time_point>();

        startDate = *fixedStartDate;
        EOS_CHECK(lastStartDate.sec_since_epoch() < startDate.sec_since_epoch(),
                      "New time share start date must be greater than the previous time share start date");
      }

      int64_t lastTimeSharex100 = lastTimeShare.getContentWrapper()
                                               .getOrFail(DETAILS, TIME_SHARE)
                                               ->getAs<int64_t>();

      //This allows withdrawing/suspending an assignment which has a current commitment of 0
      if (modifier != common::MOD_WITHDRAW) 
      {
        EOS_CHECK(
          lastTimeSharex100 != commitment,
          to_str("New commitment: [", commitment, "] must be different than current commitment: [", lastTimeSharex100, "]")
        );
      }

      TimeShare newTimeShareDoc(get_self(), 
                                assignment.getAssignee().getAccount(), 
                                commitment, 
                                startDate, 
                                assignment.getHash());

      Edge::write(get_self(), get_self(), lastTimeShareEdge.getToNode(), newTimeShareDoc.getHash(), common::NEXT_TIME_SHARE);

      lastTimeShareEdge.erase();

      Edge::write(get_self(), get_self(), assignment.getHash(), newTimeShareDoc.getHash(), common::LAST_TIME_SHARE);
   }
} // namespace hypha
