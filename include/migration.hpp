#pragma once
#include <eosio/name.hpp>
#include <eosio/crypto.hpp>

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
        Migration(const eosio::name contract);

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

        void migrateRole(const uint64_t &roleId);

    private:
        Document newDocument(const uint64_t id,
                             const name scope,
                             const time_point createdDate,
                             const map<string, name> names,
                             const map<string, string> strings,
                             const map<string, asset> assets,
                             const map<string, time_point> time_points,
                             const map<string, uint64_t> ints);

        const eosio::name m_contract;
    };
} // namespace hypha