#include <typed_document.hpp>
#include <dao.hpp>
#include <logger/logger.hpp>

namespace hypha
{

    TypedDocument::TypedDocument(dao& dao, uint64_t id, eosio::name type)
    : m_dao(dao), document(Document(dao.get_self(), id)), type(type)
    {
        TRACE_FUNCTION()
        validate();
    }

    TypedDocument::TypedDocument(dao& dao, eosio::name type)
    : m_dao(dao), type(type)
    {

    }

    TypedDocument::~TypedDocument()
    {

    }

    const std::string& TypedDocument::getNodeLabel()
    {
        TRACE_FUNCTION()
        return document.getContentWrapper().getOrFail(
            SYSTEM,
            NODE_LABEL,
            "Typed document does not have a node label"
        )->getAs<std::string>();
    }

    Document& TypedDocument::getDocument()
    {
        return document;
    }

    uint64_t TypedDocument::getId() const
    {
        return document.getID();
    }

    void TypedDocument::initializeDocument(dao& dao, ContentGroups &content)
    {
        TRACE_FUNCTION()
        document = Document(dao.get_self(), dao.get_self(), std::move(processContent(content)));
    }


    void TypedDocument::updateDocument(ContentGroups content)
    {
        document.getContentGroups() = std::move(processContent(content));
        document.update();
    }

    void TypedDocument::validate()
    {
        auto [idx, docType] = document.getContentWrapper().get(SYSTEM, TYPE);

        EOS_CHECK(idx != -1, "Content item labeled 'type' is required for this document but not found.");
        EOS_CHECK(docType->getAs<eosio::name>() == getType(),
                  to_str("invalid document type. Expected: ", getType(),
                               "; actual: ", docType->getAs<eosio::name>(), " for document: ", getId())
        )

        getNodeLabel();
    }

    ContentGroups& TypedDocument::processContent(ContentGroups& content)
    {
        ContentWrapper wrapper(content);
        auto [systemIndex, contentGroup] = wrapper.getGroupOrCreate(SYSTEM);
        wrapper.insertOrReplace(systemIndex, Content(TYPE, getType()));
        wrapper.insertOrReplace(systemIndex, Content(NODE_LABEL, buildNodeLabel(content)));

        return wrapper.getContentGroups();
    }

    dao& TypedDocument::getDao() const
    {
        return m_dao;
    }

    bool TypedDocument::documentExists(dao& dao, const uint64_t& id)
    {
        bool exists = Document::exists(dao.get_self(), id);
        if (exists) {
            return true;
        }

        return {};
    }
    void TypedDocument::update()
    {
        document.update();
    }

    void TypedDocument::erase()
    {
        m_dao.getGraph().eraseDocument(getId());
    }

    eosio::name TypedDocument::getType()
    {
        return type;
    }

}
