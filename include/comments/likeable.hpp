#pragma once
#include <typed_document.hpp>

namespace hypha
{

    class Likeable: virtual public TypedDocument
    {
        public:
            virtual ~Likeable();
            const void like(eosio::name user, eosio::name reaction);
            const void unlike(eosio::name user);
    };
}
