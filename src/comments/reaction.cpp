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
        const eosio::name reaction
    )
    : TypedDocument(dao, TYPED_DOCUMENT_TYPE)
    {
        TRACE_FUNCTION()

        EOS_CHECK(
            is_reaction(reaction),
            "Only like reaction currently supported"
        );

        ContentGroups contentGroups{
            ContentGroup{
                Content(CONTENT_GROUP_LABEL, CONTENT_GROUP_LABEL_REACTION),
                Content(CONTENT_REACTION_TYPE, reaction),
            }
        };

        initializeDocument(dao, contentGroups);

        // Content has reaction
        Edge::getOrNew(dao.get_self(), dao.get_self(), likeable.getId(), this->getId(), common::REACTION);
        // Reaction was made to content
        Edge::getOrNew(dao.get_self(), dao.get_self(), this->getId(), likeable.getId(), common::REACTION_OF);
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

    void Reaction::react(eosio::name who)
    {
        uint64_t memberId = getDao().getMemberID(who);
        uint64_t likeableId = getLikeableId();
        EOS_CHECK(
            !Edge::exists(getDao().get_self(), memberId, likeableId, common::REACTED_TO),
            "Member already reacted to this document"
        );

        // Member reacted to content
        Edge::getOrNew(getDao().get_self(), who, memberId, likeableId, common::REACTED_TO);
        // Content has been reacted by member
        Edge::getOrNew(getDao().get_self(), who, likeableId, memberId, common::REACTED_BY);
        // Member made reaction
        Edge::getOrNew(getDao().get_self(), who, memberId, this->getId(), common::REACTION_LINK);
        // Reaction was made by member
        Edge::getOrNew(getDao().get_self(), who, this->getId(), memberId, common::REACTION_LINK_REVERSE);
    }

    void Reaction::unreact(eosio::name who)
    {
        uint64_t memberId = getDao().getMemberID(who);
        uint64_t likeableId = getLikeableId();
        EOS_CHECK(
            Edge::exists(getDao().get_self(), memberId, likeableId, common::REACTED_TO),
            "Member has not reacted to this document"
        );

        Edge::get(getDao().get_self(), memberId, likeableId, common::REACTED_TO).erase();
        Edge::get(getDao().get_self(), likeableId, memberId, common::REACTED_BY).erase();
        Edge::get(getDao().get_self(), memberId, this->getId(), common::REACTION_LINK).erase();
        Edge::get(getDao().get_self(), this->getId(), memberId, common::REACTION_LINK_REVERSE).erase();
    }


    Reaction Reaction::getReactionByUser(dao& dao, Likeable& likeable, const eosio::name who)
    {
        uint64_t memberId = dao.getMemberID(who);
        Edge::get(dao.get_self(), memberId, likeable.getId(), common::REACTED_TO);

        std::vector<Edge> reactionEdges = dao.getGraph().getEdgesFromOrFail(likeable.getId(), common::REACTION);
        for (auto reactionEdge : reactionEdges)
        {
            if (Edge::exists(dao.get_self(), memberId, reactionEdge.getToNode(), common::REACTION_LINK))
            {
                return Reaction(dao, reactionEdge.getToNode());
            }
        }

        eosio::check(false, "No reaction found for user on document");
        return Reaction(dao, 0);
    }

    const std::string Reaction::buildNodeLabel(ContentGroups &content)
    {
        eosio::name type = ContentWrapper(content).getOrFail(
            CONTENT_GROUP_LABEL_REACTION,
            CONTENT_REACTION_TYPE,
            "Could not find reaction_type, this is a bug."
        )->getAs<eosio::name>();

        return "Reaction: " + type.to_string() + " for this document.";
    }

    uint64_t Reaction::getLikeableId()
    {
        return Edge::get(getDao().get_self(), this->getId(), common::REACTION_OF).getToNode();
    }
}
