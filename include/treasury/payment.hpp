#pragma once

#include <typed_document.hpp>

#include <eosio/time.hpp>

#include <macros.hpp>

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

class Redemption;

class TrsyPayment : public TypedDocument {

    DECLARE_DOCUMENT(
        Data,
        PROPERTY(creator, eosio::name, Creator),
        PROPERTY(amount_paid, eosio::asset, AmountPaid),
        PROPERTY(confirmed_date, eosio::time_point, ConfirmedDate),
        PROPERTY(is_confirmed, int64_t, IsConfirmed),
        PROPERTY(notes, string, Notes)
    )
public:
    TrsyPayment(dao& dao, uint64_t id);

    TrsyPayment(dao& dao, uint64_t treasuryID, uint64_t redemptionID, Data data);

    virtual const std::string buildNodeLabel(ContentGroups &content) override
    {
        return "Treasury Payment";
    }

    Redemption getRedemption();
};

using TrsyPaymentData = TrsyPayment::Data;

}