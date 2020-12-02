#pragma once
#include <eosio/name.hpp>
#include <eosio/crypto.hpp>

#include <document_graph/document.hpp>

namespace hypha
{
    class Member
    {
    public:
        Member(eosio::name member);

        static Document getOrNew (const eosio::name& contract, const eosio::name &creator, const eosio::name &member);
        static const eosio::checksum256 getHash(const eosio::name &member);
        static const bool isMember(const eosio::name &rootNode, const eosio::name &member);

        eosio::name m_member;
    };
} // namespace hypha