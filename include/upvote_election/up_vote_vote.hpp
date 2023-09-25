#pragma once

#include <typed_document.hpp>
#include <macros.hpp>

namespace hypha::upvote_election
{

class ElectionGroup;

// stores voted
// edges 
//      round -> vote
//      member -> vote
// 
class UpVoteVote : public TypedDocument
{
    DECLARE_DOCUMENT(
        Data,
        PROPERTY(voter_id, int64_t, VoterID, USE_GET),
        PROPERTY(voted_id, int64_t, VotedID, USE_GET)
    )

public:
    UpVoteVote(dao& dao, uint64_t id);
    UpVoteVote(dao& dao, uint64_t group_id, Data data);

    // void castVotes(ElectionGroup& group, uint64_t voted); //??

    uint64_t getElectionGroup();
    
    // static std::optional<VoteGroup> getFromRound(dao& dao, uint64_t roundId, uint64_t memberId);
private:
    virtual const std::string buildNodeLabel(ContentGroups &content) override
    {
        return "Vote Group";
    }
};

using UpVoteVoteData = UpVoteVote::Data;

} // namespace hypha::upvote_election