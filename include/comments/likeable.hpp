#pragma once
#include <typed_document.hpp>

namespace hypha
{

    class Likeable: virtual public TypedDocument
    {
        public:
            virtual ~Likeable();
            const void like(eosio::name user);
            const void unlike(eosio::name user);
            const void updateContent(ContentWrapper& wrapper);
    };
}
