#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>

namespace hypha
{
    namespace voice
    {
        struct [[eosio::table]] account {
            asset    balance;
            uint64_t last_decay_period;
            uint64_t primary_key()const { return balance.symbol.code().raw(); }
        };

        struct [[eosio::table]] currency_stats {
            asset    supply;
            asset    max_supply;
            name     issuer;
            uint64_t decay_per_period_x10M;
            uint64_t decay_period;


            uint64_t primary_key()const { return supply.symbol.code().raw(); }
        };

        typedef eosio::multi_index< "accounts"_n, account > accounts;
        typedef eosio::multi_index< "stat"_n, currency_stats > stats;
    }
}

        

        