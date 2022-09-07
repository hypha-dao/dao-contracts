#include "treasury/redemption.hpp"
#include "treasury/common.hpp"

namespace hypha::treasury {

using namespace common;

Redemption::Redemption(dao& dao, uint64_t id)
    : TypedDocument(dao, id, types::REDEMPTION)
{
    
}

} // namespace treasury
