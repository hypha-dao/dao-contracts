#pragma once

#include <typed_document.hpp>

/**
 * @brief Balance document representation. A 'Balance' document stores the amount of redeemable 'cash equivalent tokens'
 * of a member in a specific DAO
 * 
 * @dgnode Balance
 * @connection Balance -> owner -> Member
 * @connection Member -> redeembal -> Balance
 */

namespace hypha::treasury {

class Balance : public TypedDocument
{
public:
    Balance(dao& dao, uint64_t id);
};

}
