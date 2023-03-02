#pragma once

#include <eosio/multi_index.hpp>
#include <eosio/asset.hpp>
#include <eosio/name.hpp>

struct [[eosio::table]] account {
    eosio::asset    balance;
    uint64_t primary_key()const { return balance.symbol.code().raw(); }
};

// struct [[eosio::table]] currency_stats {
// eosio::asset    supply;
// eosio::asset    max_supply;
// eosio::name     issuer;

// uint64_t primary_key()const { return supply.symbol.code().raw(); }
// };

typedef eosio::multi_index< "accounts"_n, account > accounts;
//typedef eosio::multi_index< "stat"_n, currency_stats > stats;