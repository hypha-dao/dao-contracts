#include "treasury/redemption.hpp"
#include "treasury/common.hpp"

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
}

} // namespace treasury
