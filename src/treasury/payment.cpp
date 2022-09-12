#include "treasury/payment.hpp"
#include "treasury/common.hpp"

#include <dao.hpp>
#include <treasury/redemption.hpp>
#include <treasury/treasury.hpp>
#include <treasury/attestation.hpp>
#include <document_graph/edge.hpp>

namespace hypha::treasury {

using namespace common;

TrsyPayment::TrsyPayment(dao& dao, uint64_t id)
    : TypedDocument(dao, id, types::TREASURY_PAYMENT)
{}

TrsyPayment::TrsyPayment(dao& dao, uint64_t treasuryID, uint64_t redemptionID, Data data)
    : TypedDocument(dao, types::TREASURY_PAYMENT)
{
    auto cgs = convert(std::move(data));

    initializeDocument(dao, cgs);

    Edge(
        dao.get_self(),
        dao.get_self(),
        treasuryID,
        getId(),
        links::PAYMENT
    );

    Edge(
        dao.get_self(),
        dao.get_self(),
        redemptionID,
        getId(),
        links::PAID_BY
    );

    Edge(
        dao.get_self(),
        dao.get_self(),
        getId(),
        redemptionID,
        links::PAYS
    );
}

Redemption TrsyPayment::getRedemption()
{
    auto redemptionEdge = Edge::get(getDao().get_self(), getId(), links::PAYS);
    return Redemption(getDao(), redemptionEdge.getToNode());
}

Treasury TrsyPayment::getTreasury()
{
    auto payEdge = Edge::getTo(getDao().get_self(), getId(), links::PAYMENT);
    return Treasury(getDao(), payEdge.getFromNode());
}

std::vector<Attestation> TrsyPayment::getAttestations()
{
    auto attestationEdges = getDao()
                            .getGraph()
                            .getEdgesFrom(getId(), links::ATTESTATION);

    vector<Attestation> attestations;

    for (auto& attEdge : attestationEdges) {
        attestations.emplace_back(getDao(), attEdge.getToNode());
    }

    return attestations;
}

} // namespace treasury
