#pragma once
#include <eosio/name.hpp>
#include <eosio/crypto.hpp>

#include <dao.hpp>
#include <document_graph/document.hpp>

using eosio::asset;
using eosio::name;
using eosio::time_point;
using std::map;

namespace hypha
{
    class Migration
    {
    public:
        Migration(dao &dao);

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
        typedef eosio::singleton<name("config"), Config> config_table;
        typedef multi_index<name("config"), Config> config_table_placeholder;

        struct [[eosio::table, eosio::contract("dao")]] Member
        {
            name member;
            uint64_t primary_key() const { return member.value; }
        };
        typedef multi_index<name("members"), Member> member_table;

        struct [[eosio::table, eosio::contract("dao")]] Applicant
        {
            name applicant;
            std::string content;

            eosio::time_point created_date = eosio::current_time_point();
            eosio::time_point updated_date = eosio::current_time_point();

            uint64_t primary_key() const { return applicant.value; }
        };
        typedef multi_index<name("applicants"), Applicant> applicant_table;

        struct [[eosio::table, eosio::contract("dao")]] XReference
        {
            // table columns
            uint64_t id;
            eosio::name object_scope;
            uint64_t object_id;
            eosio::checksum256 document_hash;

            uint64_t primary_key() const { return id; }
            uint64_t by_scope() const { return object_scope.value; }
            uint64_t by_object() const { return object_id; }
            eosio::checksum256 by_hash() const { return document_hash; }
        };
        typedef eosio::multi_index<eosio::name("xref"), XReference,
                                   eosio::indexed_by<eosio::name("byhash"), eosio::const_mem_fun<XReference, eosio::checksum256, &XReference::by_hash>>,
                                   eosio::indexed_by<eosio::name("byscope"), eosio::const_mem_fun<XReference, uint64_t, &XReference::by_scope>>,
                                   eosio::indexed_by<eosio::name("byobjectid"), eosio::const_mem_fun<XReference, uint64_t, &XReference::by_object>>>
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

        struct [[eosio::table, eosio::contract("dao")]] Period
        {
            uint64_t period_id;
            time_point start_date;
            time_point end_date;
            string phase;

            uint64_t primary_key() const { return period_id; }
        };
        typedef multi_index<name("periods"), Period> period_table;

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

        void migrateConfig ();
        int defSetSetting(const string &key, const Content::FlexValue &value, int senderId);
        void migrateRole(const uint64_t &roleId);
        void migrateMember(const eosio::name &memberName);

        void newObject(const uint64_t &id,
                       const name &scope,
                       map<string, name> names,
                       map<string, string> strings,
                       map<string, asset> assets,
                       map<string, time_point> time_points,
                       map<string, uint64_t> ints);

        void addMemberToTable(const eosio::name &memberName);
        void addApplicant(const eosio::name &applicant, const std::string content);

        void reset4test();
        void eraseAll(bool skipPeriods);
        void eraseAllObjects(const eosio::name &scope);

    private:
        Document newDocument(const uint64_t id,
                             const name scope,
                             const time_point createdDate,
                             const map<string, name> names,
                             const map<string, string> strings,
                             const map<string, asset> assets,
                             const map<string, time_point> time_points,
                             const map<string, uint64_t> ints);

        dao &m_dao;
    };
} // namespace hypha