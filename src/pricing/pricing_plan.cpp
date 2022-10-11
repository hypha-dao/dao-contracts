#include "pricing/pricing_plan.hpp"
#include "pricing/common.hpp"
#include "pricing/price_offer.hpp"

#include <algorithm>

#include <dao.hpp>
#include <document_graph/edge.hpp>

namespace hypha::pricing {

using namespace common;

PricingPlan::PricingPlan(dao& dao, uint64_t id)
    : TypedDocument(dao, id, types::PRICING_PLAN)
{}

PricingPlan::PricingPlan(dao& dao, Data pricingData)
    : TypedDocument(dao, types::PRICING_PLAN)
{
    //TODO: Verify data (?)
    //TODO: Check special cases with links

    auto cgs = convert(std::move(pricingData));

    initializeDocument(dao, cgs);

    Edge(
        dao.get_self(),
        dao.get_self(),
        dao.getRootID(),
        getId(),
        links::PRICING_PLAN
    );
}

void PricingPlan::addOffer(const PriceOffer& offer)
{
    EOS_CHECK(
        !Edge::exists(getDao().get_self(), getId(), offer.getId(), links::PRICE_OFFER),
        to_str("The offer with id: ", offer.getId(), " is already linked to the pricing plan.")
    )

    Edge(
        getDao().get_self(), 
        getDao().get_self(),
        getId(),
        offer.getId(),
        links::PRICE_OFFER
    );
}

void PricingPlan::removeOffer(const PriceOffer& offer)
{
    EOS_CHECK(
        Edge::exists(getDao().get_self(), getId(), offer.getId(), links::PRICE_OFFER),
        to_str("The offer with id: ", offer.getId(), " is not linked to the pricing plan.")
    )

    Edge::get(
        getDao().get_self(),
        getId(),
        offer.getId(),
        links::PRICE_OFFER
    ).erase();
}

bool PricingPlan::hasOffer(uint64_t offerID)
{
    return Edge::exists(getDao().get_self(), getId(), offerID, links::PRICE_OFFER);
}

std::vector<PriceOffer> PricingPlan::getOffers()
{
    std::vector<PriceOffer> offers;

    auto offerEdges = getDao()
                      .getGraph()
                      .getEdgesFrom(getId(), links::PRICE_OFFER);


    offers.reserve(offerEdges.size());

    auto toPriceOffer = [this](Edge& offerID) { return PriceOffer(getDao(), offerID.getToNode()); };

    std::transform(offerEdges.begin(), offerEdges.end(), std::back_inserter(offers), toPriceOffer);

    return offers;
}

}