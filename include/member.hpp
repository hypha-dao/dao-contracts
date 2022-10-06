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
        Member(dao& dao, uint64_t docID);

        static bool isMember(dao& dao, uint64_t daoID, const eosio::name &member);
        static bool exists(dao& dao, const eosio::name& memberName);
        eosio::name getAccount ();
        void apply (uint64_t applyTo, const std::string content);
        void enroll (const eosio::name &enroller, uint64_t appliedTo, const std::string &content);

        /**
         * @brief Verifies if the member is already a member of the give DAO
         * and if not then it atomatically enrolls him
         * 
         * @param daoID 
         */
        void checkMembershipOrEnroll(uint64_t daoID);
    private: 
        static ContentGroups defaultContent (const eosio::name &member);
        dao& m_dao;
    };
} // namespace hypha