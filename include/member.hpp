#pragma once
#include <eosio/name.hpp>
#include <eosio/crypto.hpp>

#include <document_graph/document.hpp>

namespace hypha
{
    class Member : public Document
    {
    public:
        Member(const eosio::name contract, const eosio::name &creator, const eosio::name &member);
        Member(const eosio::name contract, const eosio::checksum256 &hash);

        static Member get (const eosio::name &contract, const eosio::name &member);

        static const bool isMember(const eosio::name &rootNode, const eosio::name &member);
        static Member getOrNew (eosio::name contract, const eosio::name &creator, const eosio::name &member);
        static const eosio::checksum256 calcHash(const eosio::name &member);

        eosio::name getAccount ();
        void apply (const eosio::checksum256& applyTo, const std::string content);
        void enroll (const eosio::name &enroller, const std::string &content);

    private: 
        static ContentGroups defaultContent (const eosio::name &member);

    };
} // namespace hypha