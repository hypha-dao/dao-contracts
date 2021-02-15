#pragma once
#include <eosio/name.hpp>
#include <eosio/crypto.hpp>
#include <eosio/multi_index.hpp>
#include <eosio/singleton.hpp>

// #include <dao.hpp>
#include <document_graph/document.hpp>

using eosio::asset;
using eosio::multi_index;
using eosio::name;
using eosio::singleton;
using eosio::time_point;
using std::map;

namespace hypha
{
    class dao;

    class Migration
    {
    public:
        Migration(dao *dao);

        typedef std::variant<name, string, asset, time_point, uint64_t> flexvalue1;

        struct [[eosio::table, eosio::contract("dao")]] Object
        {
            uint64_t id;

            // core maps
            map<string, name> names;
            map<string, string> strings;
            map<string, asset> assets;
            map<string, time_point> time_points;
            map<string, uint64_t> ints;
            map<string, eosio::transaction> trxs;
            map<string, float> floats;
            uint64_t primary_key() const { return id; }

            // indexes
            uint64_t by_owner() const { return names.at("owner").value; }
            uint64_t by_type() const { return names.at("type").value; }
            uint64_t by_fk() const { return ints.at("fk"); }

            // timestamps
            time_point created_date = eosio::current_time_point();
            time_point updated_date = eosio::current_time_point();
            uint64_t by_created() const { return created_date.sec_since_epoch(); }
            uint64_t by_updated() const { return updated_date.sec_since_epoch(); }
        };
        typedef eosio::multi_index<name("objects"), Object,                                                                           // index 1
                                   eosio::indexed_by<name("bycreated"), eosio::const_mem_fun<Object, uint64_t, &Object::by_created>>, // index 2
                                   eosio::indexed_by<name("byupdated"), eosio::const_mem_fun<Object, uint64_t, &Object::by_updated>>, // 3
                                   eosio::indexed_by<name("byowner"), eosio::const_mem_fun<Object, uint64_t, &Object::by_owner>>,     // 4
                                   eosio::indexed_by<name("bytype"), eosio::const_mem_fun<Object, uint64_t, &Object::by_type>>,       // 5
                                   eosio::indexed_by<name("byfk"), eosio::const_mem_fun<Object, uint64_t, &Object::by_fk>>            // 6
                                   >
            object_table;

        struct [[eosio::table, eosio::contract("dao")]] Config
        {
            // required configurations:
            // names : telos_decide_contract, hypha_token_contract, seeds_token_contract, last_ballot_id
            // ints  : voting_duration_sec
            map<string, name> names;
            map<string, string> strings;
            map<string, asset> assets;
            map<string, time_point> time_points;
            map<string, uint64_t> ints;
            map<string, eosio::transaction> trxs;
            map<string, float> floats;
        };
        typedef singleton<name("config"), Config> config_table;
        typedef multi_index<name("config"), Config> config_table_placeholder;

        struct [[eosio::table, eosio::contract("dao")]] MemberRecord
        {
            name member;
            uint64_t primary_key() const { return member.value; }
        };
        typedef multi_index<name("members"), MemberRecord> member_table;

        struct [[eosio::table, eosio::contract("dao")]] Applicant
        {
            name applicant;
            std::string content;

            eosio::time_point created_date = eosio::current_time_point();
            eosio::time_point updated_date = eosio::current_time_point();

            uint64_t primary_key() const { return applicant.value; }
        };
        typedef multi_index<name("applicants"), Applicant> applicant_table;

        // scoped by scope from object_table
        struct [[eosio::table, eosio::contract("dao")]] XReference
        {
            uint64_t id;
            eosio::checksum256 hash;

            uint64_t primary_key() const { return id; }
            eosio::checksum256 by_hash() const { return hash; }
        };
        typedef eosio::multi_index<eosio::name("xref"), XReference,
                                   eosio::indexed_by<eosio::name("byhash"),
                                                     eosio::const_mem_fun<XReference,
                                                                          eosio::checksum256, &XReference::by_hash>>>
            XReferenceTable;



        struct [[eosio::table, eosio::contract("dao")]] AssignmentPayout
        {
            uint64_t ass_payment_id;
            uint64_t assignment_id;
            name recipient;
            uint64_t period_id;
            std::vector<asset> payments;
            time_point payment_date;

