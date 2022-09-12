#include "treasury/attestation.hpp"
#include "treasury/common.hpp"

#include <dao.hpp>
#include <document_graph/edge.hpp>

namespace hypha::treasury {

using namespace common;

Attestation::Attestation(dao& dao, uint64_t id)
    : TypedDocument(dao, id, types::ATTESTATION)
{
    
}

Attestation::Attestation(dao& dao, uint64_t paymentID, Data data)
    : TypedDocument(dao, types::ATTESTATION)
{
    auto cgs = convert(data);

    initializeDocument(dao, cgs);

    Edge(
        dao.get_self(),
        dao.get_self(),
        paymentID,
        getId(),
        links::ATTESTATION
    );

    auto treasurerID = dao.getMemberID(getTreasurer());

    Edge(
        dao.get_self(),
        dao.get_self(),
        treasurerID,
        getId(),
        links::ATTESTED
    );

    Edge(
        dao.get_self(),
        dao.get_self(),
        getId(),
        treasurerID,
        links::ATTESTED_BY
    );
}

void Attestation::remove()
{
    getDao().getGraph().eraseDocument(getId(), true);
}

} // namespace treasury
