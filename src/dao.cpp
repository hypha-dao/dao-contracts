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
   ACTION dao::clean()
   {
     require_auth(get_self());

     document_table docs(get_self(), get_self().value);

     for (auto it = docs.begin(); it != docs.end(); ) {
       it = docs.erase(it);
     }

     edge_table edges(get_self(), get_self().value);

     for (auto it = edges.begin(); it != edges.end(); ) {
       it = edges.erase(it);
     }
   }

   void dao::propose(const checksum256& dao_hash,
                     const name &proposer,
                     const name &proposal_type,
                     ContentGroups &content_groups)
   {
      TRACE_FUNCTION()
      EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

      //TODO: Change to dao_hash instead of dao_name

      std::unique_ptr<Proposal> proposal = std::unique_ptr<Proposal>(ProposalFactory::Factory(*this, dao_hash, proposal_type));
      proposal->propose(dao_hash, proposer, content_groups);
   }

   void dao::vote(const name &voter, const checksum256 &proposal_hash, string &vote, string notes)
   {
      TRACE_FUNCTION()
      EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");
      Document docprop(get_self(), proposal_hash);
      name proposal_type = docprop.getContentWrapper().getOrFail(SYSTEM, TYPE)->getAs<eosio::name>();

      auto daoHash = Edge::get(get_self(), proposal_hash, common::DAO).getToNode();

      Proposal *proposal = ProposalFactory::Factory(*this, daoHash, proposal_type);
      proposal->vote(voter, vote, docprop, notes);
   }

   void dao::closedocprop(const checksum256 &proposal_hash)
   {
      TRACE_FUNCTION()
      EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");
      
      auto daoHash = Edge::get(get_self(), proposal_hash, common::DAO).getToNode();

      Document docprop(get_self(), proposal_hash);
      name proposal_type = docprop.getContentWrapper().getOrFail(SYSTEM, TYPE)->getAs<eosio::name>();

      Proposal *proposal = ProposalFactory::Factory(*this, daoHash, proposal_type);
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
      
      auto daoHash = Edge::get(get_self(), assignment_hash, common::DAO).getToNode();

      //Get the DAO edge from the hash
      EOS_CHECK(Member::isMember(get_self(), daoHash, assignee), "assignee must be a current member to request an extension: " + assignee.to_string());

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
      std::unique_ptr<Proposal> proposal = std::unique_ptr<Proposal>(ProposalFactory::Factory(*this, daoHash, common::EXTENSION));
      proposal->propose(daoHash, assignee, contentGroups);
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

     //Get the DAO edge from the hash
     auto daoHash = Edge::get(get_self(), hash, common::DAO).getToNode();

     EOS_CHECK(
       Member::isMember(get_self(), daoHash, proposer), 
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

     //Check which dao this document belongs to
     checksum256 dao = Edge::get(get_self(), hash, common::DAO).getToNode();

     propose(dao, proposer, common::SUSPEND, cgs);
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
      auto daoHash = Edge::get(get_self(), assignment_hash, common::DAO).getToNode();
      
      EOS_CHECK(
        Member::isMember(get_self(), daoHash, assignee), 
        "assignee must be a current member to claim pay: " + assignee.to_string()
      );

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

      auto daoSettings = getSettingsDocument(daoHash);

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
            Edge::get(get_self(), assignment.getHash(), common::CURRENT_TIME_SHARE).erase();
            Edge::write(get_self(), get_self(), assignment.getHash(), lastUsedTimeShare->getHash(), common::CURRENT_TIME_SHARE);
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

      ab = applyBadgeCoefficients(periodToClaim.value(), assignee, ab);

      makePayment(periodToClaim.value().getHash(), assignee, ab.reward, memo, eosio::name{0}, daoTokens);
      makePayment(periodToClaim.value().getHash(), assignee, ab.voice, memo, eosio::name{0}, daoTokens);
      makePayment(periodToClaim.value().getHash(), assignee, ab.peg, memo, eosio::name{0}, daoTokens);
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

   AssetBatch dao::applyBadgeCoefficients(Period &period, const eosio::name &member, AssetBatch &ab)
   {
      TRACE_FUNCTION()
      // get list of badges
      auto badges = getCurrentBadges(period, member);
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

   void dao::makePayment(const eosio::checksum256 &fromNode,
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

      std::unique_ptr<Payer> payer = std::unique_ptr<Payer>(PayerFactory::Factory(*this, quantity.symbol, paymentType, daoTokens));
      Document paymentReceipt = payer->pay(recipient, quantity, memo);
      Edge::write(get_self(), get_self(), fromNode, paymentReceipt.getHash(), common::PAYMENT);
      Edge::write(get_self(), get_self(), Member::calcHash(recipient), paymentReceipt.getHash(), common::PAID);
   }

   void dao::apply(const eosio::name &applicant, const name& dao_name, const std::string &content)
   {
      TRACE_FUNCTION()
      require_auth(applicant);
      Member member(*this, applicant, applicant);
      member.apply(getDAO(dao_name), content);
   }

   void dao::enroll(const eosio::name &enroller, const name& dao_name, const eosio::name &applicant, const std::string &content)
   {
      TRACE_FUNCTION()

      //Verify enroller is valid for the given dao
      const name onboarderAcc = getSettingOrFail<name>(dao_name, common::ONBOARDER_ACCOUNT);

      EOS_CHECK(
        enroller == onboarderAcc,
        util::to_str("Only ", onboarderAcc, " is allowed to enroll users for ", dao_name)
      )

      require_auth(enroller);

      Member member = Member::get(*this, applicant);
      member.enroll(enroller, getDAO(dao_name), content);
   }

   bool dao::isPaused() { return false; }

   Settings* dao::getSettingsDocument(const eosio::name &dao_name)
   {
      TRACE_FUNCTION()
      return getSettingsDocument(getDAO(dao_name));
   }

   Settings* dao::getSettingsDocument(const eosio::checksum256& daoHash)
   {
     TRACE_FUNCTION();

     //Check if it'S already loaded in cache
     for (auto& settingsDoc : m_settingsDocs) {
       if (settingsDoc->getRootHash() == daoHash) {
         return settingsDoc.get();
       }
     }

     //If not then we have to load it
     auto edges = m_documentGraph.getEdgesFromOrFail(daoHash, common::SETTINGS_EDGE);
     EOS_CHECK(edges.size() == 1, "There should only exists only 1 settings edge from a dao node");
    
     m_settingsDocs.emplace_back(std::make_unique<Settings>(
       *this,
       edges[0].to_node,
       daoHash
     ));

     return m_settingsDocs.back().get();
   }

   Settings* dao::getSettingsDocument()
   {
      TRACE_FUNCTION()
      return getSettingsDocument(getRoot(get_self()));
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
      auto settings = getSettingsDocument();
      settings->setSetting(Content{key, value});
   }

   void dao::setdaosetting(const eosio::checksum256& dao_hash, const string &key, const Content::FlexValue &value)
   {
      TRACE_FUNCTION()
      auto settings = getSettingsDocument(dao_hash);
      auto onboarder = settings->getOrFail<name>(common::ONBOARDER_ACCOUNT);
      require_auth(onboarder);

      setSetting(dao_hash, key, value);
   }

   void dao::setSetting(const eosio::checksum256& dao_hash, const string &key, const Content::FlexValue &value)
   {
      TRACE_FUNCTION()
      auto settings = getSettingsDocument(dao_hash);
      settings->setSetting(Content{key, value});
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
      
      auto settings = getSettingsDocument();
      settings->remSetting(key);
   }


   ACTION dao::genperiods(const eosio::checksum256& dao_hash, int64_t period_count/*, int64_t period_duration_sec*/)
   {
      TRACE_FUNCTION()

      auto settings = getSettingsDocument(dao_hash);

      auto onboarder = settings->getOrFail<eosio::name>(common::ONBOARDER_ACCOUNT);

      require_auth(onboarder);

      genPeriods(dao_hash, period_count);
   }

   void dao::addperiod(const eosio::checksum256 &predecessor, const eosio::time_point &start_time, const string &label)
   {
      TRACE_FUNCTION()
      require_auth(get_self());

      addPeriod(predecessor, start_time, label);
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

      Edge(get_self(), get_self(), root.getHash(), daoDoc.getHash(), common::DAO);
      
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
              *votingAllignment 
          },
          ContentGroup{
              Content(CONTENT_GROUP_LABEL, SYSTEM),
              Content(TYPE, common::SETTINGS_EDGE),
              Content(NODE_LABEL, "Settings")
          }
      };

      Document settingsDoc(get_self(), get_self(), std::move(settingCgs));
      Edge::write(get_self(), get_self(), daoDoc.getHash(), settingsDoc.getHash(), common::SETTINGS_EDGE);

      createTokens(voiceToken->getAs<asset>(), rewardToken->getAs<asset>(), pegToken->getAs<asset>());
      
      //Auto enroll      
      std::unique_ptr<Member> member;
      
      const checksum256 memberHash = Member::calcHash(onboarder);

      if (Document::exists(get_self(), memberHash)) {
        member = std::make_unique<Member>(*this, memberHash);
      }
      else {
        member = std::make_unique<Member>(*this, onboarder, onboarder);
      }

      member->apply(daoDoc.getHash(), "DAO Onboarder");
      member->enroll(onboarder, daoDoc.getHash(), "DAO Onboarder");
      
      //TODO: Create enroller edge
      
      //Create start period
      addPeriod(
        daoDoc.getHash(), 
        eosio::current_time_point(), 
        util::to_str(dao, " - start period")
      );

      auto startEdge = Edge::get(get_self(), daoDoc.getHash(), common::START);

      //Create end period edge
      Edge(get_self(), get_self(), daoDoc.getHash(), startEdge.getToNode(), common::END);

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
      Edge::write(get_self(), get_self(), rootDoc.getHash(), settingsDoc.getHash(), common::SETTINGS_EDGE);
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

    auto daoSettings = getSettingsDocument(assignment.getDaoHash());

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

  void dao::addPeriod(const eosio::checksum256 &predecessor, const eosio::time_point &start_time, const string &label) 
  {
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

  void dao::genPeriods(const checksum256& dao_hash, int64_t period_count/*, int64_t period_duration_sec*/)
  {
    //Max number of periods that should be created in one call
    const int64_t MAX_PERIODS_PER_CALL = 30;

    //Get last period
    Document daoDoc(get_self(), dao_hash);

    auto settings = getSettingsDocument(dao_hash);

    int64_t periodDurationSecs = settings->getOrFail<int64_t>(common::PERIOD_DURATION);

    name daoName = settings->getOrFail<eosio::name>(DAO_NAME);

    auto lastEdge = Edge::get(get_self(), daoDoc.getHash(), common::END);
    lastEdge.erase();

    checksum256 lastPeriodHash = lastEdge.getToNode();

    int64_t lastPeriodStartSecs = Period(this, lastPeriodHash)
                                    .getStartTime()
                                    .sec_since_epoch();

    for (int64_t i = 0; i < period_count; ++i) {
      time_point nextPeriodStart(eosio::seconds(lastPeriodStartSecs + periodDurationSecs));

      Period nextPeriod(
        this, 
        nextPeriodStart, 
        util::to_str(daoName, ":", nextPeriodStart.time_since_epoch().count())
      );
      
      Edge(get_self(), get_self(), lastPeriodHash, nextPeriod.getHash(), common::NEXT);

      lastPeriodStartSecs = nextPeriodStart.sec_since_epoch();
      lastPeriodHash = nextPeriod.getHash();
    }

    Edge(get_self(), get_self(), daoDoc.getHash(), lastPeriodHash, common::END);

    //Check if there are more periods to created

    if (period_count > MAX_PERIODS_PER_CALL) {
      eosio::action(
        eosio::permission_level(get_self(), "active"_n),
        get_self(),
        "genperiods"_n,
        std::make_tuple(dao_hash, 
                        period_count - MAX_PERIODS_PER_CALL)
      ).send();
    }
  }

  void dao::createTokens(const eosio::asset& voiceToken, 
                        const eosio::asset& rewardToken,
                        const eosio::asset& pegToken)
  {
    
    auto dhoSettings = getSettingsDocument();

    name governanceContract = dhoSettings->getOrFail<eosio::name>(GOVERNANCE_TOKEN_CONTRACT);

    eosio::action(
      eosio::permission_level{governanceContract, name("active")},
      governanceContract, 
      name("create"),
      std::make_tuple(
        get_self(), 
        asset{-getTokenUnit(voiceToken), voiceToken.symbol}, 
        uint64_t{0}, 
        uint64_t{0}
      )
    ).send();

    name rewardContract = dhoSettings->getOrFail<eosio::name>(REWARD_TOKEN_CONTRACT);

    eosio::action(
      eosio::permission_level{rewardContract, name("active")},
      rewardContract,
      name("create"),
      std::make_tuple(
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
        treasuryContract,
        asset{-getTokenUnit(pegToken), pegToken.symbol},
        uint64_t{0},
        uint64_t{0}
      )
    ).send();
  }

} // namespace hypha
