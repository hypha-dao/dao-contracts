#pragma once

#include <typed_document.hpp>

namespace hypha
{
    class dao;

    class TypedDocumentFactory
    {
        public:
            static std::unique_ptr<TypedDocument> getTypedDocument(dao& dao, uint64_t id, std::vector<eosio::name> expected_types);
        //static TypedDocument<std::string> getTypedDocument(std::vector<std::string> types)
    };
}
