#pragma once

#include <typed_document.hpp>

/**
 * @brief Attestation document representation. An 'Attestation' document represents an attestation
 * commited on a specific 'TrsyPayment'
 * 
 * @dgnode Attestation
 * @connection Attestation -> attestedby -> Member
 * @connection Member -> attested -> Attestation
 * @connection TrsyPayment -> attestation -> Attestation
 */

namespace hypha::treasury {

class Attestation : public TypedDocument
{
public:
    Attestation(dao& dao, uint64_t id);
};

}