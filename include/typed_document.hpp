#pragma once
#include <document_graph/document.hpp>

namespace hypha
{
    class dao;

    template<typename std::string& T>
    class TypedDocument : public Document
    {
        public:
            TypedDocument(dao& dao, const eosio::checksum256& hash);

        protected:
            TypedDocument(dao& dao, ContentGroups &content);
            dao& getDao() const;

        private:
            dao& m_dao;
            void validate();
            ContentGroups& processContent(ContentGroups& content);

    };

    namespace document_types
    {
        extern std::string VOTE;
    }
}
