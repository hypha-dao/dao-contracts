#pragma once 

#include <eosio/name.hpp>
#include <eosio/symbol.hpp>

#include "payer.hpp"

namespace hypha {

    class dao;

    class PayerFactory
    {
    public:
        static Payer* Factory(dao &dao, const eosio::symbol &symbol, const eosio::name &paymentType, const AssetBatch& daoTokens);
    };
}


