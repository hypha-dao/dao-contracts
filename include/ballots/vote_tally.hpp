#pragma once
#include <typed_document.hpp>
#include <string>
#include <eosio/name.hpp>

namespace hypha
{
    class Settings;

    class VoteTally : public TypedDocument<hypha::document_types::VOTE_TALLY>
    {
    public:
        VoteTally(dao& dao, const eosio::checksum256& hash);
        VoteTally(
            dao& dao,
            Document& proposal,
            Settings* daoSettings
        );

    protected:
        virtual const std::string buildNodeLabel(ContentGroups &content);
    };
}