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

namespace hypha
{
   void dao::propose(const name &proposer,
                     const name &proposal_type,
                     ContentGroups &content_groups)
   {
      eosio::check(!is_paused(), "Contract is paused for maintenance. Please try again later.");

      std::unique_ptr<Proposal> proposal = std::unique_ptr<Proposal>(ProposalFactory::Factory(*this, proposal_type));
      proposal->propose(proposer, content_groups);
   }

   void dao::closedocprop(const checksum256 &proposal_hash)
   {
      eosio::check(!is_paused(), "Contract is paused for maintenance. Please try again later.");

      Document docprop(get_self(), proposal_hash);
      name proposal_type = docprop.getContentWrapper().getOrFail(common::SYSTEM, common::TYPE)->getAs<eosio::name>();

      Proposal *proposal = ProposalFactory::Factory(*this, proposal_type);
      proposal->close(docprop);
   }

   void dao::claimpay(const eosio::checksum256 &hash, const uint64_t &period_id)
   {
      eosio::check(!is_paused(), "Contract is paused for maintenance. Please try again later.");

      Document assignmentDocument(get_self(), hash);
      ContentWrapper assignment = assignmentDocument.getContentWrapper();

      name assignee = assignment.getOrFail(common::DETAILS, common::ASSIGNEE)->getAs<eosio::name>();

      // assignee must still be a DHO member
      eosio::check(Member::isMember(get_self(), assignee), "assignee must be a current member to claim pay: " + assignee.to_string());
      require_auth(assignee);

      // Check that the assignment has not been paid for this period yet
      // asspay_table asspay_t(get_self(), get_self().value);
      // auto period_index = asspay_t.get_index<name("byperiod")>();
      // auto per_itr = period_index.find(period_id);
      // while (per_itr->period_id == period_id && per_itr != period_index.end())
      // {
      //    check(per_itr->assignment_id != assignment_id, "Assignment ID has already been paid for this period. Period ID: " +
      //                                                       std::to_string(period_id) + "; Assignment ID: " + std::to_string(assignment_id));
      //    per_itr++;
      // }

      // Check that the period has elapsed
      period_table period_t(get_self(), get_self().value);
      auto p_itr = period_t.find(period_id);
      eosio::check(p_itr != period_t.end(), "Cannot make payment. Period ID not found: " + std::to_string(period_id));
      eosio::check(p_itr->end_time.sec_since_epoch() < eosio::current_block_time().to_time_point().sec_since_epoch(),
                   "Cannot make payment. Period ID " + std::to_string(period_id) + " has not closed yet.");

      // Check that the creation date of the assignment is before the end of the period
      eosio::check(assignmentDocument.getCreated().sec_since_epoch() < p_itr->end_time.sec_since_epoch(),
                   "Cannot make payment to assignment. Assignment was not approved before this period.");

      // Check that pay period is between (inclusive) the start and end period of the assignment
      auto startPeriod = assignment.getOrFail(common::DETAILS, common::START_PERIOD)->getAs<int64_t>();
      auto endPeriod = assignment.getOrFail(common::DETAILS, common::END_PERIOD)->getAs<int64_t>();
      eosio::check(startPeriod <= period_id &&
                       endPeriod >= period_id,
                   "For assignment, period ID must be between " +
                       std::to_string(startPeriod) + " and " + std::to_string(endPeriod) +
                       " (inclusive). You tried: " + std::to_string(period_id));

      // We've disabled this check that confirms the period being claimed falls within a role's startPeriod and endPeriod
      // likely this will replaced with budgeting anyways
      // check(r_itr->ints.at("start_period") <= period_id && r_itr->ints.at("end_period") >= period_id, "For role, period ID must be between " +
      // std::to_string(r_itr->ints.at("start_period")) + " and " + std::to_string(r_itr->ints.at("end_period")) +
      // " (inclusive). You tried: " + std::to_string(period_id));

      // Pro-rate the payment if the assignment was created during the period being claimed
      float first_phase_ratio_calc = 1; // pro-rate based on elapsed % of the first phase
      if (assignmentDocument.getCreated().sec_since_epoch() > p_itr->start_time.sec_since_epoch())
      {
         auto elapsed_sec = p_itr->end_time.sec_since_epoch() - assignmentDocument.getCreated().sec_since_epoch();
         auto period_sec = p_itr->end_time.sec_since_epoch() - p_itr->start_time.sec_since_epoch();
         first_phase_ratio_calc = (float)elapsed_sec / (float)period_sec;
      }

      /******   DEFERRED SEEDS     */
      // If there is an explicit ESCROW SEEDS salary amount, support sending it; else it should be calculated
      eosio::asset deferredSeeds = asset{0, common::S_SEEDS};
      if (auto [idx, seedsEscrowSalary] = assignment.get(common::DETAILS, "seeds_escrow_salary_per_phase"); seedsEscrowSalary)
      {
         eosio::check(std::holds_alternative<eosio::asset>(seedsEscrowSalary->value), "fatal error: expected token type must be an asset value type: " + seedsEscrowSalary->label);
         deferredSeeds = std::get<eosio::asset>(seedsEscrowSalary->value);
      }
      else if (auto [idx, usdSalaryValue] = assignment.get(common::DETAILS, common::USD_SALARY_PER_PERIOD); usdSalaryValue)
      {
         eosio::check(std::holds_alternative<eosio::asset>(usdSalaryValue->value), "fatal error: expected token type must be an asset value type: " + usdSalaryValue->label);

         // Dynamically calculate the SEEDS amount based on the price at the end of the period being claimed
         deferredSeeds = getSeedsAmount(usdSalaryValue->getAs<eosio::asset>(),
                                        p_itr->end_time,
                                        (float)(assignment.getOrFail(common::DETAILS, common::TIME_SHARE)->getAs<int64_t>() / (float)100),
                                        (float)(assignment.getOrFail(common::DETAILS, common::DEFERRED)->getAs<int64_t>() / (float)100));
      }
      deferredSeeds = adjustAsset(deferredSeeds, first_phase_ratio_calc);

      // These values are calculated when the assignment is proposed, so simply pro-rate them if/as needed
      // If there is an explicit INSTANT SEEDS amount, support sending it
      asset instantSeeds = getProRatedAsset(&assignment, common::S_SEEDS, "seeds_instant_salary_per_phase", first_phase_ratio_calc);
      asset husd = getProRatedAsset(&assignment, common::S_HUSD, common::HUSD_SALARY_PER_PERIOD, first_phase_ratio_calc);
      asset hvoice = getProRatedAsset(&assignment, common::S_HVOICE, common::HVOICE_SALARY_PER_PERIOD, first_phase_ratio_calc);
      asset hypha = getProRatedAsset(&assignment, common::S_HYPHA, common::HYPHA_SALARY_PER_PERIOD, first_phase_ratio_calc);

      string memo = "Payment for assignment " + readableHash(assignmentDocument.getHash()) + "; Period ID: " + std::to_string(period_id);

      // creating a single struct improves performance for table queries here
      AssetBatch ab{};
      ab.d_seeds = deferredSeeds;
      ab.hypha = hypha;
      ab.seeds = instantSeeds;
      ab.voice = hvoice;
      ab.husd = husd;

      ab = applyBadgeCoefficients(period_id, assignee, ab);

      // Create the content groups for the Assignment Payment document, representing the
      // composite payment made to the member for this assignment for this period
      ContentGroups assignmentPaymentCgs{{Content(CONTENT_GROUP_LABEL, common::DETAILS),
                                          Content(common::ASSIGNMENT.to_string(), assignmentDocument.getHash()),
                                          Content(common::RECIPIENT, assignee),
                                          Content(common::PAYMENT_PERIOD, period_id)}};

      Document assignmentPayment(get_self(), get_self(), assignmentPaymentCgs);

      makePayment(assignmentPayment.getHash(), assignee, ab.hypha, memo, eosio::name{0});
      makePayment(assignmentPayment.getHash(), assignee, ab.d_seeds, memo, common::ESCROW);
      makePayment(assignmentPayment.getHash(), assignee, ab.seeds, memo, eosio::name{0});
      makePayment(assignmentPayment.getHash(), assignee, ab.voice, memo, eosio::name{0});
      makePayment(assignmentPayment.getHash(), assignee, ab.husd, memo, eosio::name{0});
   }

