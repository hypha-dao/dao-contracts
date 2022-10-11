#pragma once

#include <memory>

#include <typed_document.hpp>
#include <macros.hpp>

namespace hypha::pricing {

class PlanManager;
class PriceOffer;
class PricingPlan;

class BillingInfo : public TypedDocument
{
    DECLARE_DOCUMENT(
        Data,
        PROPERTY(start_date, eosio::time_point, StartDate),
        PROPERTY(expiration_date, eosio::time_point, ExpirationDate),
        PROPERTY(end_date, eosio::time_point, EndDate),
        PROPERTY(billing_day, int64_t, BillingDay),
        PROPERTY(period_count, int64_t, PeriodCount),
        PROPERTY(discount_perc_x10000, int64_t, DiscountPercentage),
        PROPERTY(offer_discount_perc_x10000, int64_t, OfferDiscountPercentage),
        PROPERTY(plan_name, std::string, PlanName),
        PROPERTY(plan_price, eosio::asset, PlanPrice),
        PROPERTY(total_paid, eosio::asset, TotalPaid),
        PROPERTY(is_infinite, int64_t, IsInfinite)
    )
public:
    BillingInfo(dao& dao, uint64_t id);
    BillingInfo(dao& dao, const PlanManager& planManager, const PricingPlan& pricingPlan, const PriceOffer& priceOffer, Data data);

    void setNextBill(const BillingInfo& next);

    std::unique_ptr<BillingInfo> getNextBill();

    PricingPlan getPricingPlan();

    PriceOffer getPriceOffer();
private:
    void verifyData(const Data& data);

    virtual const std::string buildNodeLabel(ContentGroups &content) override
    {
        return "Billing Info";
    }
};

using BillingInfoData = BillingInfo::Data;

}