#pragma once

#include <typed_document.hpp>

/**
 * @brief TrsyPayment document represntation. Every TrsyPayment is linked to a 'cash equivalent token'
 * redemption in the treasury. Every TrsyPayment document refeers to a single Redemption document,
 * but it could be for a lesser quantity that the one requested in the redemption (partial payment i.e.). 
 * TrsyPayment documents specify how much is going to be paid or was paid (if confirmed) to an specific Redemption
 * 
 * @dgnode TrsyPayment
 * @connection TrsyPayment -> pays -> Redemption
 * @connection Redemption -> paidby -> TrsyPayment
 * @connection Treasury -> payment -> TrsyPayment
 * @connection TrsyPayment -> attestation -> Attestation
 */

namespace hypha::treasury {

class Payment : public TypedDocument {
public:
    Payment(dao& dao, uint64_t id);
};

}