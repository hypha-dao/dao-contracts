#include "pricing/billing_info.hpp"
#include "pricing/common.hpp"

#include "pricing/plan_manager.hpp"
#include "pricing/pricing_plan.hpp"
#include "pricing/price_offer.hpp"

#include <dao.hpp>

namespace hypha::pricing {

using namespace common;

BillingInfo::BillingInfo(dao& dao, uint64_t id)
    : TypedDocument(dao, id, types::BILLING_INFO)
{}

BillingInfo::BillingInfo(dao& dao, const PlanManager& planManager, const PricingPlan& pricingPlan, const PriceOffer& priceOffer, Data data)
    : TypedDocument(dao, types::BILLING_INFO)
{
    verifyData(data);
    
    auto cgs = convert(std::move(data));

    //TODO: Check special cases with links
    initializeDocument(dao, cgs);

    Edge(
        getDao().get_self(),
        getDao().get_self(),
        planManager.getId(),
        getId(),
        links::BILL
    );

    Edge(
        getDao().get_self(),
        getDao().get_self(),
        getId(),
        pricingPlan.getId(),
        links::PRICING_PLAN
    );

    Edge(
        getDao().get_self(),
        getDao().get_self(),
        getId(),
        priceOffer.getId(),
        links::PRICE_OFFER
    );
}

void BillingInfo::setNextBill(const BillingInfo& next)
{
    EOS_CHECK(
        getDao().getGraph().getEdgesFrom(getId(), links::NEXT_BILL).empty(),
        "BillingInfo has next bill already"
    )

    Edge(
        getDao().get_self(),
        getDao().get_self(),
        getId(),
        next.getId(),
        links::NEXT_BILL
    );
}

PricingPlan BillingInfo::getPricingPlan() {
    return PricingPlan(getDao(), Edge::get(
        getDao().get_self(),
        getId(),
        links::PRICING_PLAN
    ).getToNode());
}

PriceOffer BillingInfo::getPriceOffer() {
    return PriceOffer(getDao(), Edge::get(
        getDao().get_self(),
        getId(),
        links::PRICE_OFFER
    ).getToNode());
}

std::unique_ptr<BillingInfo> BillingInfo::getNextBill() {
    if (auto [exists, nextEdge] = Edge::getIfExists(getDao().get_self(), getId(), links::NEXT_BILL);
        exists) {
        return std::make_unique<BillingInfo>(getDao(), nextEdge.getToNode());
    }

    return nullptr;
}

void BillingInfo::verifyData(const Data& data)
{
    EOS_CHECK(
        data.expiration_date > data.start_date || data.is_infinite,
        "Expiration date must happen after Start Date"
    );

    EOS_CHECK(
        data.end_date > data.start_date || data.is_infinite,
        "End date must happen after Start Date"
    );

    //Only billing from 1 to 28
    EOS_CHECK(
        data.billing_day >= 1 && data.billing_day <= 28,
        to_str("Billing day is invalid: ", data.billing_day, " valid range [1,28] inclusive")
    );
}

}