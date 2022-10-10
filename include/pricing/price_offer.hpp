#pragma once

#include <typed_document.hpp>
#include <macros.hpp>

namespace hypha::pricing {

class PriceOffer : public TypedDocument 
{
    DECLARE_DOCUMENT(
        Data,
        PROPERTY(period_count, int64_t, PeriodCount),
        PROPERTY(discount_perc_x10000, int64_t, DiscountPercentage),
        PROPERTY(tag, std::string, Tag)
    )
public:
    PriceOffer(dao& dao, uint64_t id);
    PriceOffer(dao& dao, Data data);
private:
    virtual const std::string buildNodeLabel(ContentGroups &content) override
    {
        return "Price Offer";
    }
};

using PriceOfferData = PriceOffer::Data;

}