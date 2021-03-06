#include <typed_document.hpp>
#include <dao.hpp>

namespace hypha
{
    template<typename std::string& T>
    TypedDocument<T>::TypedDocument(dao& dao, const eosio::checksum256& hash)
    : Document(dao.get_self(), hash), m_dao(dao)
    {
        this->validate();
    }

    template<typename std::string& T>
    TypedDocument<T>::TypedDocument(dao& dao, ContentGroups &content)
    : Document(dao.get_self(), dao.get_self(), processContent(content)), m_dao(dao)
    {

    }

    template<typename std::string& T>
    void TypedDocument<T>::validate() 
    {
        auto [idx, docType] = getContentWrapper().get(SYSTEM, TYPE);

        eosio::check(idx != -1, "Content item labeled 'type' is required for this document but not found.");
        eosio::check(docType->template getAs<eosio::name>() == eosio::name(T),
                     "invalid document type. Expected: " + T +
                         "; actual: " + docType->template getAs<eosio::name>().to_string());
    }

    template<typename std::string& T>
    ContentGroups& TypedDocument<T>::processContent(ContentGroups& content) 
    {
        ContentWrapper wrapper(content);
        auto [systemIndex, contentGroup] = wrapper.getGroupOrCreate(SYSTEM);
        wrapper.insertOrReplace(systemIndex, Content(TYPE, eosio::name(T)));

        return wrapper.getContentGroups();
    }

    template<typename std::string& T>
    dao& TypedDocument<T>::getDao() const 
    {
        return this->m_dao;
    }

    // Todo: All this could be replaced by macros to make it easier to:
    // define, instantiate and extend
    // Define the types in the .hpp file as well
    namespace document_types
    {
        std::string VOTE = std::string("vote");
    }

    // Instantiate each template type
    template class TypedDocument<document_types::VOTE>;

}  
