#pragma once
#include <eosio/name.hpp>
#include <eosio/crypto.hpp>

#include <document_graph/document.hpp>

namespace hypha
{

    class dao;

    class Member : public Document
    {
    public:
        Member(dao& dao, const eosio::name &creator, const eosio::name &member);
        Member(dao& dao, const eosio::checksum256 &hash);
        Member(dao& dao, uint64_t docID);

        static Member get (dao& dao, const eosio::name &member);

        static const bool isMember(const eosio::name& contract, uint64_t daoID, const eosio::name &member);
        static Member getOrNew (dao& dao, const eosio::name &creator, const eosio::name &member);
        static const eosio::checksum256 calcHash(const eosio::name &member);

        eosio::name getAccount ();
        void apply (uint64_t applyTo, const std::string content);
        void enroll (const eosio::name &enroller, uint64_t appliedTo, const std::string &content);

    private: 
        static ContentGroups defaultContent (const eosio::name &member);
        dao& m_dao;
    };
} // namespace hypha