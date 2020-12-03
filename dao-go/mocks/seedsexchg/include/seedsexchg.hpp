#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/name.hpp>
#include <eosio/singleton.hpp>

using namespace eosio;

CONTRACT seedsexchg : public contract {
   public:
      using contract::contract;

      struct [[eosio::table, eosio::contract("seedsexchg")]] configtable
      {
         asset seeds_per_usd;
         asset tlos_per_usd;
         asset citizen_limit;
         asset resident_limit;
         asset visitor_limit;
         uint64_t timestamp;
      };
      typedef singleton<name("config"), configtable> configtables;
      typedef eosio::multi_index<name("config"), configtable> dump_for_config;

      struct [[eosio::table, eosio::contract("seedsexchg")]] price_history_table {
         uint64_t id;
         asset seeds_usd;
         time_point date;

         uint64_t primary_key()const { return id; }
      };
      typedef eosio::multi_index<name("pricehistory"), price_history_table> price_history_tables;

      ACTION updateconfig( asset seeds_per_usd, asset tlos_per_usd, asset citizen_limit, 
         asset resident_limit, asset visitor_limit );

      ACTION inshistory(uint64_t id, asset seeds_usd, time_point date);
};