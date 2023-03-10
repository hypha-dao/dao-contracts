#pragma once

#include <typed_document.hpp>
#include <macros.hpp>

/**
 * @brief Balance document representation. A 'Balance' document stores the amount of redeemable 'cash equivalent tokens'
 * of a member in a specific DAO
 * 
 * @dgnode Balance
 * @connection Balance -> owner -> Member
 * @connection Member -> redeembal -> Balance
 */

#ifdef USE_TREASURY

namespace hypha::treasury {

class Balance : public TypedDocument
{
    DECLARE_DOCUMENT(
        Data,
        PROPERTY(quantity, eosio::asset, Quantity, USE_GETSET),
        PROPERTY(dao, int64_t, DaoID, USE_GETSET)
    )
public:
    Balance(dao& dao, uint64_t id);

    Balance(dao& dao, uint64_t owner, Data data);

    void add(const asset& quantity);

    void substract(const asset& quantity);

    static Balance getOrCreate(dao& dao, uint64_t daoID, uint64_t owner);

    virtual const std::string buildNodeLabel(ContentGroups &content) override
    {
        return "Balance";
    }
};

using BalanceData = Balance::Data;

}

#endif