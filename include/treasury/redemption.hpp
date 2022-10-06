#pragma once

#include <vector>

#include <typed_document.hpp>
#include <macros.hpp>

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

class TrsyPayment;
class Treasury;

class Redemption : public TypedDocument
{
    DECLARE_DOCUMENT(
        Data,
        PROPERTY(requestor, eosio::name, Requestor),
        PROPERTY(amount_requested, eosio::asset, AmountRequested),
        PROPERTY(amount_paid, eosio::asset, AmountPaid)
    )

public:
    Redemption(dao& dao, uint64_t id);

    Redemption(dao& dao, uint64_t treasuryID, Data data);

    virtual const std::string buildNodeLabel(ContentGroups &content) override
    {
        return "Redemption";
    }

    std::vector<TrsyPayment> getPayments();

    Treasury getTreasury();
};

using RedemptionData = Redemption::Data;

}