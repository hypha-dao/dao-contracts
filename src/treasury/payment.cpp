#include "treasury/payment.hpp"
#include "treasury/common.hpp"

namespace hypha::treasury {

using namespace common;

Payment::Payment(dao& dao, uint64_t id)
    : TypedDocument(dao, id, types::TREASURY_PAYMENT)
{
    
}

} // namespace treasury
