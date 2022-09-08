#include "treasury/attestation.hpp"
#include "treasury/common.hpp"

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

}

} // namespace treasury
