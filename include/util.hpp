#pragma once

#include <type_traits>
#include <utility>
#include <cmath>
#include <string>
#include <string_view>
#include <vector>
#include <eosio/crypto.hpp>
#include <eosio/name.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <eosio/multi_index.hpp>

#include <document_graph/content_wrapper.hpp>
#include <document_graph/util.hpp>
#include <settings.hpp>

namespace hypha
{
    using std::vector;
    using std::string_view;

    namespace util {
      //using property_map
    }

    struct AssetBatch
    {
      eosio::asset reward;
      eosio::asset peg;
      eosio::asset voice;
    };

    AssetBatch& operator+=(AssetBatch& self, const AssetBatch& other);

    struct SalaryConfig
    {
      // double annualSalary;
      double periodSalary;
      double rewardToPegRatio;
      double deferredPerc;
      double voiceMultipler = 2.0;
    };

    /**
     * @brief Gets the phase to year ratio using the dao setting variable for the period duration
     * 
     * @param daoSettings 
     * @return Phase to year ratio 
     */
    float getPhaseToYearRatio(Settings* daoSettings);
    float getPhaseToYearRatio(Settings* daoSettings, int64_t periodDuration);

    AssetBatch calculateSalaries(const SalaryConfig& salaryConf, const AssetBatch& tokens);

    ContentGroups getRootContent(const eosio::name &contract);
    ContentGroups getDAOContent(const eosio::name &dao_name);
    eosio::asset adjustAsset(const eosio::asset &originalAsset, const float &adjustment);

    int64_t stringViewToInt(string_view str);

    vector<string_view> splitStringView(string_view str, char delimiter);

    template<typename T>
    class ShowType;

    auto convert = [](auto& x, string_view str) {
        using Type = std::remove_reference_t<decltype(x)>;
        
        if constexpr (std::is_same_v<Type, std::string>) {
          x = std::string(str);
        }
        else if constexpr(std::is_same_v<Type, int64_t>) {
          x = stringViewToInt(str);
        }
        else if constexpr(std::is_same_v<Type, eosio::asset>) {
          auto parts = splitStringView(str, ' ');
          EOS_CHECK(parts.size() == 2, "Unkown asset string " + std::string(str))
          auto amount = parts[0];
          auto amountParts = splitStringView(amount, '.');
          auto prec = amountParts.size() == 2 ? amountParts[1].size() : size_t(0);
          auto sym = parts[1];
          x = eosio::asset(
            std::stoi(
              string{amountParts[0]} + string(amountParts.size() == 2 ? amountParts[1] : string_view{})
            ), 
            eosio::symbol(sym, prec)
          );
        }
        else {
          //Will fail to compile if the type is not valid
          ShowType<Type> y;
        }
    };

    struct Overloaded {
      std::vector<string_view>& strs;
      Overloaded(std::vector<string_view>& strviews) : strs(strviews) {}
      template<class... T>
      void operator()(T&&... args) {
        int n = 0;
        (convert(args, strs[n++]), ...);
      }
    };

    template<class ... T>
    std::tuple<T...> splitStringView(string_view str, char delimiter)
    {
      auto strs = splitStringView(str, delimiter);
      
      EOS_CHECK(
        sizeof...(T) == strs.size(),
        to_str("Mismatch of readed parameters, expected ", sizeof...(T), " but got ", strs.size())
      )
      
      std::tuple<T...> res;
      std::apply(Overloaded(strs), res);

      return res;
    }

    void issueToken(const eosio::name &token_contract,
                    const eosio::name &issuer,
                    const eosio::name &to,
                    const eosio::asset &token_amount,
                    const string &memo);

    void issueTenantToken(const eosio::name &token_contract,
                          const eosio::name &tenant,
                          const eosio::name &issuer,
                          const eosio::name &to,
                          const eosio::asset &token_amount,
                          const string &memo);

    double normalizeToken(const eosio::asset& token);

    eosio::asset denormalizeToken(double amountNormalized, const eosio::asset& token);

    /**
     * @brief Returns the representation of 1 unit in the given token
     */
    int64_t getTokenUnit(const eosio::asset& token);

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

    struct currency_stats {
        eosio::asset    supply;
        eosio::asset    max_supply;
        eosio::name     issuer;
        uint64_t decay_per_period_x10M;
        uint64_t decay_period;

        uint64_t primary_key()const { return supply.symbol.code().raw(); }
    };
    typedef eosio::multi_index< "stat"_n, currency_stats > stats;


    // price history table is used to read the seeds price
    struct price_history_table
    {
        uint64_t id;
        eosio::asset seeds_usd;
        eosio::time_point date;

        uint64_t primary_key() const { return id; }
    };
    typedef eosio::multi_index<eosio::name("pricehistory"), price_history_table> price_history_tables;
} // namespace hypha
