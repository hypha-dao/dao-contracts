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
#include <period.hpp>

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

      typedef eosio::multi_index<eosio::name("periods"), PeriodRecord> PeriodTable;

      ACTION propose(const name &proposer, const name &proposal_type, ContentGroups &content_groups);
      ACTION closedocprop(const checksum256 &proposal_hash);
      ACTION setsetting(const string &key, const Content::FlexValue &value);
      ACTION remsetting(const string &key);
      ACTION addperiod(const eosio::checksum256 &predecessor, const eosio::time_point &start_time, const string &label);

      ACTION claimpay(const eosio::checksum256 &hash);

      ACTION apply(const eosio::name &applicant, const std::string &content);
      ACTION enroll(const eosio::name &enroller, const eosio::name &applicant, const std::string &content);

      // migration only
      ACTION createobj(const uint64_t &id,
                       const name &scope,
                       std::map<string, name> names,
                       std::map<string, string> strings,
                       std::map<string, asset> assets,
                       std::map<string, eosio::time_point> time_points,
                       std::map<string, uint64_t> ints);

      DocumentGraph &getGraph();
      Document getSettingsDocument();

      template <class T>
      T getSettingOrFail(const std::string &setting)
      {
         auto settings = getSettingsDocument();
         auto content = settings.getContentWrapper().getOrFail(SETTINGS, setting, "setting " + setting + " does not exist");
         return std::get<T>(content->value);
      }

      template <class T>
      std::optional<T> getSettingOpt(const string &setting)
      {
         auto settings = getSettingsDocument();
         auto [idx, content] = settings.getContentWrapper().get(SETTINGS, setting);
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
      ACTION createroot(const std::string &notes);
      void setSetting(const string &key, const Content::FlexValue &value);

      asset getSeedsAmount(const eosio::asset &usd_amount,
                           const eosio::time_point &price_time_point,
                           const float &time_share,
                           const float &deferred_perc);

      void makePayment(const eosio::checksum256 &fromNode, const eosio::name &recipient,
                       const eosio::asset &quantity, const string &memo,
                       const eosio::name &paymentType);

   private:
      DocumentGraph m_documentGraph = DocumentGraph(get_self());

      void removeSetting(const string &key);

      asset getProRatedAsset(ContentWrapper * assignment, const symbol &symbol,
                             const string &key, const float &proration);

      struct AssetBatch
      {
         eosio::asset hypha = eosio::asset{0, common::S_HYPHA};
         eosio::asset d_seeds = eosio::asset{0, common::S_SEEDS};
         eosio::asset seeds = eosio::asset{0, common::S_SEEDS};
         eosio::asset voice = eosio::asset{0, common::S_HVOICE};
         eosio::asset husd = eosio::asset{0, common::S_HUSD};
      };

      eosio::asset applyCoefficient(ContentWrapper & badge, const eosio::asset &base, const std::string &key);
      AssetBatch applyBadgeCoefficients(Period &period, const eosio::name &member, AssetBatch &ab);
      std::vector<Document> getCurrentBadges(Period &period, const eosio::name &member);

      bool isPaused();
   };
} // namespace hypha