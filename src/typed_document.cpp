#include <typed_document.hpp>
#include <dao.hpp>
#include <logger/logger.hpp>

namespace hypha
{

    template<typename std::string& T>
    TypedDocument<T>::TypedDocument(dao& dao, uint64_t id)
    : m_dao(dao), document(Document(dao.get_self(), id))
    {
        TRACE_FUNCTION()
        validate();
    }

    template<typename std::string& T>
    TypedDocument<T>::TypedDocument(dao& dao)
    : m_dao(dao)
    {

    }

    template<typename std::string& T>
    const std::string& TypedDocument<T>::getNodeLabel()
    {
        TRACE_FUNCTION()
        return document.getContentWrapper().getOrFail(
            SYSTEM,
            NODE_LABEL,
            "Typed document does not have a node label"
        )->template getAs<std::string>();
    }

    template<typename std::string& T>
    Document& TypedDocument<T>::getDocument()
    {
        return document;
    }

    template<typename std::string& T>
    uint64_t TypedDocument<T>::getId()
    {
        return document.getID();
    }

    template<typename std::string& T>
    void TypedDocument<T>::initializeDocument(dao& dao, ContentGroups &content)
    {
        TRACE_FUNCTION()

        document = Document(dao.get_self(), dao.get_self(), processContent(content));
    }

    template<typename std::string& T>
    void TypedDocument<T>::validate()
    {
        auto [idx, docType] = document.getContentWrapper().get(SYSTEM, TYPE);

        EOS_CHECK(idx != -1, "Content item labeled 'type' is required for this document but not found.");
        EOS_CHECK(docType->template getAs<eosio::name>() == eosio::name(T),
                     "invalid document type. Expected: " + T +
                         "; actual: " + docType->template getAs<eosio::name>().to_string());

        getNodeLabel();
    }

    template<typename std::string& T>
    ContentGroups& TypedDocument<T>::processContent(ContentGroups& content)
    {
        ContentWrapper wrapper(content);
        auto [systemIndex, contentGroup] = wrapper.getGroupOrCreate(SYSTEM);
        wrapper.insertOrReplace(systemIndex, Content(TYPE, eosio::name(T)));
        wrapper.insertOrReplace(systemIndex, Content(NODE_LABEL, buildNodeLabel(content)));

        return wrapper.getContentGroups();
    }

    template<typename std::string& T>
    dao& TypedDocument<T>::getDao() const
    {
        return m_dao;
    }

    template<typename std::string& T>
    bool TypedDocument<T>::documentExists(dao& dao, const uint64_t& id)
    {
        bool exists = Document::exists(dao.get_self(), id);
        if (exists) {
            return true;
        }

        return {};
    }
    template<typename std::string& T>
    void TypedDocument<T>::update()
    {
        this->document.update();
    }

    template<typename std::string& T>
    void TypedDocument<T>::erase()
    {
        this->m_dao.getGraph().eraseDocument(getId());
    }

    // Todo: All this could be replaced by macros to make it easier to:
    // define, instantiate and extend
    // Define the types in the .hpp file as well
    namespace document_types
    {
        std::string VOTE = std::string("vote");
        std::string VOTE_TALLY = std::string("vote.tally");
        std::string COMMENT = std::string("comment");
        std::string COMMENT_SECTION = std::string("comment.section");
    }

    // Instantiate each template type
    template class TypedDocument<document_types::VOTE>;
    template class TypedDocument<document_types::VOTE_TALLY>;
    template class TypedDocument<document_types::COMMENT>;
    template class TypedDocument<document_types::COMMENT_SECTION>;

}
