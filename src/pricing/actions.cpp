#include <dao.hpp>

//#include <algorithm>
//#include <time.h>

#include "pricing/plan_manager.hpp"
#include "pricing/pricing_plan.hpp"
#include "pricing/price_offer.hpp"
#include "pricing/billing_info.hpp"
#include "pricing/common.hpp"

#include <eosio/time.hpp>

namespace hypha {

using namespace pricing;
using namespace pricing::common;

void dao::onCreditDao(uint64_t dao_id, const asset& amount)
{
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

static PricingPlan getDefaultPlan(dao& dao) {
    return PricingPlan(dao, Edge::get(dao.get_self(), 
                                      dao.getRootID(), 
                                      links::DEFAULT_PRICING_PLAN).getToNode());
}

ACTION dao::activateplan(ContentGroups& plan_info)
{
    auto cw = ContentWrapper(plan_info);

    auto daoID = cw.getOrFail(DETAILS, items::DAO_ID)
                    ->getAs<int64_t>();

    //Verify Auth
    checkAdminsAuth(daoID);

    //TODO: Verify if DAO is allowed to change it's current plan
    //child DAO's aren't

    auto defPlan = getDefaultPlan(*this).getId();

    auto planID = cw.getOrFail(DETAILS, items::PLAN_ID)
                        ->getAs<int64_t>();

    auto offerID = cw.getOrFail(DETAILS, items::OFFER_ID)
                        ->getAs<int64_t>();

    auto periods = cw.getOrFail(DETAILS, items::PERIODS)
                            ->getAs<int64_t>();

    EOS_CHECK(
        defPlan != planID,
        "Cannot change to default pricing plan"
    );

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
        planID != defPlan || currentPlan.getId() != defPlan,
        "DAO is already in default plan"
    );

    //Check if the last plan is valid or if it expired
    if (auto lastBill = planManager.getLastBill(); 
        lastBill.getIsInfinite() || now <= lastBill.getExpirationDate()) {
        
        if (currentPlan.getId() != plan.getId()) {
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
            t = planManager.getLastBill()
                        .getEndDate()
                        .sec_since_epoch();
        }
    }
    else {
        //Case where Current Plan already expired but we didn't update to the Default Plan

        //Nothing special to do for now
    }

    // EOS_CHECK(
    //     currentBill.getExpirationDate() <= eosio::current_time_point(),
    //     "Current Pricing Plan hasn't expired"
    // )

    EOS_CHECK(
        periods >= offer.getPeriodCount(),
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
        .expiration_date = end + eosio::seconds(plan.getReactivationPeriod()),
        .end_date = end,
        .billing_day = billingDay,
        .period_count = periods,
        .offer_discount_perc_x10000 = offer.getDiscountPercentage(),
        .discount_perc_x10000 = plan.getDiscountPercentage(),
        .plan_name = plan.getName(),
        .plan_price = plan.getPrice(),
        .total_paid = payAmount,
        .is_infinite = false
    });

    planManager.getLastBill().setNextBill(bill);
    planManager.setLastBill(bill);

    if (currentPlan.getId() != plan.getId()) {
        planManager.setCurrentBill(bill);
    }

    //TODO: Schedule action to terminate billing info
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

    for (auto& pricingPlanID : pricing_plan_ids) {
        PricingPlan pricingPlan(*this, pricingPlanID);
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
        .type = "prepaid"
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

// ACTION dao::updateprcpln(uint64_t pricing_plan_id, ContentGroups& pricing_plan_info)
// {
//     eosio::require_auth(get_self());

//     auto cw = ContentWrapper(pricing_plan_info);

//     PricingPlan plan(*this, pricing_plan_id);

//     auto [nIdx, name] = cw.get(DETAILS, items::NAME);
//     auto [pIdx, price] = cw.get(DETAILS, items::PRICE);
//     auto [rIdx, reactivationPeriod] = cw.get(DETAILS, items::REACTIVATION_PERIOD);
//     auto [dIdx, discount] = cw.get(DETAILS, items::DISCOUNT_PERCENTAGE);
//     auto [mIdx, maxMembers] = cw.get(DETAILS, items::MAX_MEMBER_COUNT);

//     plan.updateData(PricingPlanData {
//         .name = name ? name->getAs<std::string>() : plan.getName(),
//         .price = price ? price->getAs<eosio::asset>() : plan.getPrice(),
//         .reactivation_period_sec = reactivationPeriod ? reactivationPeriod->getAs<int64_t>() : plan.getReactivationPeriod(),
//         .discount_perc_x10000 = discount ? discount->getAs<int64_t>() : plan.getDiscountPercentage(),
//         .max_member_count = maxMembers ? maxMembers->getAs<int64_t>() : plan.getMaxMemberCount()
//     });
// }

// ACTION dao::updateprcoff(uint64_t price_offer_id, ContentGroups& price_offer_info)
// {
//     eosio::require_auth(get_self());

//     auto cw = ContentWrapper(price_offer_info);

//     PriceOffer offer(*this, price_offer_id);

//     auto [tIdx, tag] = cw.get(DETAILS, items::TAG);
//     auto [pIdx, periods] = cw.get(DETAILS, PERIOD_COUNT);
//     auto [dIdx, discount] = cw.get(DETAILS, items::DISCOUNT_PERCENTAGE);

//     offer.updateData(PriceOfferData{
//         .period_count = periods ? periods->getAs<int64_t>() : offer.getPeriodCount(),
//         .discount_perc_x10000 = discount ? discount->getAs<int64_t>() : offer.getDiscountPercentage(),
//         .tag = tag ? tag->getAs<std::string>() : offer.getTag(),
//     });
// }

ACTION dao::updatecurbil(uint64_t dao_id)
{
    //Prob it doens't require special perms
    checkAdminsAuth(dao_id);

    auto planManager = PlanManager::getFromDaoID(*this, dao_id);

    auto currentBill = planManager.getCurrentBill();

    //Doesn't need to do anything if it's infinite
    if (!currentBill.getIsInfinite()) {

    }
}

}