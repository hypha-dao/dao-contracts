#pragma once
#include <typed_document.hpp>
#include <string>
#include <eosio/name.hpp>

namespace hypha
{
    class dao;
    class Likeable;

    class Reaction: public TypedDocument
    {
        public:
            Reaction(dao& dao, uint64_t id);
            Reaction(
                dao&dao,
                Likeable& likeable,
                const eosio::name who,
                const eosio::name reaction
            );

            void remove();

            static Reaction getReaction(dao& dao, Likeable& likeable, const eosio::name who);

        protected:
            virtual const std::string buildNodeLabel(ContentGroups &content);
    };
}
