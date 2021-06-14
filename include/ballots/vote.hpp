#pragma once
#include <typed_document.hpp>
#include <string>
#include <eosio/name.hpp>

namespace hypha
{
    class dao;

    class Vote : public TypedDocument<hypha::document_types::VOTE> 
    {
    public:
        Vote(dao& dao, const eosio::checksum256& hash);
        Vote(
            dao& dao, 
            const eosio::name voter,
            std::string vote, 
            Document& proposal,
            std::optional<std::string> notes
        );

        const std::string& getVote();
        const eosio::asset& getPower();
        const eosio::name& getVoter();

    protected:
        virtual const std::string buildNodeLabel(ContentGroups &content);

    };
}