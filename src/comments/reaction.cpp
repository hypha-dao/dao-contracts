#include <comments/reaction.hpp>
#include <comments/likeable.hpp>
#include <dao.hpp>
#include <document_graph/edge.hpp>

#define TYPED_DOCUMENT_TYPE document_types::REACTION
#define CONTENT_GROUP_LABEL_REACTION "reaction"
#define CONTENT_REACTION_TYPE "type"
#define CONTENT_REACTION_WHO "who"

namespace hypha
{
    static constexpr eosio::name REACTION_LIKE("liked");


    static bool is_reaction(eosio::name reaction)
    {
        return reaction == REACTION_LIKE;
    }

    Reaction::Reaction(hypha::dao& dao, uint64_t id)
    : TypedDocument(dao, id, TYPED_DOCUMENT_TYPE)
    {
        TRACE_FUNCTION()
    }

    Reaction::Reaction(
        dao&dao,
        Likeable& likeable,
        const eosio::name who,
        const eosio::name reaction
    )
    : TypedDocument(dao, TYPED_DOCUMENT_TYPE)
    {
        TRACE_FUNCTION()

        uint64_t memberId = dao.getMemberID(who);
        EOS_CHECK(
            !Edge::exists(dao.get_self(), memberId, likeable.getId(), common::REACTED_TO),
            "Member already reacted to this document"
        );
        EOS_CHECK(
            is_reaction(reaction),
            "Only like reaction currently supported"
        );

        ContentGroups contentGroups{
            ContentGroup{
                Content(CONTENT_GROUP_LABEL, CONTENT_GROUP_LABEL_REACTION),
                Content(CONTENT_REACTION_TYPE, reaction),
                Content(CONTENT_REACTION_WHO, who),
            }
        };

        initializeDocument(dao, contentGroups);
        Edge::getOrNew(dao.get_self(), who, memberId, likeable.getId(), common::REACTED_TO);
        Edge::getOrNew(dao.get_self(), who, likeable.getId(), memberId, common::REACTED_BY);
        Edge::getOrNew(dao.get_self(), who, likeable.getId(), this->getId(), common::REACTION);
        Edge::getOrNew(dao.get_self(), who, this->getId(), likeable.getId(), common::REACTION_OF);

    }

    void Reaction::remove()
    {
        eosio::name who = getDocument().getContentWrapper().getOrFail(
            CONTENT_GROUP_LABEL_REACTION,
            CONTENT_REACTION_WHO,
            "Could not find who had this reaction, this is a bug."
        )->getAs<eosio::name>();

        uint64_t memberId = getDao().getMemberID(who);
        uint64_t likeableId = Edge::get(getDao().get_self(), memberId, common::REACTED_TO).getToNode();

        std::vector<Edge> reactions = getDao().getGraph().getEdgesFrom(likeableId, common::REACTED_TO);
        for (auto reaction : reactions) {
            if (reaction.getCreator() == who) {
                Edge::get(getDao().get_self(), reaction.getToNode(), reaction.getFromNode(), common::REACTED_BY).erase();
                reaction.erase();
                break;
            }
        }

        this->getDao().getGraph().removeEdges(getId());
        this->erase();
    }

    Reaction Reaction::getReaction(dao& dao, Likeable& likeable, const eosio::name who)
    {
        std::vector<Edge> reactions = dao.getGraph().getEdgesFrom(likeable.getId(), common::REACTION);
        for (auto reaction : reactions) {
            if (reaction.getCreator() == who) {
                return Reaction(dao, reaction.getToNode());
            }
        }

        eosio::check(false, "No reaction found for user on document");
        return Reaction(dao, 0);
    }

    const std::string Reaction::buildNodeLabel(ContentGroups &content)
    {
        eosio::name who = ContentWrapper(content).getOrFail(
            CONTENT_GROUP_LABEL_REACTION,
            CONTENT_REACTION_WHO,
            "Could not find who had this reaction, this is a bug."
        )->getAs<eosio::name>();

        eosio::name type = ContentWrapper(content).getOrFail(
            CONTENT_GROUP_LABEL_REACTION,
            CONTENT_REACTION_TYPE,
            "Could not find reaction_type, this is a bug."
        )->getAs<eosio::name>();

        return who.to_string() + " " + type.to_string() + " this document.";
    }
}
