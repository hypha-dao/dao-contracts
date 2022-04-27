#include <comments/section.hpp>
#include <comments/comment.hpp>
#include <common.hpp>
#include <document_graph/edge.hpp>
#include <dao.hpp>

namespace hypha
{

    const std::string GROUP_SECTION = "comment_section";
    const std::string ENTRY_LIKES = "likes";

    const std::string GROUP_LIKES = "likes";

    Section::Section(dao& dao, uint64_t id) : TypedDocument(dao, id)
    {
        TRACE_FUNCTION()
    }

    Section::Section(
        dao& dao,
        Document& proposal
    ) : TypedDocument(dao)
    {
        TRACE_FUNCTION()
        ContentGroups contentGroups{
            ContentGroup{
                Content(CONTENT_GROUP_LABEL, GROUP_SECTION),
                Content(ENTRY_LIKES, 0)
            },
            ContentGroup{
                Content(CONTENT_GROUP_LABEL, GROUP_LIKES)
            }
        };
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
            Document toNode(this->getDao().get_self(), edge.getToNode());
            std::string type = toNode.getContentWrapper().getOrFail(SYSTEM, TYPE)->getAs<std::string>();
            if (type == document_types::COMMENT) {
                Comment(this->getDao(), edge.getToNode()).remove();
            }
        }

        this->getDao().getGraph().removeEdges(getId());
        this->erase();
    }

    const void Section::like(name user)
    {
        TRACE_FUNCTION()

        auto content_wrapper = this->getDocument().getContentWrapper();
        auto like_group = content_wrapper.getGroup(GROUP_LIKES);

        auto content_pair = content_wrapper.get(like_group.first, user.to_string());

        eosio::check(content_pair.first == -1, "User already likes this comment section");
        content_wrapper.insertOrReplace(like_group.first, Content(user.to_string(), true));

        auto section_group = content_wrapper.getGroup(GROUP_SECTION);
        auto likes = content_wrapper.getOrFail(section_group.first, ENTRY_LIKES).second;
        content_wrapper.insertOrReplace(section_group.first, Content(ENTRY_LIKES, likes->getAs<int64_t>() + 1));

        this->update();
    }

    const void Section::unlike(name user)
    {
        TRACE_FUNCTION()

        auto content_wrapper = this->getDocument().getContentWrapper();
        auto like_group = content_wrapper.getGroup(GROUP_LIKES);

        auto content_pair = content_wrapper.get(like_group.first, user.to_string());

        eosio::check(content_pair.first != -1, "User does not like this comment section");
        content_wrapper.removeContent(like_group.first, user.to_string());

        auto section_group = content_wrapper.getGroup(GROUP_SECTION);
        auto likes = content_wrapper.getOrFail(section_group.first, ENTRY_LIKES).second;
        content_wrapper.insertOrReplace(section_group.first, Content(ENTRY_LIKES, likes->getAs<int64_t>() - 1));

        this->update();
    }

}
