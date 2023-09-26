#pragma once

#include <typed_document.hpp>
#include <macros.hpp>

namespace hypha::upvote_election
{

class ElectionRound;

class VoteGroup : public TypedDocument
{
    DECLARE_DOCUMENT(
        Data,
        PROPERTY(round_id, int64_t, RoundID, USE_GET)
    )
public:
    VoteGroup(dao& dao, uint64_t id);
    VoteGroup(dao& dao, uint64_t memberId, Data data);

    uint64_t getOwner();

    static std::optional<VoteGroup> getFromRound(dao& dao, uint64_t roundId, uint64_t memberId);
private:
    virtual const std::string buildNodeLabel(ContentGroups &content) override
    {
        return "Vote Group";
    }
};

using VoteGroupData = VoteGroup::Data;

} // namespace hypha::upvote_election