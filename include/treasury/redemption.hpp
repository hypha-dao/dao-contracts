#pragma once

#include <typed_document.hpp>

/**
 * @brief Redemption document representation. A 'Redemption' document is used to create a 'cash equivalent token' 
 * redeem request. Every 'Redemption' might be linked to 1 or more TrsyPayment documents.
 * 
 * @dgnode Redemption
 * @connection Redemption -> paidby -> TrsyPayment
 * @connection TrsyPayment -> pays -> Redemption
 * @connection Treasury -> redemption -> Redemption
 */

namespace hypha::treasury {

class Redemption : public TypedDocument
{
public:
    Redemption(dao& dao, uint64_t id);
};

}