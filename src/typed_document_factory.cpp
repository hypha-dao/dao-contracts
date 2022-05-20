#include <typed_document_factory.hpp>
#include <ballots/vote.hpp>
#include <ballots/vote_tally.hpp>
#include <comments/section.hpp>
#include <comments/comment.hpp>
#include <dao.hpp>

namespace hypha
{
    static string names_to_string_csv(std::vector<eosio::name> names)
    {
        auto begin = names.begin();
        auto end = names.end();
        std::string csv;
        bool first = true;
        for (; begin != end; begin++)
        {
            if (!first)
            {
                csv += ", ";
            }
            csv += begin->to_string();
            first = false;
        }
        return csv;
    }

    static eosio::name check_types(Document& document, std::vector<eosio::name> expected_types)
    {
        eosio::name type = document.getContentWrapper().getOrFail(SYSTEM, TYPE)->getAs<eosio::name>();
        if (expected_types.size() > 0)
        {
            eosio::check(
                std::find(expected_types.begin(), expected_types.end(), type) != expected_types.end(),
                "Requested Document of type (" + type.to_string() + ") not in the expected_types list" + names_to_string_csv(expected_types)
            );
        }
        return type;
    }

    std::unique_ptr<TypedDocument> TypedDocumentFactory::getTypedDocument(dao& dao, uint64_t id, std::vector<eosio::name> expected_types)
    {
        Document document(dao.get_self(), id);
        eosio::name type = check_types(document, expected_types);

        switch(type.value)
        {
            case document_types::VOTE.value:
                return std::unique_ptr<Vote>(new Vote(dao, id));
            case document_types::VOTE_TALLY.value:
                return std::unique_ptr<VoteTally>(new VoteTally(dao, id));
            case document_types::COMMENT.value:
                return std::unique_ptr<Comment>(new Comment(dao, id));
            case document_types::COMMENT_SECTION.value:
                return std::unique_ptr<Section>(new Section(dao, id));
            default:
                eosio::check(false, "Document with id " + std::to_string(id) + " is not a TypedDocument of any known type.");
        }

        return nullptr;
    }

    std::unique_ptr<Likeable> TypedDocumentFactory::getLikeableDocument(dao& dao, uint64_t id)
    {
        Document document(dao.get_self(), id);
        eosio::name type = check_types(document, { document_types::COMMENT, document_types::COMMENT_SECTION });

        if (type == document_types::COMMENT) {
            return std::unique_ptr<Comment>(new Comment(dao, id));
        } else if (type == document_types::COMMENT_SECTION) {
            return std::unique_ptr<Section>(new Section(dao, id));
        }

        eosio::check(false, "This is a bug - Likeable object not tagged correctly on factory");

        return nullptr;
    }

}
