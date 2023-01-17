#include <dao.hpp>

//#include <algorithm>
//#include <time.h>

#include "pricing/plan_manager.hpp"
#include "pricing/pricing_plan.hpp"
#include "pricing/price_offer.hpp"
#include "pricing/billing_info.hpp"
#include "pricing/common.hpp"
#include "pricing/features.hpp"

#include <eosio/time.hpp>

namespace hypha {

using namespace pricing;
using namespace pricing::common;

// 3 year expiry
#define STAKING_EXPIRY_MICROSECONDS eosio::days(3 * 365)

void dao::onCreditDao(uint64_t dao_id, const asset& amount)
{
    verifyDaoType(dao_id);

    EOS_CHECK(
        amount.amount > 0,
        "Credit amount must be positive"
    )

    //Verify Dao ID is valid
    EOS_CHECK(
        NameToID::exists<dao_table>(*this, dao_id),
        "Provided DAO ID doesn't belong to an existing DAO"
    )
    
    //Add funds to the Plan Manager of the specified DAO
    auto planManager = PlanManager::getFromDaoID(*this, dao_id);

    planManager.addCredit(amount);

    planManager.update();   
}

static auto getStartAndEndDates(time_t t, int64_t periods)
{
    tm* tmd = nullptr;

    tmd = gmtime(&t);

    int64_t billingDay = tmd->tm_mday;

    //If the day is > 28, then we will start next month on the first day
    if (billingDay > 28) {
        tmd->tm_mday = 1;
        tmd->tm_year += (tmd->tm_mon + 1) / 12;
        tmd->tm_mon = (tmd->tm_mon + 1) % 12;
        billingDay = tmd->tm_mday;
    }

    auto startTime = mktime(tmd);

    tmd->tm_year += (tmd->tm_mon + periods) / 12;
    tmd->tm_mon = (tmd->tm_mon + periods) % 12;

    auto endTime = mktime(tmd);

    struct Dates { 
        eosio::time_point start; 
        eosio::time_point end;
        int64_t billingDay;
    };

    return Dates {
        .start = eosio::time_point(eosio::seconds(startTime)),
        .end = eosio::time_point(eosio::seconds(endTime)),
        .billingDay = billingDay
    };
};

void dao::setEcosystemPlan(PlanManager& planManager)
{
    PricingPlan ecosystemPlan = PlanManager::getEcosystemPlan(*this);

    auto offers = ecosystemPlan.getOffers();

    EOS_CHECK(
        !offers.empty(),
        "Ecosystem Plan must have 1 offer at least"
    );
    
    PriceOffer& offer = offers.back();

    time_t t = eosio::current_time_point().sec_since_epoch();

    auto [start,end,billingDay] = getStartAndEndDates(t, 0);

    BillingInfo defBill(*this, 
                        planManager, 
                        ecosystemPlan, 
                        offer, 
                        BillingInfoData {
                            .start_date = start,
                            .expiration_date = eosio::time_point(),
                            .end_date = eosio::time_point(),
                            .billing_day = billingDay,
                            .period_count = -1,
                            .discount_perc_x10000 = ecosystemPlan.getDiscountPercentage(),
                            .offer_discount_perc_x10000 = offer.getDiscountPercentage(),
                            .plan_name = ecosystemPlan.getName(),
                            .plan_price = ecosystemPlan.getPrice(),
                            .total_paid = ecosystemPlan.getPrice(),
                            .is_infinite = true
                        });

    planManager.setCurrentBill(defBill);
    planManager.setLastBill(defBill);
    planManager.setStartBill(defBill);
}

static void scheduleBillUpdate(const BillingInfo& bill, uint64_t daoID)
{
    //TODO: Schedule action to terminate plan/bill
    //should happen twice, first after billing end date, and after billing expiration
    //date unless we downgraded to def plan

    // eosio::transaction trx;
    // trx.actions.emplace_back(eosio::action(
    //     permission_level(m_dao.get_self(), "active"_n),
    //     m_dao.get_self(),
    //     "closedocprop"_n,
    //     std::make_tuple(proposal.getID())
    // ));

    // auto expiration = proposal.getContentWrapper().getOrFail(BALLOT, EXPIRATION_LABEL, "Proposal has no expiration")->getAs<eosio::time_point>();

    // constexpr auto aditionalDelaySec = 60;
    // trx.delay_sec = (expiration.sec_since_epoch() - eosio::current_time_point().sec_since_epoch()) + aditionalDelaySec;

    // auto nextID = m_dhoSettings->getSettingOrDefault("next_schedule_id", int64_t(0));

    // trx.send(util::hashCombine(nextID, proposal.getID()), m_dao.get_self());

    // m_dhoSettings->setSetting(Content{"next_schedule_id", nextID + 1});
}

void dao::verifyEcosystemPayment(PlanManager& planManager, const string& priceItem, const string& priceStakedItem, const std::string& stakingMemo, const name& beneficiary)
{
  auto settings = getSettingsDocument();

  auto price = settings->getOrFail<asset>(groups::ECOSYSTEM, priceItem);
  
  auto priceStaked = settings->getOrFail<asset>(groups::ECOSYSTEM, priceStakedItem);

  //Verify the current credit >= price + stakedPrice
  planManager.removeCredit(price + priceStaked);

  planManager.update();

  //TODO: Burn tokens equivalent to price
  
  auto stakingContract = settings->getOrFail<name>(HYPHA_COSALE_CONTRACT);

  //Create lock for beneficiary account
  eosio::action(
      eosio::permission_level{get_self(), eosio::name("active")},
      settings->getOrFail<name>(REWARD_TOKEN_CONTRACT), 
      eosio::name("transfer"),
      std::make_tuple(
          get_self(), 
          stakingContract, 
          priceStaked, 
          ""
      )
  ).send();

  eosio::action(
      eosio::permission_level{get_self(), name("active")},
      stakingContract, 
      name("createlock"),
      std::make_tuple(
          get_self(),
          beneficiary,
          priceStaked,
          stakingMemo,
          eosio::time_point(eosio::current_time_point().time_since_epoch() + STAKING_EXPIRY_MICROSECONDS)
      )
  ).send();
}

ACTION dao::markasecosys(uint64_t dao_id)
{
    verifyDaoType(dao_id);

    eosio::require_auth(get_self());

    auto daoDoc = Document(get_self(), dao_id);

    auto daoCW = daoDoc.getContentWrapper();

    auto& daoType = daoCW.getOrFail(DETAILS, hypha::common::DAO_TYPE)
                         ->getAs<string>();

    EOS_CHECK(
        daoType == dao_types::INDIVIDUAL,
        "Only Individual DAO's can get upgraded to Ecosystem"
    );

    daoType = dao_types::ANCHOR;

    auto detailsGroup = daoDoc.getContentWrapper()
                              .getGroupOrFail(DETAILS);

    ContentWrapper::insertOrReplace(
        *detailsGroup, 
        Content{ items::IS_WAITING_ECOSYSTEM, 1 }
    );

    daoDoc.update();
    
    //Assign a PlanManager if it doesn't have one
    if (!Edge::getIfExists(get_self(),
                          dao_id,
                          links::PLAN_MANAGER).first) {
        activatedao(
            daoCW.getOrFail(DETAILS, DAO_NAME)
                 ->getAs<eosio::name>()
        );
    }

    auto planManager = PlanManager::getFromDaoID(*this, dao_id);

    //Verify the current pricing plan is the default one
    EOS_CHECK(
        planManager.getCurrentBill()
                   .getPricingPlan()
                   .getId() ==
        PlanManager::getDefaultPlan(*this).getId(),
        "Current Pricing plan must be the default one"
    );

    planManager.setType(types::UNLIMITED);

    planManager.update();
}

ACTION dao::setdaotype(uint64_t dao_id, const string& dao_type)
{
    verifyDaoType(dao_id);

    eosio::require_auth(get_self());

    auto daoDoc = Document(get_self(), dao_id);

    auto& daoType = daoDoc.getContentWrapper()
                          .getOrFail(DETAILS, hypha::common::DAO_TYPE)
                          ->getAs<string>();

    EOS_CHECK(
        dao_type == dao_types::ANCHOR || 
        dao_type == dao_types::INDIVIDUAL,
        "Unknown DAO type"
    );

    EOS_CHECK(
        daoType != dao_type,
        to_str("DAO it's already of type: ", dao_type)
    );

    daoType = dao_type;

    daoDoc.update();
}

ACTION dao::activateecos(ContentGroups& ecosystem_info)
{
    EOS_CHECK(false, "This action is not enabled");
    
    auto cw = ContentWrapper(ecosystem_info);

    auto daoID = cw.getOrFail(DETAILS, items::DAO_ID)
                    ->getAs<int64_t>();

    //Verify Auth
    checkAdminsAuth(daoID);

    auto daoDoc = Document(get_self(), daoID);

    auto daoCW = daoDoc.getContentWrapper();

    auto [detailsIdx, detailsGroup] = daoCW.getGroup(DETAILS);

    EOS_CHECK(
        detailsIdx >= 0,
        "Missing details group from DAO doc"
    );

    auto& daoType = daoCW.getOrFail(detailsIdx, hypha::common::DAO_TYPE)
                        .second->getAs<string>();

    auto& isWaiting = daoCW.getOrFail(detailsIdx, items::IS_WAITING_ECOSYSTEM)
                          .second->getAs<int64_t>();

    //Check if it's actually waiting for ecosystem activation
    EOS_CHECK(
        daoType == dao_types::ANCHOR &&
        isWaiting == 1,
        "This DAO cannot be upgraded to Ecosystem"
    );

    isWaiting = 0;

    daoDoc.update();
    
    auto planManager = PlanManager::getFromDaoID(*this, daoID);

    auto beneficiary = cw.getOrFail(DETAILS, hypha::common::BENEFICIARY_ACCOUNT)->getAs<name>();

    verifyEcosystemPayment(
        planManager,
        items::ECOSYSTEM_PRICE,
        items::ECOSYSTEM_PRICE_STAKED,
        to_str("Staking HYPHA on DAO Ecosystem activation:", daoID),
        beneficiary
    );

    //Save the start bill in a new edge as it will be overrided
    //by the Thrive plan
    Edge(
        get_self(), 
        get_self(), 
        planManager.getId(), 
        planManager.getStartBill().getId(),
        links::PREV_START_BILL
    );

    //Use default price plan
    setEcosystemPlan(planManager);
}

ACTION dao::activateplan(ContentGroups& plan_info)
{
    auto cw = ContentWrapper(plan_info);

    auto daoID = cw.getOrFail(DETAILS, items::DAO_ID)
                    ->getAs<int64_t>();

    //Verify Auth
    checkAdminsAuth(daoID);

    auto daoType = Document(get_self(), daoID).getContentWrapper()
                                               .getOrFail(DETAILS, hypha::common::DAO_TYPE)
                                               ->getAs<string>();

    //Verify if DAO is allowed to change it's current plan
    //i.e. child & anchor DAOs aren't
    EOS_CHECK(
        daoType == dao_types::INDIVIDUAL,
        "This type of DAO cannot have a single plan"
    );

    auto defPlan = PlanManager::getDefaultPlan(*this).getId();

    auto planID = cw.getOrFail(DETAILS, items::PLAN_ID)
                        ->getAs<int64_t>();

    auto offerID = cw.getOrFail(DETAILS, items::OFFER_ID)
                        ->getAs<int64_t>();

    auto periods = cw.getOrFail(DETAILS, items::PERIODS)
                            ->getAs<int64_t>();

    PricingPlan plan(*this, planID);

    PriceOffer offer(*this, offerID);

    EOS_CHECK(
        plan.hasOffer(offerID),
        "Offer is not valid for the requested Pricing plan"
    )

    auto now = eosio::current_time_point();
    time_t t = now.sec_since_epoch();
     
    if (auto [_, startDate] = cw.get(DETAILS, items::START_DATE); 
        startDate) {
        auto date = startDate->getAs<eosio::time_point>();
        // //We can bypass this if called by the contract account
        EOS_CHECK(
            /*date >= eosio::current_time_point() ||*/ eosio::has_auth(get_self()),
            "Start Date cannot be in the past"
        )

        t = date.sec_since_epoch();
    }

    PlanManager planManager = PlanManager::getFromDaoID(*this, daoID);

    bool downgradeToDef = defPlan == planID;

    auto currentBill = planManager.getCurrentBill();
    auto currentPlan = currentBill.getPricingPlan();

    EOS_CHECK(
        !currentBill.getIsInfinite() || !currentBill.getNextBill(),
        "Update your current plan before activating a new one, contact an admin"
    )

    EOS_CHECK(
        now < currentBill.getEndDate() || !currentBill.getNextBill(),
        "Update your current plan before activating a new one, contact an admin"
    )

    EOS_CHECK(
        !downgradeToDef || currentPlan.getId() != defPlan,
        "DAO is already in default plan"
    );

    bool needUpdateCurBill = false;

    //Check if the last plan is valid or if it expired
    if (auto lastBill = planManager.getLastBill();
        lastBill.getIsInfinite() || now <= lastBill.getExpirationDate()) {

        EOS_CHECK(
            !downgradeToDef || 
            (!lastBill.getIsInfinite() && now > lastBill.getEndDate()), //When downgrading in grace period
            "Cannot change to default pricing plan"
        )
        
        if (currentPlan.getId() != plan.getId()) {

            needUpdateCurBill = true;
            //Calculate credit of remaining periods since
            //we are doing a plan switch
            if (currentPlan.getId() != defPlan) {
                //NOW: Think about the case where we are chaning the plan
                //but we are on the grace period endDate < now
                planManager.addCredit(planManager.calculateCredit());
                planManager.setLastBill(currentBill);
                
                //Remove upcoming bills
                auto next = currentBill.getNextBill();
                while (next) {
                    auto tmp = next->getNextBill();
                    next->erase();
                    next = std::move(tmp);
                }
            }

            //End current plan default plan
            if (currentBill.getIsInfinite() || now < currentBill.getEndDate()) {    
                currentBill.setEndDate(eosio::time_point(eosio::seconds(t)));
                currentBill.setExpirationDate(eosio::time_point(eosio::seconds(t)));
                currentBill.setIsInfinite(false);
                currentBill.update();
            }
        }
        else {
            //Same plan
            t = lastBill.getEndDate()
                        .sec_since_epoch();
        }
    }
    else {
        //Case where Current Plan already expired but we didn't update to the Default Plan
        needUpdateCurBill = true;
    }

    // EOS_CHECK(
    //     currentBill.getExpirationDate() <= eosio::current_time_point(),
    //     "Current Pricing Plan hasn't expired"
    // )

    EOS_CHECK(
        periods >= offer.getPeriodCount() || downgradeToDef,
        to_str("Period amount must be greater or equal to ", offer.getPeriodCount())
    )

    float offerDisc = 1.0f - offer.getDiscountPercentage() / 10000.f;

    float priceDisc = 1.0f - plan.getDiscountPercentage() / 10000.f;

    auto payAmount = adjustAsset(plan.getPrice(), priceDisc * offerDisc * periods);

    planManager.removeCredit(payAmount);

    planManager.update();
    
    auto [start,end,billingDay] = getStartAndEndDates(t, periods);

    BillingInfo bill(*this, planManager, plan, offer, BillingInfoData {
        .start_date = start,
        .expiration_date = downgradeToDef ? eosio::time_point{} : end + eosio::seconds(plan.getReactivationPeriod()),
        .end_date = downgradeToDef ? eosio::time_point{} : end,
        .billing_day = billingDay,
        .period_count = downgradeToDef ? -1 : periods,
        .offer_discount_perc_x10000 = offer.getDiscountPercentage(),
        .discount_perc_x10000 = plan.getDiscountPercentage(),
        .plan_name = plan.getName(),
        .plan_price = plan.getPrice(),
        .total_paid = payAmount,
        .is_infinite = downgradeToDef //If downgrading to free plan
    });

    auto lastBill = planManager.getLastBill();

    EOS_CHECK(
        lastBill.getEndDate() <= bill.getStartDate(),
        "Previus bill end date must happen before new bill start date"
    )

    lastBill.setNextBill(bill);
    planManager.setLastBill(bill);

    if (needUpdateCurBill) {
        planManager.setCurrentBill(bill);

        if (lastBill.getPricingPlan().getId() != planID) {
            onDaoPlanChange(*this, daoID, plan);
        }
    }

    if (!bill.getIsInfinite()) {
        scheduleBillUpdate(bill, daoID);
    }
}

ACTION dao::addprcngplan(ContentGroups& pricing_plan_info, const std::vector<uint64_t>& offer_ids)
{
    require_auth(get_self());

    auto cw = ContentWrapper(pricing_plan_info);

    //TODO: Add data validators (create validators.hpp and make custom validators, also
    //allow chained validators i.e. MultiValidator(RangeValidator(1, 1000), NotEq([3, 10, 20])))

    PricingPlan pricingPlan(*this, PricingPlanData {
        .name = std::move(cw.getOrFail(DETAILS, items::NAME)->getAs<std::string>()),
        .price = std::move(cw.getOrFail(DETAILS, items::PRICE)->getAs<eosio::asset>()),
        .reactivation_period_sec = cw.getOrFail(DETAILS, items::REACTIVATION_PERIOD)->getAs<int64_t>(),
        .max_member_count = cw.getOrFail(DETAILS, items::MAX_MEMBER_COUNT)->getAs<int64_t>(),
        .discount_perc_x10000 = cw.getOrFail(DETAILS, items::DISCOUNT_PERCENTAGE)->getAs<int64_t>()
    });

    for (auto& offerID : offer_ids) {
        PriceOffer offer(*this, offerID);
        pricingPlan.addOffer(offer);
    }
}

ACTION dao::addpriceoffr(ContentGroups& price_offer_info, const std::vector<uint64_t>& pricing_plan_ids)
{
    require_auth(get_self());

    auto cw = ContentWrapper(price_offer_info);

    const auto& periodCount = cw.getOrFail(DETAILS, "period_count")->getAs<int64_t>();
    const auto& discountPerc = cw.getOrFail(DETAILS, "discount_perc_x10000")->getAs<int64_t>();
    auto& tag = cw.getOrFail(DETAILS, "tag")->getAs<std::string>();
    
    //TODO: Add data validators (create validators.hpp and make custom validators, also
    //allow chained validators i.e. MultiValidator(RangeValidator(1, 1000), NotEq([3, 10, 20])))

    PriceOffer offer(*this, PriceOfferData {
       .period_count = periodCount,
       .discount_perc_x10000 = discountPerc,
       .tag = std::move(tag)
    });

    auto defPricePlan = PlanManager::getDefaultPlan(*this);

    for (auto& pricingPlanID : pricing_plan_ids) {
        PricingPlan pricingPlan(*this, pricingPlanID);
        
        EOS_CHECK(
            pricingPlanID != defPricePlan.getId() ||
            defPricePlan.getOffers().empty(),
            "Cannot add more than 1 offer to default price plan"
        );

        pricingPlan.addOffer(offer);
    }
}

ACTION dao::setdefprcpln(uint64_t price_plan_id)
{
    require_auth(get_self());

    if (auto [exists, edge] = Edge::getIfExists(get_self(), getRootID(), links::DEFAULT_PRICING_PLAN); 
        exists) {
        edge.erase();
    }

    PricingPlan pricingPlan(*this, price_plan_id);

    Edge(
        get_self(),
        get_self(),
        getRootID(),
        pricingPlan.getId(),
        links::DEFAULT_PRICING_PLAN
    );
}

static std::vector<PriceOffer> getPriceOffers(dao& dao, const std::vector<uint64_t>& offerIDs) 
{
    auto offers = std::vector<PriceOffer>();
    offers.reserve(offerIDs.size());
    auto toPriceOffer = [&dao](uint64_t offerID) { return PriceOffer(dao, offerID); };
    std::transform(offerIDs.begin(), offerIDs.end(), std::back_inserter(offers), toPriceOffer);
    return offers;
}

ACTION dao::modoffers(const std::vector<uint64_t>& pricing_plan_ids, const std::vector<uint64_t>& offer_ids, bool unlink)
{
    require_auth(get_self());

    auto offers = getPriceOffers(*this, offer_ids);

    for (auto& pricingPlanID : pricing_plan_ids) {
        PricingPlan pricingPlan(*this, pricingPlanID);
        
        for (auto& offer : offers) {
            if (unlink) {
                pricingPlan.removeOffer(offer);
            }
            else {
                pricingPlan.addOffer(offer);
            }
        }
    }
}

ACTION dao::activatedao(eosio::name dao_name)
{
    eosio::require_auth(get_self());

    auto daoID = getDAOID(dao_name);

    EOS_CHECK(daoID.has_value(), "Invalid DAO name");

    auto settings = getSettingsDocument(*daoID);

    PlanManager planManager(*this, dao_name, PlanManagerData {
        .credit = eosio::asset{ 0, hypha::common::S_HYPHA },
        .type = types::PREPAID
    });

    //Use default price plan
    PricingPlan defPlan(*this, Edge::get(get_self(), 
                                         getRootID(), 
                                         links::DEFAULT_PRICING_PLAN).getToNode());

    auto offers = defPlan.getOffers();

    EOS_CHECK(
        offers.size() == 1,
        "Default Plan must have only 1 linked offer"
    )

    EOS_CHECK(
        defPlan.getPrice().amount == 0,
        "Default Plan must be free"
    )

    PriceOffer& offer = offers.back();

    time_t t = eosio::current_time_point().sec_since_epoch();

    auto [start,end,billingDay] = getStartAndEndDates(t, 0);

    BillingInfo defBill(*this, 
                        planManager, 
                        defPlan, 
                        offer, 
                        BillingInfoData {
                            .start_date = start,
                            .expiration_date = eosio::time_point(),
                            .end_date = eosio::time_point(),
                            .billing_day = billingDay,
                            .period_count = -1,
                            .discount_perc_x10000 = defPlan.getDiscountPercentage(),
                            .offer_discount_perc_x10000 = offer.getDiscountPercentage(),
                            .plan_name = defPlan.getName(),
                            .plan_price = defPlan.getPrice(),
                            .total_paid = defPlan.getPrice(),
                            .is_infinite = true
                        });

    planManager.setCurrentBill(defBill);
    planManager.setLastBill(defBill);
    planManager.setStartBill(defBill);
}

ACTION dao::updateprcpln(uint64_t pricing_plan_id, ContentGroups& pricing_plan_info)
{
    eosio::require_auth(get_self());

    auto cw = ContentWrapper(pricing_plan_info);

    PricingPlan plan(*this, pricing_plan_id);

    auto [nIdx, name] = cw.get(DETAILS, items::NAME);
    auto [pIdx, price] = cw.get(DETAILS, items::PRICE);
    auto [rIdx, reactivationPeriod] = cw.get(DETAILS, items::REACTIVATION_PERIOD);
    auto [dIdx, discount] = cw.get(DETAILS, items::DISCOUNT_PERCENTAGE);
    auto [mIdx, maxMembers] = cw.get(DETAILS, items::MAX_MEMBER_COUNT);

    plan.updateData(PricingPlanData {
        .name = name ? name->getAs<std::string>() : plan.getName(),
        .price = price ? price->getAs<eosio::asset>() : plan.getPrice(),
        .reactivation_period_sec = reactivationPeriod ? reactivationPeriod->getAs<int64_t>() : plan.getReactivationPeriod(),
        .discount_perc_x10000 = discount ? discount->getAs<int64_t>() : plan.getDiscountPercentage(),
        .max_member_count = maxMembers ? maxMembers->getAs<int64_t>() : plan.getMaxMemberCount()
    });
}

ACTION dao::updateprcoff(uint64_t price_offer_id, ContentGroups& price_offer_info)
{
    eosio::require_auth(get_self());

    auto cw = ContentWrapper(price_offer_info);

    PriceOffer offer(*this, price_offer_id);

    auto [tIdx, tag] = cw.get(DETAILS, items::TAG);
    auto [pIdx, periods] = cw.get(DETAILS, PERIOD_COUNT);
    auto [dIdx, discount] = cw.get(DETAILS, items::DISCOUNT_PERCENTAGE);

    offer.updateData(PriceOfferData{
        .period_count = periods ? periods->getAs<int64_t>() : offer.getPeriodCount(),
        .discount_perc_x10000 = discount ? discount->getAs<int64_t>() : offer.getDiscountPercentage(),
        .tag = tag ? tag->getAs<std::string>() : offer.getTag(),
    });
}

ACTION dao::updatecurbil(uint64_t dao_id)
{
    verifyDaoType(dao_id);

    //Prob it doens't require special perms
    checkAdminsAuth(dao_id);

    auto planManager = PlanManager::getFromDaoID(*this, dao_id);

    auto currentBill = planManager.getCurrentBill();

    //Doesn't need to do anything if it's infinite
    if (!currentBill.getIsInfinite()) {

        auto now = eosio::current_time_point();

        EOS_CHECK(
            currentBill.getEndDate() <= currentBill.getExpirationDate(),
            to_str("Wrong expiration date and end date values for bill: ", currentBill.getId())
        )

        //Let's check if we are already over the expiration date
        if (currentBill.getEndDate() < now) {

            bool resetToDefault = currentBill.getExpirationDate() < now;
    
            auto bill = currentBill.getNextBill();

            while (bill) {
                resetToDefault = true;
                auto next = bill->getNextBill();
                if (bill->getIsInfinite() ||
                    now < bill->getEndDate() ||
                    (now < bill->getExpirationDate() && !next)) {
                    planManager.setCurrentBill(*bill);
                    auto updatedPlan = bill->getPricingPlan();
                    if (currentBill.getPricingPlan().getId() != updatedPlan.getId()) {
                        onDaoPlanChange(*this, dao_id, updatedPlan);
                    }
                    return;
                }
                bill = std::move(next);
            }

            //Let's restore to default plan then
            if (resetToDefault) {

                auto defPlan = PlanManager::getDefaultPlan(*this);

                auto defOffers = defPlan.getOffers();

                EOS_CHECK(
                    defOffers.size() == 1,
                    to_str("Default Price has the wrong number of offers linked: ", defOffers.size())
                )

                eosio::action(
                    eosio::permission_level(get_self(), "active"_n),
                    get_self(),
                    "activateplan"_n,
                    std::make_tuple(
                        ContentGroups {
                            ContentGroup {
                                Content{ CONTENT_GROUP_LABEL, DETAILS },
                                Content{ items::DAO_ID, static_cast<int64_t>(dao_id) },
                                Content{ items::PLAN_ID, static_cast<int64_t>(defPlan.getId()) },
                                Content{ items::OFFER_ID, static_cast<int64_t>(defOffers.back().getId()) },
                                Content{ items::PERIODS, -1 }
                            }
                        }
                    )
                ).send();
            }
            //Else do nothing and wait till the bill expires
        }
    }
}

}