#pragma once

#include <eosio/eosio.hpp>
#include <eosio/name.hpp>
#include <eosio/contract.hpp>
#include <eosio/crypto.hpp>
#include <eosio/multi_index.hpp>
#include <eosio/singleton.hpp>

#include <document_graph/document_graph.hpp>
#include <document_graph/util.hpp>

#include <common.hpp>

using eosio::checksum256;
using eosio::multi_index;
using eosio::name;

namespace hypha
{
   CONTRACT dao : public eosio::contract
   {
   public:
      using eosio::contract::contract;

      DECLARE_DOCUMENT_GRAPH(dao)

      // configtable is usued to read the Seeds price
      struct [[eosio::table, eosio::contract("dao")]] configtable
      {
         asset seeds_per_usd;
         asset tlos_per_usd;
         asset citizen_limit;
         asset resident_limit;
         asset visitor_limit;
         uint64_t timestamp;
      };
      typedef eosio::singleton<name("config"), configtable> configtables;
      typedef eosio::multi_index<name("config"), configtable> dump_for_config;

      // price history table is used to read the seeds price
      struct [[eosio::table, eosio::contract("dao")]] price_history_table
      {
         uint64_t id;
         asset seeds_usd;
         eosio::time_point date;

         uint64_t primary_key() const { return id; }
      };
      typedef eosio::multi_index<name("pricehistory"), price_history_table> price_history_tables;

      struct [[eosio::table, eosio::contract("dao")]] PeriodRecord 
        {
            // table columns
            uint64_t id;
            eosio::time_point start_time;
            eosio::time_point end_time;
            std::string label;

            uint64_t primary_key() const { return id; }
            // EOSLIB_SERIALIZE(Period, (id)(start_time)(end_time)(label))
        };
        
        typedef eosio::multi_index<eosio::name("periods"), PeriodRecord> period_table;

      ACTION propose(const name &proposer, const name &proposal_type, ContentGroups &content_groups);
      ACTION closedocprop(const checksum256 &proposal_hash);
      ACTION setsetting(const string &key, const Content::FlexValue& value);
      ACTION remsetting(const string& key);
      ACTION addperiod(const eosio::time_point &start_time, const eosio::time_point &end_time, const string &label);

      ACTION apply(const eosio::name &applicant, const std::string &content);
      ACTION enroll(const eosio::name &enroller, const eosio::name &applicant, const std::string &content);

      DocumentGraph getGraph ();
      Document getSettingsDocument();

      template <class T>
      T getSettingOrFail(const std::string &setting)
      {
         auto settings = getSettingsDocument();
         auto content = settings.getContentWrapper().getOrFail(common::SETTINGS, setting, "setting " + setting + " does not exist");
         return std::get<T>(content->value);
      }

      template <class T>
      std::optional<T> getSettingOpt(const string &setting)
      {
         auto settings = getSettingsDocument();
         auto [idx, content] = settings.getContentWrapper().get(common::SETTINGS, setting);
         if (auto p = std::get_if<T>(&content->value))
         {
            return *p;
         }

         return {};
      }

      template <class T>
      T getSettingOrDefault(const string &setting, const T &def = T{})
      {
         if (auto content = getSettingOpt<T>(setting))
         {
            return *content;
         }

         return def;
      }

      // ADMIN/SETUP only
      ACTION createroot (const std::string &notes);
      void set_setting(const string &key, const Content::FlexValue &value);

   private:
      DocumentGraph m_documentGraph = DocumentGraph(get_self());
      
      void remove_setting(const string &key);

      bool is_paused();
      
   };
} // namespace hypha