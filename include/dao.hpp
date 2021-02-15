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

using eosio::const_mem_fun;
using eosio::indexed_by;
using std::vector;
using eosio::time_point;
using eosio::current_time_point;

namespace hypha
{
   CONTRACT dao : public eosio::contract
   {
   public:
      using eosio::contract::contract;

      DECLARE_DOCUMENT_GRAPH(dao)

      // migration only
      struct [[eosio::table, eosio::contract("dao")]] edgeold
      {
         uint64_t id;

         // these three additional indexes allow isolating/querying edges more precisely (less iteration)
         uint64_t from_node_edge_name_index;
         uint64_t from_node_to_node_index;
         uint64_t to_node_edge_name_index;
         uint64_t by_from_node_edge_name_index() const { return from_node_edge_name_index; }
         uint64_t by_from_node_to_node_index() const { return from_node_to_node_index; }
         uint64_t by_to_node_edge_name_index() const { return to_node_edge_name_index; }

         checksum256 from_node;
         checksum256 by_from() const { return from_node; }

         checksum256 to_node;
         checksum256 by_to() const { return to_node; }

         name edge_name;
         uint64_t by_edge_name() const { return edge_name.value; }

         time_point created_date = current_time_point();
         uint64_t by_created() const { return created_date.sec_since_epoch(); }

         name creator;
         uint64_t by_creator() const { return creator.value; }

         uint64_t primary_key() const { return id; }

         EOSLIB_SERIALIZE(edgeold, (id)(from_node_edge_name_index)(from_node_to_node_index)(to_node_edge_name_index)(from_node)(to_node)(edge_name)(created_date)(creator))
      };

      typedef multi_index<name("edgesold"), edgeold,
                          indexed_by<name("fromnode"), const_mem_fun<edgeold, checksum256, &edgeold::by_from>>,
                          indexed_by<name("tonode"), const_mem_fun<edgeold, checksum256, &edgeold::by_to>>,
                          indexed_by<name("edgename"), const_mem_fun<edgeold, uint64_t, &edgeold::by_edge_name>>,
                          indexed_by<name("byfromname"), const_mem_fun<edgeold, uint64_t, &edgeold::by_from_node_edge_name_index>>,
                          indexed_by<name("byfromto"), const_mem_fun<edgeold, uint64_t, &edgeold::by_from_node_to_node_index>>,
                          indexed_by<name("bytoname"), const_mem_fun<edgeold, uint64_t, &edgeold::by_to_node_edge_name_index>>,
                          indexed_by<name("bycreated"), const_mem_fun<edgeold, uint64_t, &edgeold::by_created>>,
                          indexed_by<name("bycreator"), const_mem_fun<edgeold, uint64_t, &edgeold::by_creator>>>
          edge_table_old;

      typedef std::variant<name, string, asset, time_point, int64_t, checksum256> flexvalueold;
       // a single labeled flexvalue
        struct contentold
        {
            string label;
            flexvalueold value;

            EOSLIB_SERIALIZE(contentold, (label)(value))
        };

        typedef vector<contentold> content_groupold;

        struct certificateold
        {
            name certifier;
            string notes;
            time_point certification_date = current_time_point();

            EOSLIB_SERIALIZE(certificateold, (certifier)(notes)(certification_date))
        };


      struct [[eosio::table, eosio::contract("dao")]] documentold
      {
         uint64_t id;
         checksum256 hash;
         name creator;
         vector<content_groupold> content_groups;

         vector<certificateold> certificates;
         uint64_t primary_key() const { return id; }
         uint64_t by_creator() const { return creator.value; }
         checksum256 by_hash() const { return hash; }

         time_point created_date = current_time_point();
         uint64_t by_created() const { return created_date.sec_since_epoch(); }

         EOSLIB_SERIALIZE(documentold, (id)(hash)(creator)(content_groups)(certificates)(created_date))
      };

      typedef multi_index<name("documentsold"), documentold,
                          indexed_by<name("idhash"), const_mem_fun<documentold, checksum256, &documentold::by_hash>>,
                          indexed_by<name("bycreator"), const_mem_fun<documentold, uint64_t, &documentold::by_creator>>,
                          indexed_by<name("bycreated"), const_mem_fun<documentold, uint64_t, &documentold::by_created>>>
          document_table_old;
      // end migration only

      ACTION propose(const name &proposer, const name &proposal_type, ContentGroups &content_groups);
      ACTION closedocprop(const checksum256 &proposal_hash);
      ACTION setsetting(const string &key, const Content::FlexValue &value);
      ACTION remsetting(const string &key);
      ACTION addperiod(const eosio::checksum256 &predecessor, const eosio::time_point &start_time, const string &label);

      // ACTION claimpay(const eosio::checksum256 &hash);
      // ACTION claimpayper(const eosio::checksum256 &assignment_hash, const eosio::checksum256 &period_hash);
      ACTION claimnextper(const eosio::checksum256 &assignment_hash);

      ACTION apply(const eosio::name &applicant, const std::string &content);
      ACTION enroll(const eosio::name &enroller, const eosio::name &applicant, const std::string &content);

      // ACTION suspend (const eosio::name &proposer, const eosio::checksum256 &hash);

      ACTION setalert(const eosio::name &level, const std::string &content);
      ACTION remalert(const std::string &notes);

      // migration only
      ACTION createobj(const uint64_t &id,
                       const name &scope,
                       std::map<string, name> names,
                       std::map<string, string> strings,
                       std::map<string, asset> assets,
                       std::map<string, eosio::time_point> time_points,
                       std::map<string, uint64_t> ints,
                       eosio::time_point created_date,
                       eosio::time_point updated_date);

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

      // migration actions
      ACTION migrate(const eosio::name &scope, const uint64_t &id);
      ACTION migrateconfig(const std::string &notes);
      ACTION createroot(const std::string &notes);
      ACTION migratemem(const eosio::name &member);
      ACTION migrateper(const uint64_t id);
      ACTION migasspay(const uint64_t id);
      ACTION cancel(const uint64_t senderid);

      // test setup actions
      ACTION reset4test(const std::string &notes);
      ACTION erasexfer(const eosio::name &scope);
      ACTION erasepers(const std::string &notes);
      ACTION erasegraph(const std::string &notes);
      ACTION eraseedges(const std::string &notes);
      ACTION eraseedgesb(const uint64_t &batch_size, int senderId);
      ACTION erasedoc(const eosio::checksum256 &hash);
      ACTION eraseobj(const eosio::name &scope, const uint64_t &starting_id);
      // ACTION eraseobjs(const eosio::name &scope);
      ACTION eraseobjs(const eosio::name &scope, const uint64_t &starting_id, const uint64_t &batch_size, int senderId);
      ACTION addmember(const eosio::name &member);
      ACTION addapplicant(const eosio::name &applicant, const std::string content);
      ACTION addlegper(const uint64_t &id,
                       const eosio::time_point &start_date,
                       const eosio::time_point &end_date,
                       const string &phase, const string &readable, const string &label);

      ACTION updatedoc (const eosio::checksum256 hash, const name& updater, const string &group, const string &key, const Content::FlexValue &value);
      ACTION updatexref (const name& scope, const uint64_t &id, const checksum256& hash);
      ACTION fixassprop (const checksum256& hash);
      ACTION nbadge (const name& owner, const ContentGroups& contentGroups);
      ACTION nbadass(const name& owner, const ContentGroups& contentGroups);
      ACTION nbadprop (const name& owner, const ContentGroups& contentGroups);
      ACTION killedge (const uint64_t id);

      ACTION addasspayout(const uint64_t &ass_payment_id,
                          const uint64_t &assignment_id,
                          const name &recipient,
                          uint64_t period_id,
                          std::vector<eosio::asset> payments,
                          eosio::time_point payment_date);

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
      AssetBatch applyBadgeCoefficients(Period & period, const eosio::name &member, AssetBatch &ab);
      std::vector<Document> getCurrentBadges(Period & period, const eosio::name &member);

      bool isPaused();
   };
} // namespace hypha