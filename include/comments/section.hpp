#pragma once
#include <typed_document.hpp>
#include <string>
#include <eosio/name.hpp>
#include <comments/likeable.hpp>

namespace hypha
{

    class dao;

    class Section : public Likeable
    {
        public:
            Section(dao& dao, uint64_t id);
            Section(
                dao& dao,
                Document& proposal
            );

            const void remove();

            const void move(Document& proposal);
        protected:
            virtual const std::string buildNodeLabel(ContentGroups &content);

    };

}
