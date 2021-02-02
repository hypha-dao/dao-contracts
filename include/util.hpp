#pragma once
#include <type_traits>
#include <utility>
#include <string>
#include <eosio/crypto.hpp>
#include <eosio/name.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <eosio/multi_index.hpp>

#include <document_graph/content_wrapper.hpp>

namespace hypha
{
    eosio::checksum256 getRoot(const eosio::name &contract);
    ContentGroups getRootContent(const eosio::name &contract);
    eosio::asset adjustAsset(const eosio::asset &originalAsset, const float &adjustment);
    float getSeedsPriceUsd(const eosio::time_point &price_time_point);
    float getSeedsPriceUsd();
    eosio::asset getSeedsAmount(int64_t deferralFactor,
                                const eosio::asset &usd_amount,
                                const eosio::time_point &price_time_point,
                                const float &time_share,
                                const float &deferred_perc);

    // configtable is usued to read the Seeds price
    struct configtable
    {
        eosio::asset seeds_per_usd;
        eosio::asset tlos_per_usd;
        eosio::asset citizen_limit;
        eosio::asset resident_limit;
        eosio::asset visitor_limit;
        uint64_t timestamp;
    };
    typedef eosio::singleton<eosio::name("config"), configtable> configtables;
    typedef eosio::multi_index<eosio::name("config"), configtable> dump_for_config;

    // price history table is used to read the seeds price
    struct price_history_table
    {
        uint64_t id;
        eosio::asset seeds_usd;
        eosio::time_point date;

        uint64_t primary_key() const { return id; }
    };
    typedef eosio::multi_index<eosio::name("pricehistory"), price_history_table> price_history_tables;

    namespace detail {

    template<class T>
    struct supports_to_string
    {
      template<class U>
      static auto can_pass_to_string(const U* arg) -> decltype(std::to_string(*arg), char(0))
      {}

      static std::array<char, 2> can_pass_to_string(...) { }

      static constexpr bool value = (sizeof(can_pass_to_string((T*)0)) == 1);
    };

    template<class T>
    struct supports_call_to_string
    {
      template<class U>
      static auto can_pass_to_string(const U* arg) -> decltype(arg->to_string(), char(0))
      {}

      static std::array<char, 2> can_pass_to_string(...) { }

      static constexpr bool value = (sizeof(can_pass_to_string((T*)0)) == 1);
    };
  
    template<class T>
    std::string to_str_h(T arg)
    {
      if constexpr (supports_to_string<T>::value) {
        return std::to_string(arg);
      }
      else if constexpr (supports_call_to_string<T>::value) {
        return arg.to_string();
      }
      else {
        return arg;
      }
    }

    }

    //Helper function to convert 1+ X type variables to string
    template<class T, class...Args>
    string to_str(T first, Args... others)
    {
      return (detail::to_str_h(first) + ... + detail::to_str_h(others));
    }
} // namespace hypha
