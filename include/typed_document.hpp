#pragma once
#include <document_graph/document.hpp>
#include <eosio/name.hpp>

namespace hypha
{
    class dao;

    class TypedDocument
    {
        public:
            TypedDocument(dao& dao, uint64_t id);
            const std::string& getNodeLabel();
            Document& getDocument();
            uint64_t getId();

        protected:
            TypedDocument(dao& dao);
            void initializeDocument(dao& dao, ContentGroups &content);
            dao& getDao() const;
            bool documentExists(dao& dao, const uint64_t& id);
            virtual const std::string buildNodeLabel(ContentGroups &content) = 0;
            void update();
            void erase();
            friend class Likeable;

            virtual eosio::name getType() = 0;

        private:
            dao& m_dao;
            Document document;
            void validate();
            ContentGroups& processContent(ContentGroups& content);

    };

    namespace document_types
    {
        constexpr eosio::name VOTE = eosio::name("vote");
        constexpr eosio::name VOTE_TALLY = eosio::name("vote.tally");
        constexpr eosio::name COMMENT = eosio::name("comment");
        constexpr eosio::name COMMENT_SECTION = eosio::name("cmnt.section");
    }
}
