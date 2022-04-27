#pragma once
#include <typed_document.hpp>
#include <string>
#include <eosio/name.hpp>
#include <comments/section.hpp>

namespace hypha
{
    class dao;

    class Comment : public TypedDocument<hypha::document_types::COMMENT>
    {
    public:
        Comment(dao& dao, uint64_t id);
        Comment(
            dao& dao,
            Section& section,
            const eosio::name author,
            const string& content
        );
        Comment(
            dao& dao,
            Comment& comment,
            const eosio::name author,
            const string& content
        );

        void edit(const string& new_content);
        void markAsDeleted();
        void remove();

    protected:
        virtual const std::string buildNodeLabel(ContentGroups &content);

    private:
        void initComment(
            dao& dao,
            uint64_t target_id,
            const eosio::name author,
            const string& content
        );

    };
}
