#include "treasury/redemption.hpp"
#include "treasury/common.hpp"

#include <dao.hpp>
#include <document_graph/edge.hpp>

namespace hypha::treasury {

using namespace common;

Redemption::Redemption(dao& dao, uint64_t id)
    : TypedDocument(dao, id, types::REDEMPTION)
{
    
}

Redemption::Redemption(dao& dao, uint64_t treasuryID, Data data) 
    : TypedDocument(dao, types::REDEMPTION)
{
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

} // namespace treasury
