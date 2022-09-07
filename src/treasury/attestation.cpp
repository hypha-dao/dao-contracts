#include "treasury/attestation.hpp"
#include "treasury/common.hpp"

namespace hypha::treasury {

using namespace common;

Attestation::Attestation(dao& dao, uint64_t id)
    : TypedDocument(dao, id, types::ATTESTATION)
{
    
}

} // namespace treasury
