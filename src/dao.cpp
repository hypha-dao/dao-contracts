#include <algorithm>
#include <limits>
#include <cmath>

#include <document_graph/content_wrapper.hpp>
#include <document_graph/util.hpp>
#include <proposals/proposal.hpp>
#include <proposals/proposal_factory.hpp>
#include <payers/payer_factory.hpp>
#include <payers/payer.hpp>
#include <logger/logger.hpp>
#include <memory>

#include <dao.hpp>
#include <common.hpp>
#include <util.hpp>
#include <member.hpp>
#include <period.hpp>
#include <assignment.hpp>
#include <time_share.hpp>
#include <settings.hpp>

namespace hypha
{
   /**Testenv only 
   
   ACTION dao::clean()
   {
     require_auth(get_self());

     Document::document_table docs(get_self(), get_self().value);

     for (auto it = docs.begin(); it != docs.end(); ) {
       it = docs.erase(it);
     }

     Edge::edge_table edges(get_self(), get_self().value);

     for (auto it = edges.begin(); it != edges.end(); ) {
       it = edges.erase(it);
     }
   }
   
   ACTION
   dao::autoenroll(const checksum256& dao_hash, const name& enroller, const name& member)
   {
     require_auth(get_self());

     //Auto enroll      
     std::unique_ptr<Member> mem;
     
     const checksum256 memberHash = Member::calcHash(member);
 
     if (Document::exists(get_self(), memberHash)) {
       mem = std::make_unique<Member>(*this, memberHash);
     }
     else {
       mem = std::make_unique<Member>(*this, member, member);
     }

     Document daoDoc(get_self(), dao_hash);

     mem->apply(daoDoc.getID(), "Auto enrolled member");
     mem->enroll(enroller, daoDoc.getID(), "Auto enrolled member");
   }

   ACTION dao::editdoc(uint64_t doc_id, const std::string& group, const std::string& key, const Content::FlexValue &value)
   {
     require_auth(get_self());

     Document doc(get_self(), doc_id);

     auto cw = doc.getContentWrapper();

     cw.insertOrReplace(*cw.getGroupOrFail(group), Content{key, value});

     doc.update(get_self(), std::move(cw.getContentGroups()));
   }

   ACTION dao::addedge(const checksum256& from, const checksum256& to, const name& edge_name)
   {
     require_auth(get_self());

     Document fromDoc(get_self(), from);
     Document toDoc(get_self(), to);

     Edge(get_self(), get_self(), fromDoc.getID(), toDoc.getID(), edge_name);
   }
   */

   void dao::propose(const checksum256& dao_hash,
                     const name &proposer,
                     const name &proposal_type,
                     ContentGroups &content_groups,
                     bool publish)
   {
      TRACE_FUNCTION()
      EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

      //TODO: Change to dao_id instead of dao_hash
      Document daoDoc(get_self(), dao_hash);

      std::unique_ptr<Proposal> proposal = std::unique_ptr<Proposal>(ProposalFactory::Factory(*this, daoDoc.getID(), proposal_type));
      proposal->propose(proposer, content_groups, publish);
   }

   void dao::vote(const name& voter, const checksum256 &proposal_hash, string &vote, const std::optional<string> &notes)
   {
      TRACE_FUNCTION()
      EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");
      Document docprop(get_self(), proposal_hash);
      name proposal_type = docprop.getContentWrapper().getOrFail(SYSTEM, TYPE)->getAs<eosio::name>();

      auto daoID = Edge::get(get_self(), docprop.getID(), common::DAO).getToNode();

      Proposal *proposal = ProposalFactory::Factory(*this, daoID, proposal_type);
      proposal->vote(voter, vote, docprop, notes);
   }

   void dao::closedocprop(const checksum256 &proposal_hash)
   {
      TRACE_FUNCTION()
      EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");
      
      Document docprop(get_self(), proposal_hash);

      auto daoID = Edge::get(get_self(), docprop.getID(), common::DAO).getToNode();

      name proposal_type = docprop.getContentWrapper().getOrFail(SYSTEM, TYPE)->getAs<eosio::name>();

      Proposal *proposal = ProposalFactory::Factory(*this, daoID, proposal_type);
      proposal->close(docprop);
   }

   void dao::proposepub(const name &proposer, const checksum256 &proposal_hash)
   {
      TRACE_FUNCTION()
      EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

      Document docprop(get_self(), proposal_hash);
      name proposal_type = docprop.getContentWrapper().getOrFail(SYSTEM, TYPE)->getAs<eosio::name>();

      auto daoID = Edge::get(get_self(), docprop.getID(), common::DAO).getToNode();

      Proposal *proposal = ProposalFactory::Factory(*this, daoID, proposal_type);
      proposal->publish(proposer, docprop);
   }

