#pragma once

#include <string_view>

#include <eosio/eosio.hpp>
#include <eosio/name.hpp>
#include <eosio/contract.hpp>
#include <eosio/crypto.hpp>
#include <eosio/multi_index.hpp>
#include <eosio/singleton.hpp>
#include <eosio/action.hpp>

#include <document_graph/document_graph.hpp>
#include <document_graph/util.hpp>

#include <common.hpp>
#include <period.hpp>

using eosio::checksum256;
using eosio::multi_index;
using eosio::name;

namespace hypha
{
   class Assignment;

   CONTRACT dao : public eosio::contract
   {
   public:
      using eosio::contract::contract;

      DECLARE_DOCUMENT_GRAPH(dao)

      //Might use it to optimize dao lookups instead of hashing the cgs
      // TABLE Dao
      // {
      //   uint64_t id;
      //   name name;
      //   checksum256 hash;

      //   uint64_t primary_key() const { return id; }
      //   uint64_t by_name() const { return name.value; }
      // };

      // typedef multi_index<name("daos"), Dao,
      //                     eosio::indexed_by<name("byname"), eosio::const_mem_fun<Dao, uint64_t, &Dao::by_name>>>
      //         dao_table;

      struct [[eosio::table, eosio::contract("dao")]] Payment
      {
         uint64_t payment_id;
         eosio::time_point payment_date;
         uint64_t period_id = 0;
         uint64_t assignment_id = -1;
         name recipient;
         asset amount;
         string memo;

         uint64_t primary_key() const { return payment_id; }
         uint64_t by_period() const { return period_id; }
         uint64_t by_recipient() const { return recipient.value; }
         uint64_t by_assignment() const { return assignment_id; }
      };
      typedef multi_index<name("payments"), Payment,
                          eosio::indexed_by<name("byperiod"), eosio::const_mem_fun<Payment, uint64_t, &Payment::by_period>>,
                          eosio::indexed_by<name("byrecipient"), eosio::const_mem_fun<Payment, uint64_t, &Payment::by_recipient>>,
                          eosio::indexed_by<name("byassignment"), eosio::const_mem_fun<Payment, uint64_t, &Payment::by_assignment>>>
          payment_table;

      ACTION erasepymts(const eosio::name &scope, const uint64_t &starting_id, const uint64_t &batch_size, int senderId)
      {
         require_auth(get_self());
         int counter = 0;

         payment_table o_t(get_self(), scope.value);
         auto o_itr = o_t.find(starting_id);
         while (o_itr != o_t.end() && counter < batch_size)
         {
            o_itr = o_t.erase(o_itr);
            counter++;
         }

         if (o_itr != o_t.end())
         {
            eosio::transaction out{};
            out.actions.emplace_back(
                eosio::permission_level{get_self(), name("active")},
                get_self(), name("erasepymts"),
                std::make_tuple(scope, starting_id + batch_size, batch_size, ++senderId));

            out.delay_sec = 1;
            out.send(senderId, get_self());
         }
      }

      //TODO: REMOVE
      ACTION deletetok(asset asset, name contract) {

        require_auth(get_self());

        eosio::action(
          eosio::permission_level{contract, name("active")},
          contract, 
          name("del"),
          std::make_tuple(asset)
        ).send();
      }

      ACTION clean();
      ACTION fixassigns(std::vector<checksum256>& hashes);
      ACTION propose(const name& dao_name, const name &proposer, const name &proposal_type, ContentGroups &content_groups);
      ACTION vote(const name& voter, const checksum256 &proposal_hash, string &vote, string notes);
      ACTION closedocprop(const checksum256 &proposal_hash);
      //Sets a dho/contract level setting
      ACTION setsetting(const string &key, const Content::FlexValue &value);

      //Sets a dao level setting
      ACTION setsetting(const eosio::name &dao_name, const std::string &key, const Content::FlexValue &value);

      //Removes a dao/contract level setting
      ACTION remsetting(const string &key);

      ACTION addperiod(const eosio::checksum256 &predecessor, const eosio::time_point &start_time, const string &label);
      ACTION genperiods(const name& dao_name, int64_t period_count, int64_t period_duration_sec);
      //ACTION genperiodini(const name& dao_name, int64_t period_count, int64_t period_duration_sec);

      // ACTION claimpay(const eosio::checksum256 &hash);
      // ACTION claimpayper(const eosio::checksum256 &assignment_hash, const eosio::checksum256 &period_hash);
      ACTION claimnextper(const eosio::checksum256 &assignment_hash);
      ACTION proposeextend (const eosio::checksum256 &assignment_hash, const int64_t additional_periods);

      ACTION apply(const eosio::name &applicant, const name& dao_name, const std::string &content);
      ACTION enroll(const eosio::name &enroller, const name& dao_name, const eosio::name &applicant, const std::string &content);

      // ACTION suspend (const eosio::name &proposer, const eosio::checksum256 &hash);

      ACTION setalert(const eosio::name &level, const std::string &content);
      ACTION remalert(const std::string &notes);

      DocumentGraph &getGraph();
      Document getSettingsDocument();
      Document getSettingsDocument(const eosio::name &dao_name);

      template <class T>
      T getSettingOrFail(const std::string &setting)
      {
         auto settings = getSettingsDocument();
         auto content = settings.getContentWrapper().getOrFail(SETTINGS, setting, "setting " + setting + " does not exist");
         return std::get<T>(content->value);
      }

      template <class T>
      T getSettingOrFail(const eosio::name &dao_name, const std::string &setting)
      {
         auto settings = getSettingsDocument(dao_name);
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
      std::optional<T> getSettingOpt(const eosio::name &dao_name, const string &setting)
      {
         auto settings = getSettingsDocument(dao_name);
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
      
      template <class T>
      T getSettingOrDefault(const eosio::name &dao_name, const string &setting, const T &def = T{})
      {
         if (auto content = getSettingOpt<T>(dao_name, setting))
         {
            return *content;
         }

         return def;
      }

      ACTION adjustcmtmnt(name issuer, ContentGroups& adjust_info);
      ACTION adjustdeferr(name issuer, checksum256 assignment_hash, int64_t new_deferred_perc_x100);

      ACTION withdraw(name owner, eosio::checksum256 hash);
      ACTION suspend(name proposer, eosio::checksum256 hash, string reason);

      ACTION createroot(const std::string &notes);
      ACTION createdao(ContentGroups &config);

      ACTION erasedoc(const eosio::checksum256 &hash);
      ACTION killedge(const uint64_t id);
      ACTION newedge(eosio::name & creator, const checksum256 &from_node, const checksum256 &to_node, const name &edge_name);
      ACTION updatedoc(const eosio::checksum256 hash, const name &updater, const string &group, const string &key, const Content::FlexValue &value);
      ACTION fix (const eosio::checksum256 &hash);
      // ACTION nbadge (const name& owner, const ContentGroups& contentGroups);
      // ACTION nbadass(const name& owner, const ContentGroups& contentGroups);
      // ACTION nbadprop (const name& owner, const ContentGroups& contentGroups);

      void setSetting(const eosio::name &dao_name, const string &key, const Content::FlexValue &value);

      void setSetting(const string &key, const Content::FlexValue &value);

      asset getSeedsAmount(const eosio::asset &usd_amount,
                           const eosio::time_point &price_time_point,
                           const float &time_share,
                           const float &deferred_perc);

      void makePayment(const eosio::checksum256 &fromNode, const eosio::name &recipient,
                       const eosio::asset &quantity, const string &memo,
                       const eosio::name &paymentType);

      void modifyCommitment(Assignment& assignment, 
                            int64_t commitment,
                            std::optional<eosio::time_point> fixedStartDate,
                            std::string_view modifier);

   private:
      DocumentGraph m_documentGraph = DocumentGraph(get_self());

      void addPeriod(const eosio::checksum256 &predecessor, const eosio::time_point &start_time, const string &label);
      void genPeriods(const name& dao_name, int64_t period_count, int64_t period_duration_sec);
            
      void removeSetting(const string &key);

      asset getProRatedAsset(ContentWrapper * assignment, const symbol &symbol,
                             const string &key, const float &proration);

      void createTokens(const eosio::asset& voiceToken, 
                        const eosio::asset& rewardToken);

      struct AssetBatch
      {
         eosio::asset hypha = eosio::asset{0, common::S_REWARD};
         eosio::asset voice = eosio::asset{0, common::S_VOICE};
         eosio::asset husd = eosio::asset{0, common::S_PEG};
      };

      eosio::asset applyCoefficient(ContentWrapper & badge, const eosio::asset &base, const std::string &key);
      AssetBatch applyBadgeCoefficients(Period & period, const eosio::name &member, AssetBatch &ab);
      std::vector<Document> getCurrentBadges(Period & period, const eosio::name &member);

      bool isPaused();
   };
} // namespace hypha