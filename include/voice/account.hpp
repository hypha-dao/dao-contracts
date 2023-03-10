#pragma once
#include <eosio/eosio.hpp>

namespace hypha { namespace voice {
    using eosio::asset;
    using eosio::name;
    using eosio::symbol_code;

    struct [[eosio::table("accounts.v2"), eosio::contract("voice.hypha")]] accountv2 {
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
        eosio::name("bykey"),
        eosio::const_mem_fun<accountv2, uint128_t, &accountv2::by_tenant_and_code>
    >;
    using accounts = eosio::multi_index<"accounts.v2"_n, accountv2, accounts_by_key>;
} }
