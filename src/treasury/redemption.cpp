#include "treasury/redemption.hpp"
#include "treasury/common.hpp"

#include <algorithm>
#include <iterator>

#include <dao.hpp>
#include <document_graph/edge.hpp>

#include <treasury/payment.hpp>
#include <treasury/treasury.hpp>

#ifdef USE_TREASURY

namespace hypha::treasury {

using namespace common;

Redemption::Redemption(dao& dao, uint64_t id)
    : TypedDocument(dao, id, types::REDEMPTION)
{
    
}

Redemption::Redemption(dao& dao, uint64_t treasuryID, Data data) 
    : TypedDocument(dao, types::REDEMPTION)
{
    EOS_CHECK(
        data.amount_requested.amount > 0,
        "Amount requested must be greater than 0"
    )

    auto cgs = convert(std::move(data));

    initializeDocument(dao, cgs);

    Edge(
        dao.get_self(),
        dao.get_self(),
        treasuryID,
        getId(),
        links::REDEMPTION
    );
}

std::vector<TrsyPayment> Redemption::getPayments()
{
    std::vector<TrsyPayment> payments;

    auto paymentEdges = getDao()
                        .getGraph()
                        .getEdgesFrom(getId(), links::PAID_BY);

    payments.reserve(paymentEdges.size());

    std::transform(paymentEdges.begin(), 
                   paymentEdges.end(), 
                   std::back_inserter(payments),
                   [this](Edge& payEdge){ 
                       return TrsyPayment(getDao(), payEdge.getToNode()); 
                   });

    return payments;
}

Treasury Redemption::getTreasury()
{
    auto redemptionEdge = Edge::getTo(getDao().get_self(), getId(), links::REDEMPTION);
    return Treasury(getDao(), redemptionEdge.getFromNode());
}

} // namespace treasury

#endif