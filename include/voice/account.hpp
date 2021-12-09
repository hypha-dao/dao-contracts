#pragma once
#include <eosio/eosio.hpp>

namespace hypha {
    using eosio::asset;
    using eosio::name;
    using eosio::symbol_code;

    struct [[eosio::table("account"), eosio::contract("voice.hypha")]] account {
        uint64_t id;
        name     tenant;
        asset    balance;
        uint64_t last_decay_period;

        static uint128_t build_key(const name& tenant, const symbol_code& currency) {
            return ((uint128_t)tenant.value << 64) | currency.raw();
        }

        uint64_t primary_key() const {
            return id;
        }

        uint128_t by_tenant_and_code() const {
            return build_key(tenant, balance.symbol.code());
        }
    };

    using accounts_by_key = eosio::indexed_by<
        "bykey"_n,
        eosio::const_mem_fun<account, uint128_t, &account::by_tenant_and_code>
    >;
    using accounts = eosio::multi_index<"accounts"_n, account, accounts_by_key>;
}
