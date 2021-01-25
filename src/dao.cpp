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
#include <migration.hpp>
#include <period.hpp>
#include <assignment.hpp>
#include <time_share.hpp>

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

      require_auth(assignee);

      // Valid claim identified - start process
      // process this claim
      Edge::write(get_self(), get_self(), assignment.getHash(), periodToClaim.value().getHash(), common::CLAIMED);
      int64_t assignmentApprovedDateSec = assignment.getApprovedTime().sec_since_epoch();
      int64_t periodStartSec = periodToClaim.value().getStartTime().sec_since_epoch();
      int64_t periodEndSec = periodToClaim.value().getEndTime().sec_since_epoch();

      // Pro-rate the payment if the assignment was created during the period being claimed
      float first_phase_ratio_calc = 1.f; // pro-rate based on elapsed % of the first phase

      /** This is no longer necesary since addition of TimeShare
      if (assignmentApprovedDateSec > periodStartSec)
      {
          int64_t elapsed_sec = periodEndSec - assignmentApprovedDateSec;
          int64_t period_sec = periodEndSec - periodStartSec;
          first_phase_ratio_calc = (float)elapsed_sec / (float)period_sec;
      }
      */ 
      eosio::check(first_phase_ratio_calc <= 1, "fatal error: first_phase_ratio_calc is greater than 1: " + std::to_string(first_phase_ratio_calc));

      asset deferredSeeds;
      asset husd;
      asset hvoice;
      asset hypha;

      {
        const int64_t initTimeShare = assignment.getInitialTimeShare()
                                                .getContentWrapper()
                                                .getOrFail(DETAILS, TIME_SHARE)->getAs<int64_t>();

        TimeShare current = assignment.getCurrentTimeShare();
        
        //Initialize nextOpt with current in order to have a valid initial timeShare
        auto nextOpt = std::optional<TimeShare>{current};

        int64_t currentTimeSec = periodStartSec;

        std::optional<TimeShare> lastUsedTimeShare;

        while (nextOpt) {

          ContentWrapper nextWrapper = nextOpt->getContentWrapper();

          const int64_t timeShare = nextWrapper.getOrFail(DETAILS, TIME_SHARE)->getAs<int64_t>();
          const int64_t startDateSec = nextWrapper.getOrFail(DETAILS, TIME_SHARE_START_DATE)->getAs<time_point>().sec_since_epoch();

          //If the time share doesn't belong to the claim period we 
          //finish pro-rating
          if (periodEndSec - startDateSec <= 0) {
              break;
          }

          int64_t remainingTimeSec;

          lastUsedTimeShare = std::move(nextOpt.value());

          //It's possible that time share was set on previous periods,
          //if so we should use period start date as the base date
          const int64_t baseDateSec = std::max(periodStartSec, startDateSec);

          //Check if there is another timeshare
          if ((nextOpt = lastUsedTimeShare->getNext(get_self()))) {
            const time_point commingStartDate = nextOpt->getContentWrapper().getOrFail(DETAILS, TIME_SHARE_START_DATE)->getAs<time_point>();
            const int64_t commingStartDateSec = commingStartDate.sec_since_epoch();
            //Check if the time share belongs to the same peroid
            if (commingStartDateSec >= periodEndSec) {
              remainingTimeSec = periodEndSec - baseDateSec;
            }
            else {
              remainingTimeSec = commingStartDateSec - baseDateSec;
            }
          }
          else {
            remainingTimeSec = periodEndSec - baseDateSec;
          }

          const int64_t fullPeriodSec = periodEndSec - periodStartSec;
          
          //Time share could only represent a portion of the whole period
          float relativeDuration = static_cast<float>(remainingTimeSec) / static_cast<float>(fullPeriodSec);
          float relativeCommitment = static_cast<float>(timeShare) / static_cast<float>(initTimeShare);
          float commitmentMultiplier = relativeDuration * relativeCommitment;
          
          //Accumlate each of the currencies with the time share multiplier
          deferredSeeds = (deferredSeeds.is_valid() ? deferredSeeds : eosio::asset{0, common::S_SEEDS}) + 
          adjustAsset(assignment.getSalaryAmount(&common::S_SEEDS, &periodToClaim.value()), first_phase_ratio_calc * commitmentMultiplier);

          // These values are calculated when the assignment is proposed, so simply pro-rate them if/as needed
          // If there is an explicit INSTANT SEEDS amount, support sending it
          husd = (husd.is_valid() ? husd : eosio::asset{0, common::S_HUSD}) + 
          adjustAsset(assignment.getSalaryAmount(&common::S_HUSD), first_phase_ratio_calc * commitmentMultiplier);

          hvoice = (hvoice.is_valid() ? hvoice : eosio::asset{0, common::S_HVOICE}) + 
          adjustAsset(assignment.getSalaryAmount(&common::S_HVOICE), first_phase_ratio_calc * commitmentMultiplier);
          
          hypha = (hypha.is_valid() ? hypha : eosio::asset{0, common::S_HYPHA}) + 
          adjustAsset(assignment.getSalaryAmount(&common::S_HYPHA), first_phase_ratio_calc * commitmentMultiplier);
        }

        //If the last used time share is different from current time share 
        //let's update the edge
        if (lastUsedTimeShare->getHash() != current.getHash()) {
          Edge::get(get_self(), assignment.getHash(), common::CURRENT_TIME_SHARE).erase();
          Edge::write(get_self(), get_self(), assignment.getHash(), lastUsedTimeShare->getHash(), common::CURRENT_TIME_SHARE);
        }
      }

      eosio::check(deferredSeeds.is_valid(), "fatal error: SEEDS has to be a valid asset");
      eosio::check(husd.is_valid(), "fatal error: HUSD has to be a valid asset");
      eosio::check(hvoice.is_valid(), "fatal error: HVOICE has to be a valid asset");
      eosio::check(hypha.is_valid(), "fatal error: HYPHA has to be a valid asset");

      string memo = "Payment for assignment " + readableHash(assignment.getHash()) + "; Period: " + readableHash(periodToClaim.value().getHash());

      // creating a single struct improves performance for table queries here
      AssetBatch ab{};
      ab.d_seeds = deferredSeeds;
      ab.hypha = hypha;
      ab.voice = hvoice;
      ab.husd = husd;

      ab = applyBadgeCoefficients(periodToClaim.value(), assignee, ab);

      makePayment(periodToClaim.value().getHash(), assignee, ab.hypha, memo, eosio::name{0});
      makePayment(periodToClaim.value().getHash(), assignee, ab.d_seeds, memo, common::ESCROW);
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
      // eosio::print ("getting badges    : " + std::to_string(badge_assignment_edges.size()) + "\n");
      for (Edge e : badge_assignment_edges)
      {
         Document badgeAssignmentDoc(get_self(), e.getToNode());
         auto badgeAssignment = badgeAssignmentDoc.getContentWrapper();
         Edge badge_edge = Edge::get(get_self(), badgeAssignmentDoc.getHash(), common::BADGE_NAME);
         Document badge(get_self(), badge_edge.getToNode());
         current_badges.push_back(badge);

         // fast forward to duration end (TODO: probably need a duration class)
         // Period startPeriod(this, badgeAssignment.getOrFail(DETAILS, START_PERIOD)->getAs<eosio::checksum256>());
         // eosio::print (" badge assignment start period: " + readableHash(startPeriod.getHash()) + "\n");

         // int64_t periodCount = badgeAssignment.getOrFail(DETAILS, PERIOD_COUNT)->getAs<int64_t>();
         // int64_t counter = 0;
         // std::optional<Period> seeker = std::optional<Period>{startPeriod};
         // eosio::print (" periodCount   : " + std::to_string(periodCount) + "\n");
         // while (seeker.has_value() && counter <= periodCount)
         // {
         //    eosio::print (" counter : " + std::to_string(counter) + "\n");
         //    seeker = seeker.value().next();
         //    counter++;
         // }

         // eosio::print (" checking expiration time of last period ");
         // std::optional<eosio::time_point> periodEndTime = seeker.value().getEndTime();
         // eosio::print (" got end time ");

         // eosio::check(periodEndTime != std::nullopt, "End of calendar has been reached. Contact administrator to add more time periods.");

         // int64_t badgeAssignmentExpiration = periodEndTime.value().sec_since_epoch();
         // eosio::print (" badge assignment expiration: " + std::to_string(badgeAssignmentExpiration) + "\n");

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
         // if (std::holds_alternative<std::monostate>(coefficient->value))
         // {
         //    return asset{0, base.symbol};
         // }

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
         applied_assets.seeds = applied_assets.seeds + applyCoefficient(badgeCW, ab.seeds, SEEDS_COEFFICIENT);
         applied_assets.d_seeds = applied_assets.d_seeds + applyCoefficient(badgeCW, ab.d_seeds, SEEDS_COEFFICIENT);
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

   void dao::cancel (const uint64_t senderid)
   {
      require_auth(get_self());
      eosio::cancel_deferred (senderid);
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

   void dao::addasspayout(const uint64_t &ass_payment_id,
                          const uint64_t &assignment_id,
                          const name &recipient,
                          uint64_t period_id,
                          std::vector<eosio::asset> payments,
                          eosio::time_point payment_date)
   {
      eosio::require_auth(get_self());

      Migration migration(this);
      migration.addLegacyAssPayout(ass_payment_id, assignment_id, recipient, period_id, payments, payment_date);
   }

   void dao::migrateper(const uint64_t id)
   {
      eosio::require_auth(get_self());

      Migration migration(this);
      migration.migratePeriod(id);
   }

   void dao::migasspay(const uint64_t id)
   {
      eosio::require_auth(get_self());

      Migration migration(this);
      migration.migrateAssPayout(id);
   }

   void dao::addlegper(const uint64_t &id,
                       const time_point &start_date,
                       const time_point &end_date,
                       const string &phase,
                       const string &readable,
                       const string &label)
   {
      eosio::require_auth(get_self());

      Migration migration(this);
      migration.addLegacyPeriod(id, start_date, end_date, phase, readable, label);
   }

   void dao::addmember(const eosio::name &member)
   {
      eosio::require_auth(get_self());

      Migration migration(this);
      migration.addLegacyMember(member);
   }

   void dao::migratemem(const eosio::name &member)
   {
      eosio::require_auth(get_self());

      Migration migration(this);
      migration.migrateMember(member);
   }

   void dao::addapplicant(const eosio::name &applicant, const std::string content)
   {
      eosio::require_auth(get_self());

      Migration migration(this);
      migration.addLegacyApplicant(applicant, content);
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

   void dao::migrate(const eosio::name &scope, const uint64_t &id)
   {
      require_auth(get_self());

      if (scope == common::ROLE_NAME)
      {
         Migration migration(this);
         migration.migrateRole(id);
      }
      else if (scope == common::ASSIGNMENT)
      {
         Migration migration(this);
         migration.migrateAssignment(id);
      }
      else if (scope == common::PAYOUT)
      {
         Migration migration(this);
         migration.migratePayout(id);
      }
   }

   void dao::migrateconfig(const std::string &notes)
   {
      require_auth(get_self());
      Migration migration(this);
      migration.migrateConfig();
   }

   void dao::reset4test(const std::string &notes)
   {
      require_auth(get_self());
      Migration migration(this);
      migration.reset4test();
   }

   void dao::erasepers(const std::string &notes)
   {
      require_auth(get_self());
      Migration migration(this);
      migration.erasePeriods();
   }

   void dao::erasegraph(const std::string &notes)
   {
      require_auth(get_self());
      Migration migration(this);
      migration.eraseGraph();
   }

   // void dao::eraseobjs(const eosio::name &scope)
   // {
   //    require_auth(get_self());
   //    Migration migration(this);
   //    migration.eraseAllObjects(scope);
   // }

   void dao::eraseobj(const eosio::name &scope, const uint64_t &starting_id)
   {
      require_auth(get_self());
      Migration::object_table o_t(get_self(), scope.value);
      auto o_itr = o_t.find(starting_id);
      eosio::check(o_itr != o_t.end(), "id not found");
      o_t.erase(o_itr);
   }

   void dao::eraseobjs(const eosio::name &scope, const uint64_t &starting_id, const uint64_t &batch_size, int senderId)
   {
      int counter = 0;
      
      Migration::object_table o_t(get_self(), scope.value);
      auto o_itr = o_t.find(starting_id);
      while (o_itr != o_t.end() && counter < batch_size)
      {
         o_itr = o_t.erase(o_itr);
         counter++;
      }

      if (o_itr != o_t.end()) {
         eosio::transaction out{};
         out.actions.emplace_back(
            eosio::permission_level{get_self(), name("active")},
            get_self(), name("eraseobjs"),
            std::make_tuple(scope, starting_id + batch_size, batch_size, ++senderId));

         out.delay_sec = 1;
         out.send(senderId, get_self());
      }      
   }

   void dao::erasexfer(const eosio::name &scope)
   {
      require_auth(get_self());
      Migration migration(this);
      migration.eraseXRefs(scope);
   }

   void dao::erasedoc(const checksum256 &hash)
   {
      require_auth(get_self());

      DocumentGraph dg(get_self());
      dg.eraseDocument(hash);
   }

   void dao::setalert(const eosio::name &level, const std::string &content)
   {
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

   void dao::createobj(const uint64_t &id,
                       const name &scope,
                       std::map<string, name> names,
                       std::map<string, string> strings,
                       std::map<string, asset> assets,
                       std::map<string, eosio::time_point> time_points,
                       std::map<string, uint64_t> ints,
                       eosio::time_point created_date,
                       eosio::time_point updated_date)
   {
      Migration migration(this);
      migration.addLegacyObject(id, scope, names, strings, assets, time_points, ints, created_date, updated_date);
   }


  /**
  * Info Structure
  * 
  * ContentGroups
  * [
  *   Group Assignment 0 Details: [
  *     assignemnt_id: [checksum256]
  *     new_time_share_x100: [int64_t] min_time_share_x100 <= new_time_share_x100 <= time_share_x100
  *   ],
  *   Group Assignment 1 Details: [
  *     assignemnt_id: [checksum256]
  *     new_time_share_x100: [int64_t] min_time_share_x100 <= new_time_share_x100 <= time_share_x100
  *   ],
  *   Group Assignment N Details: [
  *     assignemnt_id: [checksum256]
  *     new_time_share_x100: [int64_t] min_time_share_x100 <= new_time_share_x100 <= time_share_x100
  *   ]
  * ]
  */
  ACTION dao::adjustcmtmnt(name issuer, ContentGroups& adjust_info) 
  {
    //TODO: Test utility function to_str
    //eosio::check(false, to_str("Test: ", 22, name(" abc "), 3.4, NEW_TIME_SHARE));
    require_auth(issuer);

    ContentWrapper cw(adjust_info);

    for (size_t i = 0; i < adjust_info.size(); ++i) {

      Assignment assignment = Assignment(this, 
                                         cw.getOrFail(i, "assignemnt_id").second->getAs<checksum256>());

      eosio::check(assignment.getAssignee().getAccount() == issuer, 
                   "Only the owner of the assignment can adjust it");

      ContentWrapper assignmentCW = assignment.getContentWrapper();

      Document roleDocument(get_self(), assignmentCW.getOrFail(DETAILS, ROLE_STRING)->getAs<eosio::checksum256>());
      auto role = roleDocument.getContentWrapper();

      //Check min_time_share_x100 <= new_time_share_x100 <= time_share_x100
      int64_t originalTimeShare = assignmentCW.getOrFail(DETAILS, TIME_SHARE)->getAs<int64_t>();
      int64_t minTimeShare = role.getOrFail(DETAILS, MIN_TIME_SHARE)->getAs<int64_t>();
      int64_t newTimeShare = cw.getOrFail(i, NEW_TIME_SHARE).second->getAs<int64_t>();

      eosio::check(newTimeShare >= minTimeShare, 
                    NEW_TIME_SHARE + string(" must be greater than or equal to: ") + std::to_string(minTimeShare) 
                                   + string(" You submitted: ") + std::to_string(newTimeShare));
      eosio::check(newTimeShare <= originalTimeShare, 
                    NEW_TIME_SHARE + string(" must be less than or equal to original time_share_x100: ") + std::to_string(originalTimeShare)
                                   + string(" You submitted: ") + std::to_string(newTimeShare));

      //Update lasttimeshare
      Edge lastTimeShareEdge = Edge::get(get_self(), assignment.getHash(), common::LAST_TIME_SHARE);
    
      time_point startDate = eosio::current_time_point();

      if (auto [idx, startDateContent] = cw.get(i, TIME_SHARE_START_DATE); 
          startDateContent) {
        startDate = startDateContent->getAs<time_point>();
      }

      TimeShare newTimeShareDoc(get_self(), issuer, newTimeShare, startDate);

      Edge::write(get_self(), get_self(), lastTimeShareEdge.getToNode(), newTimeShareDoc.getHash(), common::NEXT_TIME_SHARE);

      lastTimeShareEdge.erase();

      Edge::write(get_self(), get_self(), assignment.getHash(), newTimeShareDoc.getHash(), common::LAST_TIME_SHARE);
    }
  }

} // namespace hypha