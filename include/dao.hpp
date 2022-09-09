#pragma once

#include <string_view>
#include <memory>

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
#include <util.hpp>
#include <period.hpp>
#include <settings.hpp>

using eosio::multi_index;
using eosio::name;

namespace hypha
{
   class Assignment;
   class RecurringActivity;
   class Member;
   class TimeShare;

   CONTRACT dao : public eosio::contract
   {
   public:
      using eosio::contract::contract;

      DECLARE_DOCUMENT_GRAPH(dao)

      TABLE NameToID
      {
        uint64_t id;
        name name;
        uint64_t primary_key() const { return name.value; }
        uint64_t by_id() const { return id; }
      };

      typedef multi_index<name("daos"), NameToID,
                          eosio::indexed_by<name("bydocid"),
                          eosio::const_mem_fun<NameToID, uint64_t, &NameToID::by_id>>>
              dao_table;

      typedef multi_index<name("members"), NameToID,
                          eosio::indexed_by<name("bydocid"),
                          eosio::const_mem_fun<NameToID, uint64_t, &NameToID::by_id>>>
              member_table;

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

      ACTION propose(uint64_t dao_id, const name &proposer, const name &proposal_type, ContentGroups &content_groups, bool publish);
      ACTION vote(const name& voter, uint64_t proposal_id, string &vote, const std::optional<string> & notes);
      ACTION closedocprop(uint64_t proposal_id);

      ACTION proposepub(const name &proposer, uint64_t proposal_id);
      ACTION proposerem(const name &proposer, uint64_t proposal_id);
      ACTION proposeupd(const name &proposer, uint64_t proposal_id, ContentGroups &content_groups);

      // comment related
      ACTION cmntmigrate(const uint64_t &dao_id);
      ACTION cmntadd(const name &author, const string content, const uint64_t comment_or_section_id);
      ACTION cmntupd(const string new_content, const uint64_t comment_id);
      ACTION cmntrem(const uint64_t comment_id);

      // reaction related
      ACTION reactadd(const name &user, const name &reaction, const uint64_t document_id);
      ACTION reactrem(const name &user, const uint64_t document_id);

      //Sets a dho/contract level setting
      ACTION setsetting(const string &key, const Content::FlexValue &value, std::optional<std::string> group);

      //Sets a dao level setting
      ACTION setdaosetting(const uint64_t& dao_id, std::map<std::string, Content::FlexValue> kvs, std::optional<std::string> group);
      //ACTION adddaosetting(const uint64_t& dao_id, const std::map<std::string, Content::FlexValue>& kvs, std::optional<std::string> group);

      ACTION remdaosetting(const uint64_t& dao_id, const std::string &key, std::optional<std::string> group);
      ACTION remkvdaoset(const uint64_t& dao_id, const std::string &key, const Content::FlexValue &value, std::optional<std::string> group);

      ACTION addenroller(const uint64_t dao_id, name enroller_account);
      ACTION addadmin(const uint64_t dao_id, name admin_account);
      ACTION remenroller(const uint64_t dao_id, name enroller_account);
      ACTION remadmin(const uint64_t dao_id, name admin_account);

      //Removes a dho/contract level setting
      ACTION remsetting(const string &key);

      ACTION genperiods(uint64_t dao_id, int64_t period_count/*, int64_t period_duration_sec*/);

      ACTION claimnextper(uint64_t assignment_id);
      ACTION simclaimall(name account, uint64_t dao_id, bool only_ids);
      ACTION simclaim(uint64_t assignment_id);
      ACTION proposeextend (uint64_t assignment_id, const int64_t additional_periods);

      ACTION apply(const eosio::name &applicant, uint64_t dao_id, const std::string &content);
      ACTION enroll(const eosio::name &enroller, uint64_t dao_id, const eosio::name &applicant, const std::string &content);
      
      /**
       * @brief Adds/Edits/Removes alerts from the specified root object
       * 
       * @param root_id DAO id for local alerts or DHO id for global alerts
       * @param alerts  
       */
      ACTION modalerts(uint64_t root_id, ContentGroups& alerts);

      ACTION modsalaryband(uint64_t dao_id, ContentGroups& salary_bands);

      /**TODO: Remove */
      //ACTION adddocs(std::vector<Document>& docs);

      /**TODO: Remove */
      ACTION remdoc(uint64_t doc_id)
      {
         eosio::require_auth(get_self());
         Document doc(get_self(), doc_id);
         auto cw = doc.getContentWrapper();
         if (cw.getOrFail(SYSTEM, TYPE)->getAs<name>() == common::DAO) {
            remNameID<dao_table>(cw.getOrFail(DETAILS, DAO_NAME)->getAs<name>());
         }
         m_documentGraph.eraseDocument(doc_id, true);
      }

      ACTION fixassig(uint64_t assignment_id);

      ACTION setclaimenbld(uint64_t dao_id, bool enabled);

      /**TODO: Remove */
      // ACTION remedge(uint64_t from_node, uint64_t to_node, name edge_name)
      // {
      //    eosio::require_auth(get_self());
      //    Edge::get(get_self(), from_node, to_node, edge_name).erase();
      // }

      /**TODO: Remove */
      //ACTION editdoc(uint64_t doc_id, const std::string& group, const std::string& key, const Content::FlexValue &value);

      /**TODO: Remove */
      struct InputEdge {
         eosio::name creator;
         eosio::time_point created_date;
         uint64_t from_node;
         uint64_t to_node;
         name edge_name;
      };

      /**TODO: Remove */
      //ACTION addedge(std::vector<InputEdge>& edges);

      ACTION autoenroll(uint64_t dao_id, const name& enroller, const name& member);
      /**Testenv only
      ACTION deletetok(asset asset, name contract) {

        require_auth(get_self());

        eosio::action(
          eosio::permission_level{contract, name("active")},
          contract,
          name("del"),
          std::make_tuple(asset)
        ).send();
      }


      ACTION approve(uint64_t doc_id)
      {
         eosio::require_auth(get_self());

         Document doc(get_self(), doc_id);
         auto cw = doc.getContentWrapper();
         cw.insertOrReplace(*cw.getGroupOrFail(DETAILS), Content{common::STATE, common::STATE_APPROVED});
         doc.update();
      }

      ACTION reject(uint64_t doc_id)
      {
         eosio::require_auth(get_self());
         Document doc(get_self(), doc_id);
         auto cw = doc.getContentWrapper();
         cw.insertOrReplace(*cw.getGroupOrFail(DETAILS), Content{common::STATE, common::STATE_REJECTED});
         doc.update();
      }
      */

      DocumentGraph &getGraph();
      Settings* getSettingsDocument();

      Settings* getSettingsDocument(uint64_t daoID);

      template <class T>
      const T& getSettingOrFail(const std::string &setting)
      {
         auto settings = getSettingsDocument();
         return settings->getOrFail<T>(setting);
      }

      template <class T>
      std::optional<T> getSettingOpt(const string &setting)
      {
         auto settings = getSettingsDocument();
         return settings->getSettingOpt<T>(setting);
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

      ACTION remmember(uint64_t dao_id, const std::vector<name>& member_names);
      ACTION remapplicant(uint64_t dao_id, const std::vector<name>& applicant_names);

      ACTION adjustcmtmnt(name issuer, ContentGroups& adjust_info);
      ACTION adjustdeferr(name issuer, uint64_t assignment_id, int64_t new_deferred_perc_x100);

      ACTION withdraw(name owner, uint64_t document_id);
      ACTION suspend(name proposer, uint64_t document_id, string reason);

      ACTION createroot(const std::string &notes);
      ACTION createdao(ContentGroups &config);

      ACTION archiverecur(uint64_t document_id);

      ACTION modpayments(const std::vector<uint64_t>& payments, uint64_t dao_id);
      
      //Treasury actions
      ACTION createtrsy(uint64_t dao_id);
      ACTION addtreasurer(uint64_t treasury_id, name treasurer);
      ACTION remtreasurer(uint64_t treasury_id, name treasurer);
      ACTION redeem(uint64_t dao_id, name requestor, const asset& amount);
      ACTION newpayment(uint64_t dao_id, name treasurer, uint64_t redemption_id, const asset& amount, const string& notes);
      ACTION attestpaymnt(name treasurer, uint64_t payment_id, const asset& amount, const std::optional<std::string>& notes);

      void setSetting(const string &key, const Content::FlexValue &value);

      asset getSeedsAmount(const eosio::asset &usd_amount,
                           const eosio::time_point &price_time_point,
                           const float &time_share,
                           const float &deferred_perc);

      void makePayment(Settings* daoSettings, uint64_t fromNode, const eosio::name &recipient,
                       const eosio::asset &quantity, const string &memo,
                       const eosio::name &paymentType,
                       const AssetBatch& daoTokens);

      void modifyCommitment(RecurringActivity& assignment,
                            int64_t commitment,
                            std::optional<eosio::time_point> fixedStartDate,
                            std::string_view modifier);

      uint64_t getMemberID(const name& memberName);

      template<class Table>
      void addNameID(const name& n, uint64_t id)
      {
         Table t(get_self(), get_self().value);

         EOS_CHECK(
            t.find(n.value) == t.end(),
            util::to_str(n, ": entry already existis in table")
         )

         t.emplace(get_self(), [n, id](NameToID& entry) {
            entry.id = id;
            entry.name = n;
         });
      }

      template<class Table>
      void remNameID(const name& n){
         Table t(get_self(), get_self().value);
         
         auto it = t.find(n.value);
         EOS_CHECK(
            it != t.end(),
            util::to_str("Cannot remove entry since it doesn't exits: ", n)
         )

         t.erase(it);
      }

      [[eosio::on_notify("*::transfer")]]
      void ontransfer(const name& from, const name& to, const asset& quantity, const string& memo) {

         auto settings = getSettingsDocument();
         auto pegContract = settings->getOrFail<name>(PEG_TOKEN_CONTRACT);

         if (get_first_receiver() == pegContract &&
             to == get_self() &&
             from != get_self()) {
            //Buying Hypha tokens with HUSD
            if (memo == "buy") {  
               
               EOS_CHECK(
                  quantity.symbol == common::S_HUSD,
                  "Buying HYPHA is only available with HUSD tokens"
               )

               on_husd(from, to, quantity, memo);
            }
            else if (memo == "redeem") {
               onCashTokenTransfer(from, to, quantity, memo);
            }
            else {
               EOS_CHECK(
                  false, 
                  "No available actions, please specify in the memo string [buy|redeem]"
               )
            }
         } 
      }

      using transfer_action = eosio::action_wrapper<name("transfer"), &dao::ontransfer>;

   private:

      AssetBatch calculatePendingClaims(uint64_t assignmentID, const AssetBatch& daoTokens);

      AssetBatch calculatePeriodPayout(Period& period,
                                       const AssetBatch& salary,
                                       const AssetBatch& daoTokens, 
                                       std::optional<TimeShare>& nextTimeShareOpt,
                                       std::optional<TimeShare>& lastUsedTimeShare,
                                       int64_t initTimeShare);

      void on_husd(const name& from, const name& to, const asset& quantity, const string& memo);

      void onCashTokenTransfer(const name& from, const name& to, const asset& quantity, const string& memo);

      void updateDaoURL(name dao, const Content::FlexValue& newURL);

      void changeDecay(Settings* dhoSettings, Settings* daoSettings, uint64_t decayPeriod, uint64_t decayPerPeriod);

      void addDefaultSettings(ContentGroup& settingsGroup, const string& daoTitle);

      template<class Table>
      std::optional<uint64_t> getNameID(const name& n)
      {
         Table t(get_self(), get_self().value);

         if (auto it = t.find(n.value); it != t.end()) {
            return it->id;
         }

         return {};
      }

      uint64_t getRootID();

      std::optional<uint64_t> getDAOID(const name& daoName);

      Member getOrCreateMember(const name& member);

      void checkAdminsAuth(uint64_t dao_id);

      void checkEnrollerAuth(uint64_t dao_id, const name& account);

      DocumentGraph m_documentGraph = DocumentGraph(get_self());

      void genPeriods(uint64_t dao_id, int64_t period_count/*, int64_t period_duration_sec*/);

      asset getProRatedAsset(ContentWrapper * assignment, const symbol &symbol,
                             const string &key, const float &proration);

      void createVoiceToken(const eosio::name& daoName,
                            const eosio::asset& voiceToken,
                            const uint64_t& decayPeriod,
                            const uint64_t& decayPerPeriodx10M);

      void createToken(const std::string& contractType, name issuer, const asset& token);

      eosio::asset applyCoefficient(ContentWrapper & badge, const eosio::asset &base, const std::string &key);
      AssetBatch applyBadgeCoefficients(Period & period, const eosio::name &member, uint64_t dao, AssetBatch &ab);
      std::vector<Document> getCurrentBadges(Period & period, const eosio::name &member, uint64_t dao);

      bool isPaused();

      std::vector<std::unique_ptr<Settings>> m_settingsDocs;
   };
} // namespace hypha
