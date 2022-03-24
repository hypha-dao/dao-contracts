#pragma once
#include <document_graph/document.hpp>

namespace hypha
{
    class dao;

    template<typename std::string& T>
    class TypedDocument
    {
public:
        TypedDocument(dao& dao, uint64_t id);
        const std::string& getNodeLabel();
        Document& getDocument();

protected:
        TypedDocument(dao& dao);
        void initializeDocument(dao& dao, ContentGroups& content);
        dao& getDao() const;
        bool documentExists(dao& dao, const uint64_t& id);
        virtual const std::string buildNodeLabel(ContentGroups& content) = 0;

private:
        dao& m_dao;
        Document document;
        void validate();
        ContentGroups& processContent(ContentGroups& content);
    };

    namespace document_types
    {
        extern std::string VOTE;
        extern std::string VOTE_TALLY;
    }
}
