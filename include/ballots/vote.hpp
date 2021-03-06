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
    
        static Vote build(
            dao& dao, 
            const eosio::name voter, 
            std::string vote, 
            Document& proposal // This should be a Proposal document in the future.
        );

        const std::string& getVote();
        const eosio::asset& getPower();
        const eosio::name& getVoter();

    private:
        Vote(dao& dao, ContentGroups &content);
    };
}