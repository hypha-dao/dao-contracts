#pragma once
#include <document_graph/document.hpp>
#include <eosio/name.hpp>

namespace hypha
{
    class dao;

    class TypedDocument
    {
        public:
            TypedDocument(dao& dao, uint64_t id, eosio::name type);
            const std::string& getNodeLabel();
            Document& getDocument();
            uint64_t getId() const;
            virtual ~TypedDocument();
            void update();
            void erase();
        protected:
            TypedDocument(dao& dao, eosio::name type);
            void initializeDocument(dao& dao, ContentGroups &content);
            dao& getDao() const;
            bool documentExists(dao& dao, const uint64_t& id);
            virtual const std::string buildNodeLabel(ContentGroups &content) = 0;
            void updateDocument(ContentGroups content);

            eosio::name getType();
            ContentWrapper getContentWrapper() { return document.getContentWrapper(); }
        private:
            dao& m_dao;
            Document document;
            void validate();
            ContentGroups& processContent(ContentGroups& content);
            eosio::name type;

    };

    namespace document_types
    {
        constexpr eosio::name VOTE = eosio::name("vote");
        constexpr eosio::name VOTE_TALLY = eosio::name("vote.tally");
        constexpr eosio::name COMMENT = eosio::name("comment");
        constexpr eosio::name COMMENT_SECTION = eosio::name("cmnt.section");
        constexpr eosio::name REACTION = eosio::name("reaction");
    }
}
