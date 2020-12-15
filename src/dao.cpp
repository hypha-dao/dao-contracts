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

   void dao::claimpay(const eosio::checksum256 &hash)
   {
      eosio::check(!isPaused(), "Contract is paused for maintenance. Please try again later.");

      Document assignmentDocument(get_self(), hash);
      ContentWrapper assignment = assignmentDocument.getContentWrapper();

      eosio::check(assignment.getOrFail(SYSTEM, TYPE)->getAs<eosio::name>() == common::ASSIGNMENT,
                   "claim pay must be made on an assignment document: " + readableHash(hash));

      // assignee must still be a DHO member
      name assignee = assignment.getOrFail(DETAILS, ASSIGNEE)->getAs<eosio::name>();
      eosio::check(Member::isMember(get_self(), assignee), "assignee must be a current member to claim pay: " + assignee.to_string());
      require_auth(assignee);

      // Check that pay period is between (inclusive) the start and end period of the assignment
      Period startPeriod(this, assignment.getOrFail(DETAILS, START_PERIOD)->getAs<eosio::checksum256>());
      int64_t periodCount = assignment.getOrFail(DETAILS, PERIOD_COUNT)->getAs<int64_t>();
      int64_t claimedPeriods = 0;
      std::optional<Period> seeker = std::optional<Period>{startPeriod};
      while (seeker.has_value() && claimedPeriods < periodCount)
      {
         if (Edge::exists(get_self(), assignmentDocument.getHash(), startPeriod.getHash(), common::CLAIMED))
         {
            claimedPeriods++;
         }
         else
         {
            std::optional<eosio::time_point> periodEndTime = seeker.value().getEndTime();
            eosio::check(periodEndTime != std::nullopt, "End of calendar has been reached. Contact administrator to add more time periods.");

            // fastforward the seeker, ensure we are past the beginning of the next period
            if (periodEndTime.value().sec_since_epoch() < eosio::current_time_point().sec_since_epoch())
            {
               // process this claim
               Edge::write(get_self(), get_self(), assignmentDocument.getHash(), seeker->getHash(), common::CLAIMED);

               // Pro-rate the payment if the assignment was created during the period being claimed
               float first_phase_ratio_calc = 1; // pro-rate based on elapsed % of the first phase
               if (assignmentDocument.getCreated().sec_since_epoch() > seeker->getStartTime().sec_since_epoch())
               {
                  auto elapsed_sec = periodEndTime.value().sec_since_epoch() - assignmentDocument.getCreated().sec_since_epoch();
                  auto period_sec = periodEndTime.value().sec_since_epoch() - seeker->getStartTime().sec_since_epoch();
                  first_phase_ratio_calc = (float)elapsed_sec / (float)period_sec;
               }

               /******   DEFERRED SEEDS     */
               // If there is an explicit ESCROW SEEDS salary amount, support sending it; else it should be calculated
               eosio::asset deferredSeeds = asset{0, common::S_SEEDS};
               if (auto [idx, seedsEscrowSalary] = assignment.get(DETAILS, "seeds_escrow_salary_per_phase"); seedsEscrowSalary)
               {
                  eosio::check(std::holds_alternative<eosio::asset>(seedsEscrowSalary->value), "fatal error: expected token type must be an asset value type: " + seedsEscrowSalary->label);
                  deferredSeeds = std::get<eosio::asset>(seedsEscrowSalary->value);
               }
               else if (auto [idx, usdSalaryValue] = assignment.get(DETAILS, USD_SALARY_PER_PERIOD); usdSalaryValue)
               {
                  eosio::check(std::holds_alternative<eosio::asset>(usdSalaryValue->value), "fatal error: expected token type must be an asset value type: " + usdSalaryValue->label);

                  // Dynamically calculate the SEEDS amount based on the price at the end of the period being claimed
                  deferredSeeds = getSeedsAmount(usdSalaryValue->getAs<eosio::asset>(),
                                                 periodEndTime.value(),
                                                 (float)(assignment.getOrFail(DETAILS, TIME_SHARE)->getAs<int64_t>() / (float)100),
                                                 (float)(assignment.getOrFail(DETAILS, DEFERRED)->getAs<int64_t>() / (float)100));
               }
               deferredSeeds = adjustAsset(deferredSeeds, first_phase_ratio_calc);

               // These values are calculated when the assignment is proposed, so simply pro-rate them if/as needed
               // If there is an explicit INSTANT SEEDS amount, support sending it
               asset instantSeeds = getProRatedAsset(&assignment, common::S_SEEDS, "seeds_instant_salary_per_phase", first_phase_ratio_calc);
               asset husd = getProRatedAsset(&assignment, common::S_HUSD, HUSD_SALARY_PER_PERIOD, first_phase_ratio_calc);
               asset hvoice = getProRatedAsset(&assignment, common::S_HVOICE, HVOICE_SALARY_PER_PERIOD, first_phase_ratio_calc);
               asset hypha = getProRatedAsset(&assignment, common::S_HYPHA, HYPHA_SALARY_PER_PERIOD, first_phase_ratio_calc);

               string memo = "Payment for assignment " + readableHash(assignmentDocument.getHash()) + "; Period: " + readableHash(seeker->getHash());

               // creating a single struct improves performance for table queries here
               AssetBatch ab{};
               ab.d_seeds = deferredSeeds;
               ab.hypha = hypha;
               ab.seeds = instantSeeds;
               ab.voice = hvoice;
               ab.husd = husd;

               ab = applyBadgeCoefficients(*seeker, assignee, ab);

               makePayment(seeker->getHash(), assignee, ab.hypha, memo, eosio::name{0});
               makePayment(seeker->getHash(), assignee, ab.d_seeds, memo, common::ESCROW);
               makePayment(seeker->getHash(), assignee, ab.seeds, memo, eosio::name{0});
               makePayment(seeker->getHash(), assignee, ab.voice, memo, eosio::name{0});
               makePayment(seeker->getHash(), assignee, ab.husd, memo, eosio::name{0});
            }
         }
         seeker = seeker.value().next();
      }

      eosio::check(claimedPeriods <= periodCount, "Maximum number of periods have been claimed on this assignment; period count: " + std::to_string(periodCount));
   }

   void dao::claimnextper(const eosio::checksum256 &assignment_hash)
   {
      eosio::print (" claiming next period on assignment: " + readableHash(assignment_hash) + "\n");
      
      eosio::check(!isPaused(), "Contract is paused for maintenance. Please try again later.");

      Document assignmentDocument(get_self(), assignment_hash);
      ContentWrapper assignment = assignmentDocument.getContentWrapper();

      eosio::check(assignment.getOrFail(SYSTEM, TYPE)->getAs<eosio::name>() == common::ASSIGNMENT,
                   "claim pay must be made on an assignment document: " + readableHash(assignment_hash));

      // assignee must still be a DHO member
      name assignee = assignment.getOrFail(DETAILS, ASSIGNEE)->getAs<eosio::name>();
      eosio::check(Member::isMember(get_self(), assignee), "assignee must be a current member to claim pay: " + assignee.to_string());
      require_auth(assignee);

      std::optional<eosio::time_point> periodEndTime = std::nullopt;

      int64_t assignmentCreatedSec = assignmentDocument.getCreated().sec_since_epoch();

      int64_t now = eosio::current_time_point().sec_since_epoch();

      eosio::print (" assignmentCreatedSec   : " + std::to_string(assignmentCreatedSec) + "\n");
      eosio::print (" now                    : " + std::to_string(now) + "\n");
      
      // Ensure that the claimed period is within the approved period count
      Period startPeriod(this, assignment.getOrFail(DETAILS, START_PERIOD)->getAs<eosio::checksum256>());
      int64_t periodCount = assignment.getOrFail(DETAILS, PERIOD_COUNT)->getAs<int64_t>();
      int64_t counter = 0;
      bool found = false;
      std::optional<Period> seeker = std::optional<Period>{startPeriod};
      while (!found && seeker.has_value() && counter < periodCount)
      {  
         eosio::print("    checking period      : " + readableHash(seeker.value().getHash()) + "\n");
         periodEndTime = seeker.value().getEndTime();
         eosio::check(periodEndTime != std::nullopt, "End of calendar has been reached. Contact administrator to add more time periods.");

         eosio::print ("   periodEndTime        : " + std::to_string(periodEndTime.value().sec_since_epoch()) + "\n");

         if (  assignmentDocument.getCreated().sec_since_epoch() < periodEndTime.value().sec_since_epoch() &&         // assignment created before end of period
               periodEndTime.value().sec_since_epoch() < eosio::current_time_point().sec_since_epoch() &&             // period has lapsed
               !Edge::exists(get_self(), assignmentDocument.getHash(), seeker.value().getHash(), common::CLAIMED))    // period not yet claimed
         {
            found = true;
            break;
         }
         counter++;
         seeker = seeker.value().next();
      }
      eosio::check(found, "All periods for this assignment have been claimed: " + readableHash(assignment_hash));


      // Valid claim identified - start process
      // process this claim
      Edge::write(get_self(), get_self(), assignmentDocument.getHash(), seeker.value().getHash(), common::CLAIMED);

      // Pro-rate the payment if the assignment was created during the period being claimed
      float first_phase_ratio_calc = 1; // pro-rate based on elapsed % of the first phase

      int64_t periodStartSec = seeker.value().getStartTime().sec_since_epoch();
      int64_t periodEndSec = periodEndTime.value().sec_since_epoch();

      if (assignmentCreatedSec > periodStartSec)
      {
         int64_t elapsed_sec = periodEndSec - assignmentCreatedSec;
         int64_t period_sec = periodEndSec - periodStartSec;

         eosio::print (" elapsed_sec  : " + std::to_string(elapsed_sec) + "\n");
         eosio::print (" period_sec  : " + std::to_string(period_sec) + "\n");

         first_phase_ratio_calc = (float)elapsed_sec / (float)period_sec;
      }
      eosio::check (first_phase_ratio_calc <= 1, "fatal error: first_phase_ratio_calc is greater than 1: " + std::to_string(first_phase_ratio_calc));

      /******   DEFERRED SEEDS     */
      // If there is an explicit ESCROW SEEDS salary amount, support sending it; else it should be calculated
      eosio::asset deferredSeeds = asset{0, common::S_SEEDS};
      if (auto [idx, seedsEscrowSalary] = assignment.get(DETAILS, "seeds_escrow_salary_per_phase"); seedsEscrowSalary)
      {
         eosio::check(std::holds_alternative<eosio::asset>(seedsEscrowSalary->value), "fatal error: expected token type must be an asset value type: " + seedsEscrowSalary->label);
         deferredSeeds = std::get<eosio::asset>(seedsEscrowSalary->value);
      }
      else if (auto [idx, usdSalaryValue] = assignment.get(DETAILS, USD_SALARY_PER_PERIOD); usdSalaryValue)
      {
         eosio::check(std::holds_alternative<eosio::asset>(usdSalaryValue->value), "fatal error: expected token type must be an asset value type: " + usdSalaryValue->label);

         // Dynamically calculate the SEEDS amount based on the price at the end of the period being claimed
         deferredSeeds = getSeedsAmount(usdSalaryValue->getAs<eosio::asset>(),
                                        periodEndTime.value(),
                                        (float)(assignment.getOrFail(DETAILS, TIME_SHARE)->getAs<int64_t>() / (float)100),
                                        (float)(assignment.getOrFail(DETAILS, DEFERRED)->getAs<int64_t>() / (float)100));
      }
      deferredSeeds = adjustAsset(deferredSeeds, first_phase_ratio_calc);

      // These values are calculated when the assignment is proposed, so simply pro-rate them if/as needed
      // If there is an explicit INSTANT SEEDS amount, support sending it
      asset instantSeeds = getProRatedAsset(&assignment, common::S_SEEDS, "seeds_instant_salary_per_phase", first_phase_ratio_calc);
      asset husd = getProRatedAsset(&assignment, common::S_HUSD, HUSD_SALARY_PER_PERIOD, first_phase_ratio_calc);
      asset hvoice = getProRatedAsset(&assignment, common::S_HVOICE, HVOICE_SALARY_PER_PERIOD, first_phase_ratio_calc);
      asset hypha = getProRatedAsset(&assignment, common::S_HYPHA, HYPHA_SALARY_PER_PERIOD, first_phase_ratio_calc);

      string memo = "Payment for assignment " + readableHash(assignmentDocument.getHash()) + "; Period: " + readableHash(seeker->getHash());

      // creating a single struct improves performance for table queries here
      AssetBatch ab{};
      ab.d_seeds = deferredSeeds;
      ab.hypha = hypha;
      ab.seeds = instantSeeds;
      ab.voice = hvoice;
      ab.husd = husd;

      eosio::print (" initial hypha amount: " + hypha.to_string() + "\n");
      ab = applyBadgeCoefficients(seeker.value(), assignee, ab);

      makePayment(seeker.value().getHash(), assignee, ab.hypha, memo, eosio::name{0});
      makePayment(seeker.value().getHash(), assignee, ab.d_seeds, memo, common::ESCROW);
      makePayment(seeker.value().getHash(), assignee, ab.seeds, memo, eosio::name{0});
      makePayment(seeker.value().getHash(), assignee, ab.voice, memo, eosio::name{0});
      makePayment(seeker.value().getHash(), assignee, ab.husd, memo, eosio::name{0});
   }

   void dao::claimpayper(const eosio::checksum256 &assignment_hash, const eosio::checksum256 &period_hash)
   {
      eosio::check(!isPaused(), "Contract is paused for maintenance. Please try again later.");

      Document assignmentDocument(get_self(), assignment_hash);
      ContentWrapper assignment = assignmentDocument.getContentWrapper();

      eosio::check(assignment.getOrFail(SYSTEM, TYPE)->getAs<eosio::name>() == common::ASSIGNMENT,
                   "claim pay must be made on an assignment document: " + readableHash(assignment_hash));

      // assignee must still be a DHO member
      name assignee = assignment.getOrFail(DETAILS, ASSIGNEE)->getAs<eosio::name>();
      eosio::check(Member::isMember(get_self(), assignee), "assignee must be a current member to claim pay: " + assignee.to_string());
      require_auth(assignee);

      // Check that claimed period has not already been claimed
      Period payPeriod(this, period_hash);
      eosio::check(!Edge::exists(get_self(), assignmentDocument.getHash(), payPeriod.getHash(), common::CLAIMED),
                   "Assignment has already been claimed for this period. Assignment: " + readableHash(assignmentDocument.getHash()) +
                       ", period: " + readableHash(payPeriod.getHash()));

      // Ensure that the claimed period is within the approved period count
      Period startPeriod(this, assignment.getOrFail(DETAILS, START_PERIOD)->getAs<eosio::checksum256>());
      int64_t periodCount = assignment.getOrFail(DETAILS, PERIOD_COUNT)->getAs<int64_t>();
      int64_t counter = 0;
      bool found = false;
      std::optional<Period> seeker = std::optional<Period>{startPeriod};
      while (!found && seeker.has_value() && counter < periodCount)
      {
         if (seeker.value().getHash() == period_hash)
         {
            found = true;
         }
         counter++;
         seeker = seeker.value().next();
      }
      eosio::check(found, "Provided period hash does not fall within the assignments active time period.");

      // check that the period end time is in the past, meaning this period has elapsed
      std::optional<eosio::time_point> periodEndTime = payPeriod.getEndTime();
      eosio::check(periodEndTime != std::nullopt, "End of calendar has been reached. Contact administrator to add more time periods.");
      eosio::check(periodEndTime.value().sec_since_epoch() < eosio::current_time_point().sec_since_epoch(),
                   "This time period has not yet elapsed: " + readableHash(period_hash));

      // process this claim
      Edge::write(get_self(), get_self(), assignmentDocument.getHash(), seeker->getHash(), common::CLAIMED);

      // Pro-rate the payment if the assignment was created during the period being claimed
      float first_phase_ratio_calc = 1; // pro-rate based on elapsed % of the first phase
      if (assignmentDocument.getCreated().sec_since_epoch() > seeker->getStartTime().sec_since_epoch())
      {
         auto elapsed_sec = periodEndTime.value().sec_since_epoch() - assignmentDocument.getCreated().sec_since_epoch();
         auto period_sec = periodEndTime.value().sec_since_epoch() - seeker->getStartTime().sec_since_epoch();
         first_phase_ratio_calc = (float)elapsed_sec / (float)period_sec;
      }

      /******   DEFERRED SEEDS     */
      // If there is an explicit ESCROW SEEDS salary amount, support sending it; else it should be calculated
      eosio::asset deferredSeeds = asset{0, common::S_SEEDS};
      if (auto [idx, seedsEscrowSalary] = assignment.get(DETAILS, "seeds_escrow_salary_per_phase"); seedsEscrowSalary)
      {
         eosio::check(std::holds_alternative<eosio::asset>(seedsEscrowSalary->value), "fatal error: expected token type must be an asset value type: " + seedsEscrowSalary->label);
         deferredSeeds = std::get<eosio::asset>(seedsEscrowSalary->value);
      }
      else if (auto [idx, usdSalaryValue] = assignment.get(DETAILS, USD_SALARY_PER_PERIOD); usdSalaryValue)
      {
         eosio::check(std::holds_alternative<eosio::asset>(usdSalaryValue->value), "fatal error: expected token type must be an asset value type: " + usdSalaryValue->label);

         // Dynamically calculate the SEEDS amount based on the price at the end of the period being claimed
         deferredSeeds = getSeedsAmount(usdSalaryValue->getAs<eosio::asset>(),
                                        periodEndTime.value(),
                                        (float)(assignment.getOrFail(DETAILS, TIME_SHARE)->getAs<int64_t>() / (float)100),
                                        (float)(assignment.getOrFail(DETAILS, DEFERRED)->getAs<int64_t>() / (float)100));
      }
      deferredSeeds = adjustAsset(deferredSeeds, first_phase_ratio_calc);

      // These values are calculated when the assignment is proposed, so simply pro-rate them if/as needed
      // If there is an explicit INSTANT SEEDS amount, support sending it
      asset instantSeeds = getProRatedAsset(&assignment, common::S_SEEDS, "seeds_instant_salary_per_phase", first_phase_ratio_calc);
      asset husd = getProRatedAsset(&assignment, common::S_HUSD, HUSD_SALARY_PER_PERIOD, first_phase_ratio_calc);
      asset hvoice = getProRatedAsset(&assignment, common::S_HVOICE, HVOICE_SALARY_PER_PERIOD, first_phase_ratio_calc);
      asset hypha = getProRatedAsset(&assignment, common::S_HYPHA, HYPHA_SALARY_PER_PERIOD, first_phase_ratio_calc);

      string memo = "Payment for assignment " + readableHash(assignmentDocument.getHash()) + "; Period: " + readableHash(seeker->getHash());

      // creating a single struct improves performance for table queries here
      AssetBatch ab{};
      ab.d_seeds = deferredSeeds;
      ab.hypha = hypha;
      ab.seeds = instantSeeds;
      ab.voice = hvoice;
      ab.husd = husd;

      ab = applyBadgeCoefficients(*seeker, assignee, ab);

      makePayment(seeker->getHash(), assignee, ab.hypha, memo, eosio::name{0});
      makePayment(seeker->getHash(), assignee, ab.d_seeds, memo, common::ESCROW);
      makePayment(seeker->getHash(), assignee, ab.seeds, memo, eosio::name{0});
      makePayment(seeker->getHash(), assignee, ab.voice, memo, eosio::name{0});
      makePayment(seeker->getHash(), assignee, ab.husd, memo, eosio::name{0});
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

   eosio::asset dao::getSeedsAmount(const eosio::asset &usd_amount,
                                    const eosio::time_point &price_time_point,
                                    const float &time_share,
                                    const float &deferred_perc)
   {
      asset adjusted_usd_amount = adjustAsset(adjustAsset(usd_amount, deferred_perc), time_share);
      float seeds_deferral_coeff = (float)getSettingOrFail<int64_t>(SEEDS_DEFERRAL_FACTOR_X100) / (float)100;
      float seeds_price = getSeedsPriceUsd(price_time_point);
      return adjustAsset(asset{static_cast<int64_t>(adjusted_usd_amount.amount * (float)100 * (float)seeds_deferral_coeff),
                               common::S_SEEDS},
                         (float)1 / (float)seeds_price);
   }

   std::vector<Document> dao::getCurrentBadges(Period &period, const name &member)
   {
      std::vector<Document> current_badges;
      std::vector<Edge> badge_assignment_edges = m_documentGraph.getEdgesFrom(Member::getHash(member), common::ASSIGN_BADGE);
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
         if (std::holds_alternative<std::monostate>(coefficient->value))
         {
            return asset{0, base.symbol};
         }

         eosio::check(std::holds_alternative<int64_t>(coefficient->value), "fatal error: coefficient must be an int64_t type: key: " + key);

         float coeff_float = (float)((float)std::get<int64_t>(coefficient->value) / (float)10000);
         float adjustment = (float)coeff_float - (float)1;
         eosio::print (" applying a coeffecient: " + std::to_string(adjustment) + "\n");
         return adjustAsset(base, adjustment);
      }
      return asset{0, base.symbol};
   }

   dao::AssetBatch dao::applyBadgeCoefficients(Period &period, const eosio::name &member, dao::AssetBatch &ab)
   {
      // get list of badges
      auto badges = getCurrentBadges(period, member);
      AssetBatch applied_assets = ab;

      eosio::print (" badges found: " + std::to_string(badges.size()) + "\n");

      // for each badge, apply appropriate coefficients
      for (auto &badge : badges)
      {
         eosio::print (" applying badge: " + readableHash(badge.getHash()) + "\n");

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

      eosio::print(" making payment: " + quantity.to_string() + "\n");
      // eosio::check (false, "failed");

      std::unique_ptr<Payer> payer = std::unique_ptr<Payer>(PayerFactory::Factory(*this, quantity.symbol, paymentType));
      Document paymentReceipt = payer->pay(recipient, quantity, memo);
      Edge::write(get_self(), get_self(), fromNode, paymentReceipt.getHash(), common::PAYMENT);
      Edge::write(get_self(), get_self(), Member::getHash(recipient), paymentReceipt.getHash(), common::PAID);
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
      Member member(get_self(), enroller, applicant);
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

      //Might want to return by & instead of const &
      auto contentGroups = document.getContentGroups();
      auto &settingsGroup = contentGroups[0];

      ContentWrapper::insertOrReplace(settingsGroup, settingContent);
      ContentWrapper::insertOrReplace(settingsGroup, updateDateContent);

      m_documentGraph.updateDocument(get_self(), oldHash, std::move(contentGroups));
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

      eosio::print(" creating new period: " + readableHash(predecessor) + " label: " + label + "\n");
      Period newPeriod(this, start_time, label);
      eosio::print(" new period hash: " + readableHash(newPeriod.getHash()) + "\n");

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

   void dao::createroot(const std::string &notes)
   {
      require_auth(get_self());

      Document rootDoc(get_self(), get_self(), Content(ROOT_NODE, get_self()));

      //Create the settings document as well and add an edge to it
      ContentGroups settingCgs{{Content(CONTENT_GROUP_LABEL, SETTINGS),
                                Content(ROOT_NODE, readableHash(rootDoc.getHash()))}};

      Document settingsDoc(get_self(), get_self(), std::move(settingCgs));
      Edge::write(get_self(), get_self(), rootDoc.getHash(), settingsDoc.getHash(), common::SETTINGS_EDGE);
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
                       std::map<string, uint64_t> ints)
   {
      Migration migration(*this);
      migration.newObject(id, scope, names, strings, assets, time_points, ints);
   }
} // namespace hypha