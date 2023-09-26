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
        PROPERTY(voter_id, int64_t, VoterId, USE_GET),
        PROPERTY(voted_id, int64_t, VotedId, USE_GETSET)
    )

public:
    UpVoteVote(dao& dao, uint64_t id);
    UpVoteVote(dao& dao, uint64_t group_id, Data data);

    uint64_t getElectionGroup();
    
private:
    virtual const std::string buildNodeLabel(ContentGroups &content) override
    {
        return "Vote Group";
    }
};

using UpVoteVoteData = UpVoteVote::Data;

} // namespace hypha::upvote_election