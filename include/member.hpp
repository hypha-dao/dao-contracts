#pragma once
#include <eosio/name.hpp>
#include <eosio/crypto.hpp>

#include <document_graph/document.hpp>

namespace hypha
{
    class Member
    {
    public:
        Member(const eosio::name contract, const eosio::name &creator, const eosio::name &member);

        static const bool isMember(const eosio::name &rootNode, const eosio::name &member);
        static Document getOrNew (eosio::name contract, const eosio::name &creator, const eosio::name &member);
        static const eosio::checksum256 getHash(const eosio::name &member);

        void apply (const eosio::checksum256& applyTo, const std::string content);
        void enroll (const eosio::name &enroller, const std::string &content);

        const eosio::name m_contract;
        const eosio::name m_member;
        Document m_document; 
    };
} // namespace hypha