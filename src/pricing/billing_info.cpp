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