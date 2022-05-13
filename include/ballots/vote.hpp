#pragma once
#include <typed_document.hpp>
#include <string>
#include <eosio/name.hpp>

namespace hypha
{
    class dao;

    class Vote : public TypedDocument
    {
    public:
        Vote(dao& dao, uint64_t id);
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
