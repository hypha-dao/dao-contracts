#include "treasury/balance.hpp"
#include "treasury/common.hpp"

namespace hypha::treasury {

using namespace common;

Balance::Balance(dao& dao, uint64_t id)
    : TypedDocument(dao, id, types::BALANCE)
{
    
}

} // namespace treasury
