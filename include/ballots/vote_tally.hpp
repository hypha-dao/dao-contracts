#pragma once
#include <typed_document.hpp>
#include <string>
#include <eosio/name.hpp>

namespace hypha
{
    class Settings;

    class VoteTally : public TypedDocument
    {
    public:
        VoteTally(dao& dao, uint64_t id);
        VoteTally(
            dao& dao,
            Document& proposal,
            Settings* daoSettings
        );

    protected:
        virtual const std::string buildNodeLabel(ContentGroups &content);
    };
}
