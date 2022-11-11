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
        PROPERTY(start_date, eosio::time_point, StartDate, USE_GETSET),
        PROPERTY(expiration_date, eosio::time_point, ExpirationDate, USE_GETSET),
        PROPERTY(end_date, eosio::time_point, EndDate, USE_GETSET),
        PROPERTY(billing_day, int64_t, BillingDay, NO_USE_GETSET),
        PROPERTY(period_count, int64_t, PeriodCount, NO_USE_GETSET),
        PROPERTY(discount_perc_x10000, int64_t, DiscountPercentage, NO_USE_GETSET),
        PROPERTY(offer_discount_perc_x10000, int64_t, OfferDiscountPercentage, NO_USE_GETSET),
        PROPERTY(plan_name, std::string, PlanName, NO_USE_GETSET),
        PROPERTY(plan_price, eosio::asset, PlanPrice, NO_USE_GETSET),
        PROPERTY(total_paid, eosio::asset, TotalPaid, USE_GETSET),
        PROPERTY(is_infinite, int64_t, IsInfinite, USE_GETSET)
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