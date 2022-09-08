#pragma once

#include <typed_document.hpp>
#include <macros.hpp>

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
    DECLARE_DOCUMENT(
        Data,
        PROPERTY(treasurer, eosio::name, Treasurer)
    )
public:
    Attestation(dao& dao, uint64_t id);

    Attestation(dao& dao, uint64_t paymentID, Data data);

    virtual const std::string buildNodeLabel(ContentGroups &content) override
    {
        return "Attestation";
    }
};

using AttestationData = Attestation::Data;

}