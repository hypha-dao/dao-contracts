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

    time_t t = eosio::current_time_point().sec_since_epoch();
     
    // if (auto [_, startDate] = cw.get(DETAILS, items::START_DATE); 
    //     startDate) {
    //     auto date = startDate->getAs<eosio::time_point>();
    //     // //We can bypass this if called by the contract account
    //     EOS_CHECK(
    //         date >= eosio::current_time_point() || eosio::has_auth(get_self()),
    //         "Start Date cannot be in the past"
    //     )

    //     t = date.sec_since_epoch();
    // }

    PlanManager planManager = PlanManager::getFromDaoID(*this, daoID);

    auto currentBill = planManager.getCurrentBill();
    auto currentPlan = currentBill.getPricingPlan();

    if (currentPlan.getId() == defPlan || 
        currentPlan.getId() != plan.getId()) {
        //End current plan default plan
        currentBill.setEndDate(eosio::time_point(eosio::seconds(t)));
        currentBill.setExpirationDate(eosio::time_point(eosio::seconds(t)));
        currentBill.setIsInfinite(false);
        currentBill.update();

        //Calculate credit of remaining periods since
        //we are doing a plan switch
        if (currentPlan.getId() != defPlan) {
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
    }
    else {
        //Same plan
        t = planManager.getLastBill()
                       .getEndDate()
                       .sec_since_epoch();
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

    auto payAmount = adjustAsset(plan.getPrice(), priceDisc * offerDisc) * periods;

    planManager.removeCredit(payAmount);

    planManager.update();
    auto [start,end,billingDay] = getStartAndEndDates(t, periods);

    BillingInfo bill(*this, planManager, plan, offer, BillingInfoData {
        .start_date = start,
        .expiration_date = end + eosio::seconds(plan.getReactivationPeriod()),
        .end_date = end,
        .billing_day = billingDay,
        .period_count = periods,
        .discount_perc_x10000 = plan.getDiscountPercentage(),
        .plan_name = plan.getName(),
        .plan_price = plan.getPrice(),
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

    auto& name = cw.getOrFail(DETAILS, "name")->getAs<std::string>();
    auto& price = cw.getOrFail(DETAILS, "price")->getAs<eosio::asset>();
    const auto& reactivationPeriod = cw.getOrFail(DETAILS, "reactivation_period_sec")->getAs<int64_t>();
    const auto& maxMemberCount = cw.getOrFail(DETAILS, "max_member_count")->getAs<int64_t>();
    const auto& discountPerc = cw.getOrFail(DETAILS, "discount_perc_x10000")->getAs<int64_t>();

    //TODO: Add data validators (create validators.hpp and make custom validators, also
    //allow chained validators i.e. MultiValidator(RangeValidator(1, 1000), NotEq([3, 10, 20])))

    PricingPlan pricingPlan(*this, PricingPlanData {
        .name = std::move(name),
        .price = std::move(price),
        .reactivation_period_sec = reactivationPeriod,
        .max_member_count = maxMemberCount,
        .discount_perc_x10000 = discountPerc
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
                            .discount_perc_x10000 = offer.getDiscountPercentage(),
                            .plan_name = defPlan.getName(),
                            .plan_price = defPlan.getPrice(),
                            .is_infinite = true
                        });

    planManager.setCurrentBill(defBill);
    planManager.setLastBill(defBill);
    planManager.setStartBill(defBill);
}

}