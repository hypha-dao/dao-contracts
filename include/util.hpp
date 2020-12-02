#pragma once
#include <eosio/crypto.hpp>
#include <eosio/name.hpp>

namespace hypha {

    eosio::checksum256 getRoot (const eosio::name& contract);

}