   asset dao::getProRatedAsset(ContentWrapper *assignment, const symbol &symbol, const string &key, const float &proration)
   {
      asset assetToPay = asset{0, symbol};
      if (auto [idx, assetContent] = assignment->get(common::DETAILS, key); assetContent)
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
      float seeds_deferral_coeff = (float)getSettingOrFail<int64_t>(common::SEEDS_DEFERRAL_FACTOR_X100) / (float)100;
      float seeds_price = getSeedsPriceUsd(price_time_point);
      return adjustAsset(asset{static_cast<int64_t>(adjusted_usd_amount.amount * (float)100 * (float)seeds_deferral_coeff),
                               common::S_SEEDS},
                         (float)1 / (float)seeds_price);
   }

   std::vector<Document> dao::getCurrentBadges(const uint64_t &period_id, const name &member)
   {
      std::vector<Document> current_badges;
      std::vector<Edge> badge_assignment_edges = m_documentGraph.getEdgesFrom(Member::getHash(member), common::ASSIGN_BADGE);
      for (Edge e : badge_assignment_edges)
      {
         Document badgeAssignmentDoc(get_self(), e.getToNode());
         auto badgeAssignment = badgeAssignmentDoc.getContentWrapper();
         int64_t start_period = badgeAssignment.getOrFail(common::DETAILS, common::START_PERIOD)->getAs<int64_t>();
         int64_t end_period = badgeAssignment.getOrFail(common::DETAILS, common::END_PERIOD)->getAs<int64_t>();

         // check that period_id falls within start_period and end_period
         if (period_id >= start_period && period_id <= end_period)
         {
            Edge badge_edge = Edge::get(get_self(), badgeAssignmentDoc.getHash(), common::BADGE_NAME);
            Document badge(get_self(), badge_edge.getToNode());
            current_badges.push_back(badge);
         }
      }
      return current_badges;
   }

