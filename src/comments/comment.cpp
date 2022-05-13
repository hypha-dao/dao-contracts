#include <comments/comment.hpp>
#include <document_graph/edge.hpp>
#include <dao.hpp>
#include <common.hpp>

#define TYPED_DOCUMENT_TYPE document_types::COMMENT

namespace hypha
{

    const std::string GROUP_COMMENT = "comment";
    const std::string ENTRY_AUTHOR = "author";
    const std::string ENTRY_CONTENT = "content";
    const std::string ENTRY_EDITED = "edited";
    const std::string ENTRY_DELETED = "deleted";
    const std::string ENTRY_COMMENT_COUNT = "comment_count";

    Comment::Comment(dao& dao, uint64_t id) : TypedDocument(dao, id, TYPED_DOCUMENT_TYPE)
    {
        TRACE_FUNCTION()
    }

    Comment::Comment(
        dao& dao,
        Section& section,
        const eosio::name author,
        const string& content
    ) : TypedDocument(dao, TYPED_DOCUMENT_TYPE)
    {
        TRACE_FUNCTION()
        this->initComment(
            dao,
            section.getId(),
            author,
            content
        );
    }

    Comment::Comment(
        dao& dao,
        Comment& comment,
        const eosio::name author,
        const string& content
    ) : TypedDocument(dao, TYPED_DOCUMENT_TYPE)
    {
        TRACE_FUNCTION()
        this->initComment(
            dao,
            comment.getId(),
            author,
            content
        );
    }

    const std::string Comment::buildNodeLabel(ContentGroups &content)
    {
        TRACE_FUNCTION()

        ContentWrapper content_wrapper(content);
        const eosio::name author = content_wrapper.getOrFail(GROUP_COMMENT, ENTRY_AUTHOR)->getAs<eosio::name>();

        if (content_wrapper.exists(GROUP_COMMENT, ENTRY_DELETED)) {
            return "Deleted content from: " + author.to_string();
        }

        const std::string comment = ContentWrapper(content).getOrFail(
            GROUP_COMMENT, ENTRY_CONTENT
        )->getAs<std::string>();

        return author.to_string() + ": " + comment.substr(0, 10) + (comment.size() > 10 ? "..." : "");
    }

    void Comment::initComment(
        dao& dao,
        uint64_t target_id,
        const eosio::name author,
        const string& content
    )
    {
        ContentGroups contentGroups{
            ContentGroup{
                Content(CONTENT_GROUP_LABEL, GROUP_COMMENT),
                Content(ENTRY_AUTHOR, author),
                Content(ENTRY_CONTENT, content),
                Content(ENTRY_COMMENT_COUNT, 0)
            }
        };
        initializeDocument(dao, contentGroups);

        Edge::getOrNew(dao.get_self(), author, target_id, getId(), common::COMMENT);
        Edge::getOrNew(dao.get_self(), author, getId(), target_id, common::COMMENT_OF);
    }

    eosio::name Comment::getAuthor()
    {
        TRACE_FUNCTION()
        auto content_wrapper = this->getDocument().getContentWrapper();
        return content_wrapper.getOrFail(GROUP_COMMENT, ENTRY_AUTHOR)->getAs<eosio::name>();
    }

    void Comment::edit(const string& new_content)
    {
        TRACE_FUNCTION()

        auto content_wrapper = this->getDocument().getContentWrapper();
        eosio::check(content_wrapper.exists(GROUP_COMMENT, ENTRY_DELETED) == false, "Comment has been deleted");
        auto group = content_wrapper.getGroupOrFail(GROUP_COMMENT);

        content_wrapper.insertOrReplace(*group, Content(ENTRY_CONTENT, new_content));
        content_wrapper.insertOrReplace(*group, Content(ENTRY_EDITED, true));
        this->update();
    }

    void Comment::markAsDeleted()
    {
        TRACE_FUNCTION()

        auto content_wrapper = this->getDocument().getContentWrapper();
        eosio::check(content_wrapper.exists(GROUP_COMMENT, ENTRY_DELETED) == false, "Comment has been deleted already");
        auto group = content_wrapper.getGroupOrFail(GROUP_COMMENT);

        content_wrapper.insertOrReplace(*group, Content(ENTRY_DELETED, true));
        this->update();
    }

    void Comment::remove()
    {
        TRACE_FUNCTION()

        // only comments should be here
        std::vector<Edge> commentsOf = this->getDao().getGraph().getEdgesFrom(getId(), common::COMMENT);
        for (auto& edge : commentsOf) {
            Comment(this->getDao(), edge.getToNode()).remove();
        }

        this->getDao().getGraph().removeEdges(getId());
        this->erase();
    }
}
