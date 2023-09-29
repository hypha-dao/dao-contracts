#include "upvote_election/up_vote_vote.hpp"

#include "upvote_election/common.hpp"

#include "upvote_election/election_round.hpp"

#include "dao.hpp"

// this document keeps all votes a member has made in a round
// this will need to change since each member can only vote once in a round now, not as many as they want


namespace hypha::upvote_election {

using namespace upvote_election::common;

UpVoteVote::UpVoteVote(dao& dao, uint64_t id)
    : TypedDocument(dao, id, types::ELECTION_UP_VOTE)
{}

UpVoteVote::UpVoteVote(dao& dao, uint64_t group_id, Data data)
    : TypedDocument(dao, types::ELECTION_UP_VOTE)
{
    auto cgs = convert(std::move(data));

    initializeDocument(dao, cgs);

    // edge from group to vote
    Edge(
        getDao().get_self(),
        getDao().get_self(),
        group_id,
        getId(),
        links::UP_VOTE_VOTE
    );

}

// not sure we need this but we have the edge...
uint64_t UpVoteVote::getElectionGroup()
{
    return Edge::getTo(
        getDao().get_self(), 
        getId(), 
        links::UP_VOTE_VOTE
    ).getFromNode();
}

}