   void dao::proposerem(const name &proposer, const checksum256 &proposal_hash)
   {
      TRACE_FUNCTION()
      EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

      Document docprop(get_self(), proposal_hash);
      name proposal_type = docprop.getContentWrapper().getOrFail(SYSTEM, TYPE)->getAs<eosio::name>();
      
      auto daoID = Edge::get(get_self(), docprop.getID(), common::DAO).getToNode();

      Proposal *proposal = ProposalFactory::Factory(*this, daoID, proposal_type);
      proposal->remove(proposer, docprop);
   }

   void dao::proposeupd(const name &proposer, const checksum256 &proposal_hash, ContentGroups &content_groups) 
   {
      TRACE_FUNCTION()
      EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

      Document docprop(get_self(), proposal_hash);
      name proposal_type = docprop.getContentWrapper().getOrFail(SYSTEM, TYPE)->getAs<eosio::name>();

      auto daoID = Edge::get(get_self(), docprop.getID(), common::DAO).getToNode();

      Proposal *proposal = ProposalFactory::Factory(*this, daoID, proposal_type);
      proposal->update(proposer, docprop, content_groups);
   }

   void dao::proposeextend(const checksum256 &assignment_hash, const int64_t additional_periods)
   {
      TRACE_FUNCTION()
      EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

      Assignment assignment(this, assignment_hash);

      // only the assignee can submit an extension proposal
      eosio::name assignee = assignment.getAssignee().getAccount();
      eosio::require_auth(assignee);
      
      auto daoID = Edge::get(get_self(), assignment.getID(), common::DAO).getToNode();

      //Get the DAO edge from the hash
      EOS_CHECK(
        Member::isMember(get_self(), daoID, assignee), 
        "assignee must be a current member to request an extension: " + assignee.to_string());

      eosio::print("\nproposer is: " + assignee.to_string() + "\n");
      // construct ContentGroups to call propose
      auto contentGroups = ContentGroups{
        ContentGroup{
              Content(CONTENT_GROUP_LABEL, DETAILS),
              Content(PERIOD_COUNT, assignment.getPeriodCount() + additional_periods),
              Content(TITLE, std::string("Assignment Extension Proposal")),
              Content(ORIGINAL_DOCUMENT, assignment.getHash())
        }
      };

      // propose the extension
      std::unique_ptr<Proposal> proposal = std::unique_ptr<Proposal>(ProposalFactory::Factory(*this, daoID, common::EXTENSION));
      proposal->propose(assignee, contentGroups, false);
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
       state != common::STATE_PROPOSED &&
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
                                           assignment.getID(), 
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

     Document doc(get_self(), hash);

     //Get the DAO edge from the hash
     auto daoID = Edge::get(get_self(), doc.getID(), common::DAO).getToNode();

     EOS_CHECK(
       Member::isMember(get_self(), daoID, proposer), 
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

     Document daoDoc(get_self(), daoID);

     propose(daoDoc.getHash(), proposer, common::SUSPEND, cgs, true);
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
      auto daoID = Edge::get(get_self(), assignment.getID(), common::DAO).getToNode();
      
      EOS_CHECK(
        Member::isMember(get_self(), daoID, assignee), 
        "assignee must be a current member to claim pay: " + assignee.to_string()
      );

      std::optional<Period> periodToClaim = assignment.getNextClaimablePeriod();
      EOS_CHECK(periodToClaim != std::nullopt, "All available periods for this assignment have been claimed: " + readableHash(assignment_hash));

      // require_auth(assignee);
      EOS_CHECK(has_auth(assignee) || has_auth(get_self()), "only assignee or " + get_self().to_string() + " can claim pay");

      // Valid claim identified - start process
      // process this claim
      Edge::write(get_self(), get_self(), assignment.getID(), periodToClaim.value().getID(), common::CLAIMED);
      int64_t periodStartSec = periodToClaim.value().getStartTime().sec_since_epoch();
      int64_t periodEndSec = periodToClaim.value().getEndTime().sec_since_epoch();

      // Pro-rate the payment if the assignment was created during the period being claimed
      //float first_phase_ratio_calc = 1.f; // pro-rate based on elapsed % of the first phase

      //EOS_CHECK(first_phase_ratio_calc <= 1, "fatal error: first_phase_ratio_calc is greater than 1: " + std::to_string(first_phase_ratio_calc));

      auto daoSettings = getSettingsDocument(daoID);

      auto daoTokens = AssetBatch{
        .reward = daoSettings->getOrFail<asset>(common::REWARD_TOKEN),
        .peg = daoSettings->getOrFail<asset>(common::PEG_TOKEN),
        .voice = daoSettings->getOrFail<asset>(common::VOICE_TOKEN)
      };
      
      const asset pegSalary = assignment.getPegSalary();
      const asset voiceSalary = assignment.getVoiceSalary();
      const asset rewardSalary = assignment.getRewardSalary();

      asset deferredSeeds;
      asset peg;
      asset voice;
      asset reward;

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

            peg = (peg.is_valid() ? peg : eosio::asset{0, daoTokens.peg.symbol}) +
                   adjustAsset(pegSalary, commitmentMultiplier);

            voice = (voice.is_valid() ? voice : eosio::asset{0, daoTokens.voice.symbol}) +
                     adjustAsset(voiceSalary, commitmentMultiplier);

            reward = (reward.is_valid() ? reward : eosio::asset{0, daoTokens.reward.symbol}) +
                    adjustAsset(rewardSalary, commitmentMultiplier);
         }

         //If the last used time share is different from current time share
         //let's update the edge
         if (lastUsedTimeShare->getHash() != current.getHash())
         {
            Edge::get(get_self(), assignment.getID(), common::CURRENT_TIME_SHARE).erase();
            Edge::write(get_self(), get_self(), assignment.getID(), lastUsedTimeShare->getID(), common::CURRENT_TIME_SHARE);
         }
      }

