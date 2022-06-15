#include <circles/circle.hpp>
#include <dao.hpp>

#define TYPED_DOCUMENT_TYPE document_types::CIRCLE

namespace hypha
{
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
                Content(CONTENT_GROUP_LABEL, DETAILS),
                Content(TITLE_LABEL, title),
                Content(DESCRIPTION_LABEL, description),
                Content(BUDGET_LABEL, budget)
            }
        };

        initializeDocument(dao, contentGroups);

        if (parent_circle != 0) {
            // Validate the circle exists
            Circle(dao, parent_circle);
        }

        uint64_t parent = parent_circle == 0 ? dao_id : parent_circle;
        Edge::getOrNew(dao.get_self(), dao.get_self(), parent, this->getId(), common::CIRCLE);
        Edge::getOrNew(dao.get_self(), dao.get_self(), this->getId(), parent, common::CIRCLE_OF);

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
}
