#pragma once

#include <typed_document.hpp>
#include <macros.hpp>

/**
 * @brief PricingPlan document representation. A 'PricingPlan' document stores flags enabled for a given 
 * pricing plan, it also stores it's price and it's linked to existing price offer documents.
 * 
 * @dgnode PricingPlan
 * @connection DHO -> pricingplan -> PricingPlan
 * @connection PricingPlan -> priceoffer -> PriceOffer
 * @connection BillingInfo -> pricingplan -> PricingPlan
 */

namespace hypha::pricing {

class PriceOffer;

class PricingPlan : public TypedDocument
{
    DECLARE_DOCUMENT(
        Data,
        PROPERTY(name, std::string, Name, USE_GETSET),
        PROPERTY(price, eosio::asset, Price, USE_GETSET),
        PROPERTY(reactivation_period_sec, int64_t, ReactivationPeriod, USE_GETSET),
        PROPERTY(max_member_count, int64_t, MaxMemberCount, USE_GETSET),
        PROPERTY(discount_perc_x10000, int64_t, DiscountPercentage, USE_GETSET)
    )
public:
    PricingPlan(dao& dao, uint64_t id);
    PricingPlan(dao& dao, Data pricingData);

    void addOffer(const PriceOffer& offer);
    void removeOffer(const PriceOffer& offer);

    std::vector<PriceOffer> getOffers();

    bool hasOffer(uint64_t offerID);

    void updateData(Data data);

private:
    void verifyData(const Data& data);
    
    virtual const std::string buildNodeLabel(ContentGroups &content) override
    {
        return "Pricing Plan";
    }
};

using PricingPlanData = PricingPlan::Data;

}