      // EOS_CHECK(deferredSeeds.is_valid(), "fatal error: SEEDS has to be a valid asset");
      EOS_CHECK(peg.is_valid(), "fatal error: PEG has to be a valid asset");
      EOS_CHECK(voice.is_valid(), "fatal error: VOICE has to be a valid asset");
      EOS_CHECK(reward.is_valid(), "fatal error: REWARD has to be a valid asset");

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
      ab.reward = reward;
      ab.voice = voice;
      ab.peg = peg;

      ab = applyBadgeCoefficients(periodToClaim.value(), assignee, daoID, ab);

      makePayment(daoSettings, periodToClaim.value().getID(), assignee, ab.reward, memo, eosio::name{0}, daoTokens);
      makePayment(daoSettings, periodToClaim.value().getID(), assignee, ab.voice, memo, eosio::name{0}, daoTokens);
      makePayment(daoSettings, periodToClaim.value().getID(), assignee, ab.peg, memo, eosio::name{0}, daoTokens);
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

   std::vector<Document> dao::getCurrentBadges(Period &period, const name &member, uint64_t dao)
   {
      TRACE_FUNCTION()
      std::vector<Document> current_badges;

      Member memDoc(*this, Member::calcHash(member));

      std::vector<Edge> badge_assignment_edges = m_documentGraph.getEdgesFrom(memDoc.getID(), common::ASSIGN_BADGE);
      for (Edge e : badge_assignment_edges)
      {
        Document badgeAssignmentDoc(get_self(), e.getToNode());
        Edge badge_edge = Edge::get(get_self(), badgeAssignmentDoc.getID(), common::BADGE_NAME);
        
        //Verify badge still exists
        EOS_CHECK(
          Document::exists(get_self(), badge_edge.getToNode()),
          util::to_str("Badge document doesn't exits for badge assignment:", 
                 badgeAssignmentDoc.getHash(),
                 " badge:", badge_edge.getToNode())
        )

        //Verify the badge is actually from the requested DAO
        if (Edge::get(get_self(), badge_edge.getToNode(), common::DAO).getToNode() != dao) {
          continue;
        }

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

   AssetBatch dao::applyBadgeCoefficients(Period &period, const eosio::name &member, uint64_t dao, AssetBatch &ab)
   {
      TRACE_FUNCTION()
      // get list of badges
      auto badges = getCurrentBadges(period, member, dao);
      AssetBatch applied_assets = ab;

      // for each badge, apply appropriate coefficients
      for (auto &badge : badges)
      {
         ContentWrapper badgeCW = badge.getContentWrapper();
         applied_assets.reward = applied_assets.reward + applyCoefficient(badgeCW, ab.reward, common::REWARD_COEFFICIENT);
         applied_assets.peg = applied_assets.peg + applyCoefficient(badgeCW, ab.peg, common::PEG_COEFFICIENT);
         applied_assets.voice = applied_assets.voice + applyCoefficient(badgeCW, ab.voice, common::VOICE_COEFFICIENT);
      }

      return applied_assets;
   }

   void dao::makePayment(Settings* daoSettings,
                         uint64_t fromNode,
                         const eosio::name &recipient,
                         const eosio::asset &quantity,
                         const string &memo,
                         const eosio::name &paymentType,
                         const AssetBatch& daoTokens)
   {
      TRACE_FUNCTION()
      // nothing to do if quantity is zero of symbol is USD, a known placeholder
      if (quantity.amount == 0 || quantity.symbol == common::S_USD)
      {
         return;
      }

      std::unique_ptr<Payer> payer = std::unique_ptr<Payer>(PayerFactory::Factory(*this, daoSettings, quantity.symbol, paymentType, daoTokens));
      Document paymentReceipt = payer->pay(recipient, quantity, memo);
      Document recipientDoc(get_self(), Member::calcHash(recipient));
      Edge::write(get_self(), get_self(), fromNode, paymentReceipt.getID(), common::PAYMENT);
      Edge::write(get_self(), get_self(), recipientDoc.getID(), paymentReceipt.getID(), common::PAID);
   }

   void dao::apply(const eosio::name &applicant, const checksum256& dao_hash, const std::string &content)
   {
      TRACE_FUNCTION()
      require_auth(applicant);
      Document daoDoc(get_self(), dao_hash);

      std::unique_ptr<Member> member;
      
      const checksum256 memberHash = Member::calcHash(applicant);

      if (Document::exists(get_self(), memberHash)) {
        member = std::make_unique<Member>(*this, memberHash);
      }
      else {
        member = std::make_unique<Member>(*this, applicant, applicant);
      }

      member->apply(daoDoc.getID(), content);
   }

   void dao::enroll(const eosio::name &enroller, const checksum256& dao_hash, const eosio::name &applicant, const std::string &content)
   {
      TRACE_FUNCTION()

      require_auth(enroller);

      Document daoDoc(get_self(), dao_hash);

      checkEnrollerAuth(daoDoc.getID(), enroller);

      Member member = Member::get(*this, applicant);
      member.enroll(enroller, daoDoc.getID(), content);
   }

   bool dao::isPaused() { return false; }

   Settings* dao::getSettingsDocument(const eosio::name &dao_name)
   {
      TRACE_FUNCTION()
      return getSettingsDocument(getDAO(dao_name));
   }

   Settings* dao::getSettingsDocument(const checksum256& daoHash)
   {
     TRACE_FUNCTION()
     Document daoDoc(get_self(), daoHash);
     return getSettingsDocument(daoDoc.getID());
   }

   Settings* dao::getSettingsDocument(uint64_t daoID)
   {
     TRACE_FUNCTION();

     //Check if it'S already loaded in cache
     for (auto& settingsDoc : m_settingsDocs) {
       if (settingsDoc->getRootID() == daoID) {
         return settingsDoc.get();
       }
     }

     //If not then we have to load it
     auto edges = m_documentGraph.getEdgesFromOrFail(daoID, common::SETTINGS_EDGE);
     EOS_CHECK(edges.size() == 1, "There should only exists only 1 settings edge from a dao node");
    
     m_settingsDocs.emplace_back(std::make_unique<Settings>(
       *this,
       edges[0].to_node,
       daoID
     ));

     return m_settingsDocs.back().get();
   }

   Settings* dao::getSettingsDocument()
   {
      TRACE_FUNCTION()
      return getSettingsDocument(getRoot(get_self()));
   }

   void dao::setsetting(const string &key, const Content::FlexValue &value, std::optional<std::string> group)
   {
      TRACE_FUNCTION()
      require_auth(get_self());
      auto settings = getSettingsDocument();

      settings->setSetting(group.value_or(string{"settings"}), Content{key, value});
   }

   void dao::setdaosetting(const uint64_t& dao_id, const string &key, const Content::FlexValue &value, std::optional<std::string> group)
   {
     TRACE_FUNCTION()
     checkAdminsAuth(dao_id);
     auto settings = getSettingsDocument(dao_id);
     settings->setSetting(group.value_or(string{"settings"}), Content{key, value});
   }

   void dao::adddaosetting(const uint64_t& dao_id, const std::string &key, const Content::FlexValue &value, std::optional<std::string> group)
   {
     TRACE_FUNCTION()     
     checkAdminsAuth(dao_id);
     auto settings = getSettingsDocument(dao_id);
     settings->addSetting(group.value_or(string{"settings"}), Content{key, value});
   }

   void dao::remdaosetting(const uint64_t& dao_id, const std::string &key, std::optional<std::string> group)
   {
     TRACE_FUNCTION()     
     checkAdminsAuth(dao_id);
     auto settings = getSettingsDocument(dao_id);
     settings->remSetting(group.value_or(string{"settings"}), key);
   }
   
   void dao::remkvdaoset(const uint64_t& dao_id, const std::string &key, const Content::FlexValue &value, std::optional<std::string> group)
   {
     TRACE_FUNCTION()
     checkAdminsAuth(dao_id);
     auto settings = getSettingsDocument(dao_id);
     settings->remKVSetting(group.value_or(string{"settings"}), Content{ key, value });
   }

   void dao::remsetting(const string &key)
   {
      TRACE_FUNCTION()
      require_auth(get_self());
      auto settings = getSettingsDocument();
      settings->remSetting(key);
   }

   void  dao::addenroller(const uint64_t dao_id, name enroller_account)
   {
     TRACE_FUNCTION()
     checkAdminsAuth(dao_id);
     Edge::getOrNew(get_self(), get_self(), dao_id, getMemberDoc(enroller_account).getID(), common::ENROLLER);
   }
   void dao::addadmin(const uint64_t dao_id, name admin_account)
   {
     TRACE_FUNCTION()
     checkAdminsAuth(dao_id);
     Edge::getOrNew(get_self(), get_self(), dao_id, getMemberDoc(admin_account).getID(), common::ADMIN);
   }
   void dao::remenroller(const uint64_t dao_id, name enroller_account)
   {
     TRACE_FUNCTION()
     checkAdminsAuth(dao_id);
     Edge::get(get_self(), dao_id, getMemberDoc(enroller_account).getID(), common::ENROLLER).erase();
   }
   void dao::remadmin(const uint64_t dao_id, name admin_account)
   {
     TRACE_FUNCTION()
     checkAdminsAuth(dao_id);
     Edge::get(get_self(), dao_id, getMemberDoc(admin_account).getID(), common::ADMIN).erase();
   }

   ACTION dao::genperiods(uint64_t dao_id, int64_t period_count/*, int64_t period_duration_sec*/)
   {
      TRACE_FUNCTION()

      checkAdminsAuth(dao_id);

      genPeriods(dao_id, period_count);
   }

   void dao::createdao(ContentGroups &config)
   {
      TRACE_FUNCTION()

      //Extract mandatory configurations
      auto configCW = ContentWrapper(config);

      auto [detailsIdx, _] = configCW.getGroup(DETAILS);

      EOS_CHECK(
        detailsIdx != -1,
        to_str("Missing Details Group")
      )

      auto daoName = configCW.getOrFail(detailsIdx, DAO_NAME).second;

      const name dao = daoName->getAs<name>();
      
      Document daoDoc(get_self(), get_self(), getDAOContent(daoName->getAs<name>()));

      //Verify Root exists
      Document root(get_self(), getRoot(get_self()));

      Edge(get_self(), get_self(), root.getID(), daoDoc.getID(), common::DAO);
      
      // dao_table daos(get_self(), get_self().value);

      // auto bynameIdx = daos.get_index<"byname"_n>();

      // EOS_CHECK(
      //   bynameIdx.find(dao_name.value) == bynameIdx.end(),
      //   util::to_str("Dao with name: ", dao_name, " already exists")
      // )

      // daos.emplace(get_self(), [&](Dao& newDao){ 
      //   newDao.id = daos.available_primary_key();
      //   newDao.name = dao_name;
      //   newDao.hash = dao.getHash();
      // });

      auto votingDurationSeconds = configCW.getOrFail(detailsIdx, VOTING_DURATION_SEC).second;

      EOS_CHECK(
        votingDurationSeconds->getAs<int64_t>() > 0,
        util::to_str(VOTING_DURATION_SEC, " has to be a positive number")
      )

      auto daoDescription = configCW.getOrFail(detailsIdx, common::DAO_DESCRIPTION).second;

      EOS_CHECK(
        daoDescription->getAs<std::string>().size() <= 512,
        "Dao description has be less than 512 characters"  
      )

      auto daoTitle = configCW.getOrFail(detailsIdx, common::DAO_TITLE).second;

      EOS_CHECK(
        daoTitle->getAs<std::string>().size() <= 48,
        "Dao title has be less than 48 characters"  
      )

      auto pegToken = configCW.getOrFail(detailsIdx, common::PEG_TOKEN).second;

      auto voiceToken = configCW.getOrFail(detailsIdx, common::VOICE_TOKEN).second;
      auto voiceTokenDecayPeriod = configCW.getOrFail(detailsIdx, "voice_token_decay_period").second;
      auto voiceTokenDecayPerPeriodX10M = configCW.getOrFail(detailsIdx, "voice_token_decay_per_period_x10M").second;

      auto rewardToken = configCW.getOrFail(detailsIdx, common::REWARD_TOKEN).second;

      auto rewardToPegTokenRatio = configCW.getOrFail(detailsIdx, common::REWARD_TO_PEG_RATIO).second;

      rewardToPegTokenRatio->getAs<asset>();

      //Generate periods
      //auto inititialPeriods = configCW.getOrFail(detailsIdx, PERIOD_COUNT);

      auto periodDurationSeconds = configCW.getOrFail(detailsIdx, common::PERIOD_DURATION).second;

      EOS_CHECK(
        periodDurationSeconds->getAs<int64_t>() > 0,
        util::to_str(common::PERIOD_DURATION, " has to be a positive number")
      )

      auto onboarderAcc = configCW.getOrFail(detailsIdx, common::ONBOARDER_ACCOUNT).second;
      
      const name onboarder = onboarderAcc->getAs<name>();

      auto votingQuorum = configCW.getOrFail(detailsIdx, VOTING_QUORUM_FACTOR_X100).second;

      votingQuorum->getAs<int64_t>();

      auto votingAllignment = configCW.getOrFail(detailsIdx, VOTING_ALIGNMENT_FACTOR_X100).second;

      votingAllignment->getAs<int64_t>();

      int64_t useSeeds = 0;

      if (auto [_, daoUsesSeeds] = configCW.get(detailsIdx, common::DAO_USES_SEEDS); 
          daoUsesSeeds) 
      {
        useSeeds = daoUsesSeeds->getAs<int64_t>();
      }

      require_auth(onboarder);
      
      // Create the settings document as well and add an edge to it
      ContentGroups settingCgs{
          ContentGroup{
              Content(CONTENT_GROUP_LABEL, SETTINGS),
              *daoName,
              *daoTitle,
              *daoDescription,
              *pegToken,
              *voiceToken,
              *rewardToken,
              *rewardToPegTokenRatio,
              *periodDurationSeconds,
              *votingDurationSeconds,
              *onboarderAcc,
              *votingQuorum,
              *votingAllignment,
              Content{common::DAO_USES_SEEDS, useSeeds}
          },
          ContentGroup{
              Content(CONTENT_GROUP_LABEL, SYSTEM),
              Content(TYPE, common::SETTINGS_EDGE),
              Content(NODE_LABEL, "Settings")
          }
      };

      Document settingsDoc(get_self(), get_self(), std::move(settingCgs));
      Edge::write(get_self(), get_self(), daoDoc.getID(), settingsDoc.getID(), common::SETTINGS_EDGE);

      createVoiceToken(
          dao,
          voiceToken->getAs<asset>(),
          voiceTokenDecayPeriod->getAs<int64_t>(),
          voiceTokenDecayPerPeriodX10M->getAs<int64_t>()
      );

      createTokens(dao, rewardToken->getAs<asset>(), pegToken->getAs<asset>());

      //Auto enroll
      std::unique_ptr<Member> member;
      
      const checksum256 memberHash = Member::calcHash(onboarder);

      if (Document::exists(get_self(), memberHash)) {
        member = std::make_unique<Member>(*this, memberHash);
      }
      else {
        member = std::make_unique<Member>(*this, onboarder, onboarder);
      }

      member->apply(daoDoc.getID(), "DAO Onboarder");
      member->enroll(onboarder, daoDoc.getID(), "DAO Onboarder");
      
      //Create owner, admin and enroller edges
      Edge(get_self(), get_self(), daoDoc.getID(), member->getID(), common::ENROLLER);
      Edge(get_self(), get_self(), daoDoc.getID(), member->getID(), common::OWNER);
      Edge(get_self(), get_self(), daoDoc.getID(), member->getID(), common::ADMIN);

      //Create start period
      Period newPeriod(this, eosio::current_time_point(), util::to_str(dao, " start period"));

      //Create start & end edges
      Edge(get_self(), get_self(), daoDoc.getID(), newPeriod.getID(), common::START);
      Edge(get_self(), get_self(), daoDoc.getID(), newPeriod.getID(), common::END);
      Edge(get_self(), get_self(), daoDoc.getID(), newPeriod.getID(), common::PERIOD);
      Edge(get_self(), get_self(), newPeriod.getID(), daoDoc.getID(), common::DAO);

      // EOS_CHECK(
      //   inititialPeriods->getAs<int64_t>() - 1 <= 30,
      //   util::to_str("The max number of initial periods is 30")
      // )

      // genPeriods(
      //   dao, 
      //   inititialPeriods->getAs<int64_t>() - 1, 
      //   periodDurationSeconds->getAs<int64_t>()
      // );
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
      Edge::write(get_self(), get_self(), rootDoc.getID(), settingsDoc.getID(), common::SETTINGS_EDGE);
   }

   void dao::setalert(const eosio::name &level, const std::string &content)
   {
      TRACE_FUNCTION()

      Document rootDoc(get_self(), getRoot(get_self()));

      auto [exists, edge] = Edge::getIfExists(get_self(), rootDoc.getID(), common::ALERT);
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

      Edge::write(get_self(), get_self(), rootDoc.getID(), alert.getID(), common::ALERT);
   }

   void dao::remalert(const string &notes)
   {
      TRACE_FUNCTION()

      Document rootDoc(get_self(), getRoot(get_self()));

      Edge alertEdge = Edge::get(get_self(), rootDoc.getID(), common::ALERT);
      Document alert(get_self(), alertEdge.getToNode());
      getGraph().eraseDocument(alert.getID());
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
             state != common::STATE_PROPOSED &&
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

    auto daoSettings = getSettingsDocument(assignment.getDaoID());

    auto usdPerPeriodCommitmentAdjusted = normalizeToken(usdPerPeriod) * (initialTimeshare / 100.0);

    auto deferred = new_deferred_perc_x100 / 100.0;
          
    auto pegVal = usdPerPeriodCommitmentAdjusted * (1.0 - deferred);
    //husdVal.symbol = common::S_PEG;
    
    cw.insertOrReplace(*detailsGroup, Content{
      DEFERRED,
      new_deferred_perc_x100
    });

    auto rewardToPegVal = normalizeToken(daoSettings->getOrFail<eosio::asset>(common::REWARD_TO_PEG_RATIO));

    auto rewardToken = daoSettings->getOrFail<eosio::asset>(common::REWARD_TOKEN);

    auto pegToken = daoSettings->getOrFail<eosio::asset>(common::PEG_TOKEN);

    auto rewardVal = usdPerPeriodCommitmentAdjusted * deferred / rewardToPegVal;

      cw.insertOrReplace(*detailsGroup, Content{
      common::REWARD_SALARY_PER_PERIOD, 
      denormalizeToken(rewardVal, rewardToken)
    });

    cw.insertOrReplace(*detailsGroup, Content{
      common::PEG_SALARY_PER_PERIOD,
      denormalizeToken(pegVal, pegToken)
    });

    m_documentGraph.updateDocument(issuer, assignment.getID(), cw.getContentGroups());
  }

   void dao::modifyCommitment(Assignment& assignment, int64_t commitment, std::optional<eosio::time_point> fixedStartDate, std::string_view modifier)
   {
      TRACE_FUNCTION()
      
      ContentWrapper assignmentCW = assignment.getContentWrapper();

      //Should use the role edge instead of the role content item
      //in case role document is modified (causes it's hash to change)
      auto assignmentToRoleEdge = m_documentGraph.getEdgesFrom(assignment.getID(), common::ROLE_NAME);
      
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
      Edge lastTimeShareEdge = Edge::get(get_self(), assignment.getID(), common::LAST_TIME_SHARE);

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
                                assignment.getID());

      Edge::write(get_self(), get_self(), lastTimeShareEdge.getToNode(), newTimeShareDoc.getID(), common::NEXT_TIME_SHARE);

      lastTimeShareEdge.erase();

      Edge::write(get_self(), get_self(), assignment.getID(), newTimeShareDoc.getID(), common::LAST_TIME_SHARE);
   }
  
  Document dao::getMemberDoc(const name& account)
  {
    return Document(get_self(), Member::calcHash(account));
  }

  void dao::checkEnrollerAuth(uint64_t dao_id, const name& account)
  {
    TRACE_FUNCTION()

    auto memberID = getMemberDoc(account).getID();

    EOS_CHECK(
      Edge::exists(get_self(), dao_id, memberID, common::ENROLLER),
      util::to_str("Only enrollers of the dao are allowed to perform this action")
    )
  }

  void dao::checkAdminsAuth(uint64_t dao_id)
  {
    TRACE_FUNCTION()

    auto adminEdges = m_documentGraph.getEdgesFrom(dao_id, common::ADMIN);

    EOS_CHECK(
      std::any_of(adminEdges.begin(), adminEdges.end(), [this](const Edge& adminEdge) {
        Member member(*this, adminEdge.to_node);
        return eosio::has_auth(member.getAccount());
      }),
      util::to_str("Only admins of the dao are allowed to perform this action")
    )
  }

  void dao::genPeriods(uint64_t dao_id, int64_t period_count/*, int64_t period_duration_sec*/)
  {
    //Max number of periods that should be created in one call
    const int64_t MAX_PERIODS_PER_CALL = 30;

    //Get last period
    //Document daoDoc(get_self(), dao_id);

    auto settings = getSettingsDocument(dao_id);

    int64_t periodDurationSecs = settings->getOrFail<int64_t>(common::PERIOD_DURATION);

    name daoName = settings->getOrFail<eosio::name>(DAO_NAME);

    auto lastEdge = Edge::get(get_self(), dao_id, common::END);
    lastEdge.erase();

    uint64_t lastPeriodID = lastEdge.getToNode();

    int64_t lastPeriodStartSecs = Period(this, lastPeriodID)
                                    .getStartTime()
                                    .sec_since_epoch();

    for (int64_t i = 0; i < std::min(MAX_PERIODS_PER_CALL, period_count); ++i) {
      time_point nextPeriodStart(eosio::seconds(lastPeriodStartSecs + periodDurationSecs));

      Period nextPeriod(
        this, 
        nextPeriodStart, 
        util::to_str(daoName, ":", nextPeriodStart.time_since_epoch().count())
      );
      
      Edge(get_self(), get_self(), lastPeriodID, nextPeriod.getID(), common::NEXT);
      Edge(get_self(), get_self(), dao_id, nextPeriod.getID(), common::PERIOD);
      Edge(get_self(), get_self(), nextPeriod.getID(), dao_id, common::DAO);

      lastPeriodStartSecs = nextPeriodStart.sec_since_epoch();
      lastPeriodID = nextPeriod.getID();
    }

    Edge(get_self(), get_self(), dao_id, lastPeriodID, common::END);

    //Check if there are more periods to created

    if (period_count > MAX_PERIODS_PER_CALL) {
      eosio::action(
        eosio::permission_level(get_self(), "active"_n),
        get_self(),
        "genperiods"_n,
        std::make_tuple(dao_id, 
                        period_count - MAX_PERIODS_PER_CALL)
      ).send();
    }
  }

  void dao::createVoiceToken(const eosio::name& daoName,
                             const eosio::asset& voiceToken,
                             const uint64_t& decayPeriod,
                             const uint64_t& decayPerPeriodx10M)
  {
    auto dhoSettings = getSettingsDocument();
    name governanceContract = dhoSettings->getOrFail<eosio::name>(GOVERNANCE_TOKEN_CONTRACT);

    eosio::action(
          eosio::permission_level{governanceContract, name("active")},
          governanceContract,
          name("create"),
          std::make_tuple(
            daoName,
            get_self(),
            asset{-getTokenUnit(voiceToken), voiceToken.symbol},
            decayPeriod,
            decayPerPeriodx10M
          )
        ).send();
  }

  void dao::createTokens(const eosio::name& daoName,
                         const eosio::asset& rewardToken,
                         const eosio::asset& pegToken)
  {

    auto dhoSettings = getSettingsDocument();

    name rewardContract = dhoSettings->getOrFail<eosio::name>(REWARD_TOKEN_CONTRACT);

    eosio::action(
      eosio::permission_level{rewardContract, name("active")},
      rewardContract,
      name("create"),
      std::make_tuple(
        daoName,
        get_self(),
        asset{-getTokenUnit(rewardToken), rewardToken.symbol},
        uint64_t{0},
        uint64_t{0}
      )
    ).send();

    name pegContract = dhoSettings->getOrFail<eosio::name>(PEG_TOKEN_CONTRACT);

    name treasuryContract = dhoSettings->getOrFail<eosio::name>(TREASURY_CONTRACT);

    eosio::action(
      eosio::permission_level{pegContract, name("active")},
      pegContract,
      name("create"),
      std::make_tuple(
        daoName,
        treasuryContract,
        asset{-getTokenUnit(pegToken), pegToken.symbol},
        uint64_t{0},
        uint64_t{0}
      )
    ).send();
  }

} // namespace hypha