            uint64_t primary_key() const { return ass_payment_id; }
            uint64_t by_assignment() const { return assignment_id; }
            uint64_t by_period() const { return period_id; }
            uint64_t by_recipient() const { return recipient.value; }
        };
        typedef multi_index<name("asspayouts"), AssignmentPayout,
                            eosio::indexed_by<name("byassignment"), eosio::const_mem_fun<AssignmentPayout, uint64_t, &AssignmentPayout::by_assignment>>,
                            eosio::indexed_by<name("byperiod"), eosio::const_mem_fun<AssignmentPayout, uint64_t, &AssignmentPayout::by_period>>,
                            eosio::indexed_by<name("byrecipient"), eosio::const_mem_fun<AssignmentPayout, uint64_t, &AssignmentPayout::by_recipient>>>
            asspay_table;

        struct [[eosio::table, eosio::contract("dao")]] Debug
        {
            uint64_t debug_id;
            string notes;
            time_point created_date = eosio::current_time_point();
            uint64_t primary_key() const { return debug_id; }
        };
        typedef multi_index<name("debugs"), Debug> debug_table;

        
        // void erase_debugs();


        struct [[eosio::table, eosio::contract("dao")]] PeriodRecord
        {
            uint64_t period_id;
            time_point start_date;
            time_point end_date;
            string phase;
            // string label;
            // string readable;

            uint64_t primary_key() const { return period_id; }
        };
        typedef multi_index<name("periods"), PeriodRecord> period_table;

        struct [[eosio::table, eosio::contract("dao")]] Payment
        {
            uint64_t payment_id;
            time_point payment_date;
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

        void migrateConfig();
        int defSetSetting(const string &key, const Content::FlexValue &value, int senderId);
        eosio::checksum256 getAccountHash (const eosio::name &account) ;
        void migrateRole(const uint64_t &roleId);
        void migrateAssignment(const uint64_t &assignmentId);
        void migrateMember(const eosio::name &memberName);
        void migratePeriod(const uint64_t &id);
        void migrateAssPayout (const uint64_t &ass_payout_id);
        void migratePayout(const uint64_t &payoutId);
        void migrateProposal(const uint64_t &proposalId);
        ContentGroups newMemo(const std::string memo);

        void newXRef(eosio::checksum256 hash, eosio::name scope, uint64_t id);
        void addLegacyObject(const uint64_t &id,
                       const name &scope,
                       map<string, name> names,
                       map<string, string> strings,
                       map<string, asset> assets,
                       map<string, time_point> time_points,
                       map<string, uint64_t> ints, 
                       time_point created_date, 
                       time_point updated_date);

        void addLegacyMember(const eosio::name &memberName);
        void addLegacyPeriod(const uint64_t &id,
                              const time_point &start_date,
                              const time_point &end_date,
                              const string &phase,
                              const string &readable,
                              const string &label);

        void addLegacyAssPayout(const uint64_t &ass_payment_id,
                                 const uint64_t &assignment_id,
                                 const name &recipient,
                                 uint64_t period_id,
                                 std::vector<eosio::asset> payments,
                                 time_point payment_date);

        void migrateApplicant (const eosio::name &applicant);
        void addLegacyApplicant(const eosio::name &applicant, const std::string content);
        void fixAssProp(const eosio::checksum256 &hash);
        void createBadge (const name& owner, const ContentGroups& contentGroups);
        void createBadgeAssignment (const name& owner, const ContentGroups& contentGroups);
        void createBadgeAssignmentProposal (const name& owner, const ContentGroups& contentGroups);

        void migratedOldDocs ();
        void reset4test();
        void eraseGraph();
        void eraseAssPayouts();
        void erasePeriods();
        void eraseXRefs(const eosio::name &scope);
        // void eraseAll(bool );
        void eraseAllObjects(const eosio::name &scope);

        // void getObject (eosio::checksum256 hash);
        eosio::checksum256 getXRef(eosio::name scope, uint64_t id);

        dao *m_dao;

    private:
        ContentGroups newContentGroups(const uint64_t id,
                                       const name scope,
                                       const time_point createdDate,
                                       const map<string, name> names,
                                       const map<string, string> strings,
                                       const map<string, asset> assets,
                                       const map<string, time_point> time_points,
                                       const map<string, uint64_t> ints);
    };
} // namespace hypha