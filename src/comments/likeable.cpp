#include <comments/likeable.hpp>
#include <dao.hpp>

namespace hypha
{

    const std::string ENTRY_LIKES = "likes";
    const std::string GROUP_LIKES = "likes";

    const void Likeable::like(TypedDocument& typed_document, name user)
    {
        TRACE_FUNCTION()

        auto content_wrapper = typed_document.getDocument().getContentWrapper();
        auto like_group = content_wrapper.getGroup(GROUP_LIKES);

        auto content_pair = content_wrapper.get(like_group.first, user.to_string());

        eosio::check(content_pair.first == -1, "User already likes this comment section");
        content_wrapper.insertOrReplace(like_group.first, Content(user.to_string(), true));

        auto details_group = content_wrapper.getGroup(DETAILS);
        auto likes = content_wrapper.getOrFail(details_group.first, ENTRY_LIKES).second;
        content_wrapper.insertOrReplace(details_group.first, Content(ENTRY_LIKES, likes->getAs<int64_t>() + 1));

        typed_document.update();
    }

    const void Likeable::unlike(TypedDocument& typed_document, name user)
    {
        TRACE_FUNCTION()

        auto content_wrapper = typed_document.getDocument().getContentWrapper();
        auto like_group = content_wrapper.getGroup(GROUP_LIKES);

        auto content_pair = content_wrapper.get(like_group.first, user.to_string());

        eosio::check(content_pair.first != -1, "User does not like this comment section");
        content_wrapper.removeContent(like_group.first, user.to_string());

        auto details_group = content_wrapper.getGroup(DETAILS);
        auto likes = content_wrapper.getOrFail(details_group.first, ENTRY_LIKES).second;
        content_wrapper.insertOrReplace(details_group.first, Content(ENTRY_LIKES, likes->getAs<int64_t>() - 1));

        typed_document.update();
    }

    const void Likeable::updateContent(ContentWrapper& wrapper)
    {
        TRACE_FUNCTION()
        auto group = wrapper.getGroupOrCreate(DETAILS);
        wrapper.insertOrReplace(*group.second, Content(ENTRY_LIKES, 0));
        wrapper.getGroupOrCreate(GROUP_LIKES);
    }
}
