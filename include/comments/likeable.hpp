#include <typed_document.hpp>

namespace hypha
{

    class Likeable
    {
        public:
            const void like(TypedDocument& typed_document, eosio::name user);
            const void unlike(TypedDocument& typed_document, eosio::name user);
            const void updateContent(ContentWrapper& wrapper);
    };
}
