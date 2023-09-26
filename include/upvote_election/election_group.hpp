#pragma once

#include <optional>

#include <typed_document.hpp>

#include <macros.hpp>

#include <member.hpp>

#include "upvote_election/up_vote_vote.hpp"

namespace hypha::upvote_election
{

class UpvoteElection;

class ElectionGroup : public TypedDocument
{

    DECLARE_DOCUMENT(
        Data,
        PROPERTY(member_count, int64_t, MemberCount, USE_GETSET),
        PROPERTY(winner, int64_t, Winner, USE_GETSET)
        
    )
public:
    ElectionGroup(dao& dao, uint64_t id);
    ElectionGroup(dao& dao, uint64_t round_id, std::vector<uint64_t> member_ids, Data data);

    bool isElectionRoundMember(uint64_t accountId);
    void vote(int64_t from, int64_t to);

private:
    virtual const std::string buildNodeLabel(ContentGroups &content) override
    {
        return "Election Group";
    }
};

using ElectionGroupData = ElectionGroup::Data;

} // namespace hypha::upvote_election