   eosio::asset dao::applyCoefficient(ContentWrapper &badge, const eosio::asset &base, const std::string &key)
   {
      if (auto [idx, coefficient] = badge.get(common::DETAILS, key); coefficient)
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

   dao::AssetBatch dao::applyBadgeCoefficients(const uint64_t &period_id, const eosio::name &member, dao::AssetBatch &ab)
   {
      // get list of badges
      auto badges = getCurrentBadges(period_id, member);
      AssetBatch applied_assets = ab;

      // for each badge, apply appropriate coefficients
      for (auto &badge : badges)
      {
         ContentWrapper badgeCW = badge.getContentWrapper();
         applied_assets.hypha = applied_assets.hypha + applyCoefficient(badgeCW, ab.hypha, common::HYPHA_COEFFICIENT);
         applied_assets.husd = applied_assets.husd + applyCoefficient(badgeCW, ab.husd, common::HUSD_COEFFICIENT);
         applied_assets.voice = applied_assets.voice + applyCoefficient(badgeCW, ab.voice, common::HVOICE_COEFFICIENT);
         applied_assets.seeds = applied_assets.seeds + applyCoefficient(badgeCW, ab.seeds, common::SEEDS_COEFFICIENT);
         applied_assets.d_seeds = applied_assets.d_seeds + applyCoefficient(badgeCW, ab.d_seeds, common::SEEDS_COEFFICIENT);
      }

      return applied_assets;
   }

   void dao::makePayment(const eosio::checksum256 &fromNode,
                         const eosio::name &recipient,
                         const eosio::asset &quantity,
                         const string &memo,
                         const eosio::name &paymentType)
   {
      if (quantity.amount == 0)
      {
         return;
      }

      //std::unique_ptr<Payer> payer = std::unique_ptr<Payer>(PayerFactory::Factory(*this, quantity.symbol, paymentType));
      //Edge::write(get_self(), get_self(), fromNode, payer->pay(recipient, quantity, memo).getHash(), common::PAYMENT);
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

   bool dao::is_paused() { return false; }

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
      set_setting(key, value);
   }

   void dao::set_setting(const string &key, const Content::FlexValue &value)
   {
      auto document = getSettingsDocument();
      auto oldHash = document.getHash();
      auto settingContent = Content(key, value);
      auto updateDateContent = Content(common::UPDATED_DATE, eosio::current_time_point());

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
      remove_setting(key);
   }

   void dao::remove_setting(const string &key)
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
         auto updateDateContent = Content(common::UPDATED_DATE, eosio::current_time_point());
         ContentWrapper::insertOrReplace(settingsGroup, updateDateContent);
         m_documentGraph.updateDocument(get_self(), oldHash, std::move(contentGroups));
      }
      //Should we assert if setting doesn't exits ?
      eosio::check(false, "The specified setting does not exist: " + key);
   }

   void dao::addperiod(const eosio::time_point &start_time, const eosio::time_point &end_time, const string &label)
   {
      require_auth(get_self());
      Period period(*this, start_time, end_time, label);
      period.emplace();
   }

   void dao::createroot(const std::string &notes)
   {
      require_auth(get_self());

      Document rootDoc(get_self(), get_self(), Content(common::ROOT_NODE, get_self()));

      //Create the settings document as well and add an edge to it
      ContentGroups settingCgs{{Content(CONTENT_GROUP_LABEL, common::SETTINGS),
                                Content(common::ROOT_NODE, readableHash(rootDoc.getHash()))}};

      Document settingsDoc(get_self(), get_self(), std::move(settingCgs));
      Edge::write(get_self(), get_self(), rootDoc.getHash(), settingsDoc.getHash(), common::SETTINGS_EDGE);
   }

} // namespace hypha