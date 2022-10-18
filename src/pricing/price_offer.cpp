#include "pricing/price_offer.hpp"
#include "pricing/common.hpp"

#include <dao.hpp>

namespace hypha::pricing {

using namespace common;

PriceOffer::PriceOffer(dao& dao, uint64_t id)
    : TypedDocument(dao, id, types::PRICE_OFFER)
{}

PriceOffer::PriceOffer(dao& dao, Data data)
    : TypedDocument(dao, types::PRICE_OFFER)
{
    
    auto cgs = convert(std::move(data));

    initializeDocument(dao, cgs);
}

void PriceOffer::updateData(Data data)
{
    verifyData(data);
    updateDocument(convert(std::move(data)));
}

void PriceOffer::verifyData(const Data& data) 
{
    EOS_CHECK(
        data.discount_perc_x10000 >= 0 && data.discount_perc_x10000 <= 10000,
        "Offer discount percentage has to be in the range [0, 10000]"
    )

    EOS_CHECK(
        data.tag.size() < 50, 
        "Offer tag must be less than 50 chars length"
    )

    EOS_CHECK(
        data.period_count > 0,
        "Offer periods must be a positive amount and greater than 0"
    )
}

} // namespace hypha::pricing
