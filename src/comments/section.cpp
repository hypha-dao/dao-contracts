#include <comments/section.hpp>
#include <comments/comment.hpp>
#include <common.hpp>
#include <document_graph/edge.hpp>
#include <dao.hpp>

#define TYPED_DOCUMENT_TYPE document_types::COMMENT_SECTION

namespace hypha
{

    const std::string GROUP_SECTION = "comment_section";

    Section::Section(dao& dao, uint64_t id) : TypedDocument(dao, id, TYPED_DOCUMENT_TYPE)
    {
        TRACE_FUNCTION()
    }

    Section::Section(
        dao& dao,
        Document& proposal
    ) : TypedDocument(dao, TYPED_DOCUMENT_TYPE)
    {
        TRACE_FUNCTION()
        ContentGroups contentGroups{
            ContentGroup{
                Content(CONTENT_GROUP_LABEL, GROUP_SECTION)
            }
        };

        ContentWrapper wrapper = ContentWrapper(contentGroups);
        initializeDocument(dao, contentGroups);

        Edge::getOrNew(dao.get_self(), dao.get_self(), proposal.getID(), getId(), common::COMMENT_SECTION);
        Edge::getOrNew(dao.get_self(), dao.get_self(), getId(), proposal.getID(), common::COMMENT_SECTION_OF);
    }

    const void Section::move(Document& proposal)
    {
        Edge::getTo(this->getDao().get_self(), getId(), common::COMMENT_SECTION).erase();
        Edge::get(this->getDao().get_self(), getId(), common::COMMENT_SECTION_OF).erase();

        Edge::getOrNew(this->getDao().get_self(), this->getDao().get_self(), proposal.getID(), getId(), common::COMMENT_SECTION);
        Edge::getOrNew(this->getDao().get_self(), this->getDao().get_self(), getId(), proposal.getID(), common::COMMENT_SECTION_OF);
    }

    const std::string Section::buildNodeLabel(ContentGroups &content)
    {
        TRACE_FUNCTION()

        return "Comment section";
    }

    const void Section::remove()
    {
        TRACE_FUNCTION()

        std::vector<Edge> comments = this->getDao().getGraph().getEdgesFrom(getId(), common::COMMENT);
        for (auto& edge : comments) {
            Comment(this->getDao(), edge.getToNode()).remove();
        }

        this->getDao().getGraph().removeEdges(getId());
        this->erase();
    }
}
