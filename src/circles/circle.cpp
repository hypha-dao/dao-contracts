#include <circles/circle.hpp>
#include <dao.hpp>

#define TYPED_DOCUMENT_TYPE document_types::CIRCLE

namespace hypha
{
    static const std::string AUTHOR_LABEL = "author";
    static const std::string TITLE_LABEL = "title";
    static const std::string DESCRIPTION_LABEL = "description";
    static const std::string BUDGET_LABEL = "budget";

    Circle::Circle(dao& dao, uint64_t id)
    : TypedDocument(dao, id, TYPED_DOCUMENT_TYPE)
    {
        TRACE_FUNCTION()
    }

    Circle::Circle(
        dao& dao,
        const name author,
        const string title,
        const string description,
        const uint64_t dao_id,
        const uint64_t parent_circle,
        const asset budget
    )
    : TypedDocument(dao, TYPED_DOCUMENT_TYPE)
    {
        TRACE_FUNCTION()
        ContentGroups contentGroups{
            ContentGroup{
                Content(AUTHOR_LABEL, author),
                Content(CONTENT_GROUP_LABEL, DETAILS),
                Content(TITLE_LABEL, title),
                Content(DESCRIPTION_LABEL, description),
                Content(BUDGET_LABEL, budget),
            }
        };

        initializeDocument(dao, contentGroups);

        if (parent_circle != 0) {
            // Validate the circle exists
            Circle(dao, parent_circle);
        }

        if (parent_circle != 0)
        {
            Edge::getOrNew(dao.get_self(), dao.get_self(), parent_circle, this->getId(), common::CIRCLE);
            Edge::getOrNew(dao.get_self(), dao.get_self(), this->getId(), parent_circle, common::CIRCLE_OF);
        }

        Edge::getOrNew(dao.get_self(), dao.get_self(), dao_id, this->getId(), common::CIRCLE_DAO);
        Edge::getOrNew(dao.get_self(), dao.get_self(), this->getId(), dao_id, common::CIRCLE_DAO_OF);
    }

    const std::string Circle::buildNodeLabel(ContentGroups &content)
    {
        TRACE_FUNCTION()
        std::string title = ContentWrapper(content).getOrFail(
            DETAILS,
            TITLE_LABEL,
            "Circle does not have " DETAILS " content group"
        )->getAs<std::string>();
        return "Circle: " + title;
    }

    void Circle::remove()
    {
        TRACE_FUNCTION()
        // In the future we will need to check relations and see if we need to update something
        // Right now removing all the edges is enough
        getDao().getGraph().removeEdges(this->getId());
        this->erase();
    }

    name Circle::getAuthor()
    {
        return this->getDocument().getContentWrapper().getOrFail(
            DETAILS,
            AUTHOR_LABEL,
            "Circle does not have " DETAILS " content group"
        )->getAs<name>();
    }

    void Circle::join(const name& member)
    {
        uint64_t memberId = getDao().getMemberID(member);

        // currently, each member can only join one circle
        auto circleEdges = this->getDao().getGraph().getEdgesFrom(memberId, common::CIRCLE_MEMBER_OF);
        for (auto circleEdge : circleEdges) {
            Edge::get(
                this->getDao().get_self(), circleEdge.getToNode(), circleEdge.getFromNode(), common::CIRCLE_MEMBER
            ).erase();
            circleEdge.erase();
        }

        Edge::getOrNew(this->getDao().get_self(), this->getDao().get_self(), this->getId(), memberId, common::CIRCLE_MEMBER);
        Edge::getOrNew(this->getDao().get_self(), this->getDao().get_self(), memberId, this->getId(), common::CIRCLE_MEMBER_OF);
    }

    void Circle::exit(const name& member)
    {
        uint64_t memberId = getDao().getMemberID(member);
        Edge::get(this->getDao().get_self(), this->getId(), memberId, common::CIRCLE_MEMBER).erase();
        Edge::get(this->getDao().get_self(), memberId, this->getId(), common::CIRCLE_MEMBER_OF).erase();
    }
}
