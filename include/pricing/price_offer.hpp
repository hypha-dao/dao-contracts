#pragma once

#include <typed_document.hpp>
#include <macros.hpp>

namespace hypha::pricing {

//TODO: Improve validation methods
//IDEA: Add a validator to each PROPERTY()
//which will be called on setX or convert Function
//or manually by a verify function

class PriceOffer : public TypedDocument 
{
    DECLARE_DOCUMENT(
        Data,
        PROPERTY(period_count, int64_t, PeriodCount, USE_GETSET),
        PROPERTY(discount_perc_x10000, int64_t, DiscountPercentage, USE_GETSET),
        PROPERTY(tag, std::string, Tag, USE_GETSET)
    )
public:
    PriceOffer(dao& dao, uint64_t id);
    PriceOffer(dao& dao, Data data);

    void updateData(Data data);
private:
    void verifyData(const Data& data);
    virtual const std::string buildNodeLabel(ContentGroups &content) override
    {
        return "Price Offer";
    }
};

using PriceOfferData = PriceOffer::Data;

}