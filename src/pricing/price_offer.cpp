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

} // namespace hypha::pricing
