#pragma once
#include <typed_document.hpp>
#include <string>
#include <eosio/name.hpp>

namespace hypha
{

    class dao;

    class Section : public TypedDocument<document_types::COMMENT_SECTION>
    {
        public:
            Section(dao& dao, uint64_t id);
            Section(
                dao& dao,
                Document& proposal
            );

            const void remove();
            const void like(eosio::name user);
            const void unlike(eosio::name user);
        protected:
            virtual const std::string buildNodeLabel(ContentGroups &content);
    };

}
