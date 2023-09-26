#include "upvote_election/election_group.hpp"
#include "upvote_election/upvote_election.hpp"
#include "upvote_election/common.hpp"
#include "upvote_election/up_vote_vote.hpp"

#include <document_graph/edge.hpp>

#include "dao.hpp"
#include <unordered_map> 

namespace hypha::upvote_election {

using namespace upvote_election::common;


ElectionGroup::ElectionGroup(dao& dao, uint64_t id)
    : TypedDocument(dao, id,  types::ELECTION_GROUP)
{}

ElectionGroup::ElectionGroup(dao& dao, uint64_t round_id, std::vector<uint64_t> member_ids, Data data)
    : TypedDocument(dao, types::ELECTION_GROUP)
{

    auto cgs = convert(std::move(data));

    initializeDocument(dao, cgs);

    EOS_CHECK(member_ids.size() <= 6, "max 6 members in group");

    // create edges to all members (max 6)
    for (size_t i = 0; i < member_ids.size(); ++i) {
        Edge(
            getDao().get_self(),
            getDao().get_self(),
            getId(),                        // from this
            member_ids[i],                  // to the member
            links::ELECTION_ROUND_MEMBER
        );
    }

    
    // Create edge from the round to this group
    Edge(
        getDao().get_self(),
        getDao().get_self(),
        round_id,
        getId(),
        links::ELECTION_GROUP_LINK
    );

    // I don't think we need 2 way links everywhere?!
    // Edge(
    //     getDao().get_self(),
    //     getDao().get_self(),
    //     getId(),
    //     election_id,
    //     links::ELECTION
    // );
}

bool ElectionGroup::isElectionRoundMember(uint64_t accountId)
{
    return Edge::exists(
        getDao().get_self(),
        getId(),
        accountId,
        links::ELECTION_ROUND_MEMBER
    );
} 

void ElectionGroup::vote(int64_t from, int64_t to)
{

    // get all existing vote edges
    std::vector<Edge> voteEdges = getDao().getGraph().getEdgesFrom(getId(), links::UP_VOTE_VOTE);
    
    // make sure both members are part of this group

    // get the member count
    auto memcount = getMemberCount();
    uint64_t votesToWin = memcount * 2 / 3 + 1;

    // iterate over vote edges - calculate who has the most votes and find the "from" edge
    std::unordered_map<int64_t, int> voteCount;

    bool existingVote = false;
    bool hasWinner = false;
    int64_t winner = 0;

    for (auto& edge : voteEdges) {
        UpVoteVote upvote(getDao(), edge.getToNode());

        int64_t voted_id = upvote.getVotedId();
        int64_t voter_id = upvote.getVoterId();

        if (voteCount.find(voted_id) == voteCount.end()) {
            voteCount[voted_id] = 1;
        } else {
            voteCount[voted_id] = voteCount[voted_id] + 1;
        }

        // check for winner
        if (voteCount[voted_id] >= votesToWin) {
            hasWinner = true;
            winner = voted_id;
        }

        // check if we need to change the vote
        if (voter_id == from) {
            // change vote
            existingVote = true;
            upvote.setVotedId(to);
            upvote.update();
        }
    }

    if (!existingVote) {
        // Create a new vote
        UpVoteVote vote(getDao(), getId(), UpVoteVoteData{
            .voter_id = from,
            .voted_id = to
        });
        // edge case - can't happen in upvote elections
        // if (votesToWin == 1) {
        //     hasWinner = true;
        //     winner = to;
        // }
    }

    if (hasWinner) {
        setWinner(winner);
        update();
    }
    

    // if from edge, delete the edge and create a new upvote doc - or change its voted field and save it back?
    // if no from edge, create a new UpVoteVote doc

    


    // if (exists) {
    //     vote = UpVoteVote(getDao(), edge.getToNode());
    // } else {
    //     vote = UpVoteVote()
    // }

    // Edge(
    //     getDao().get_self(),
    //     getDao().get_self(),
    //     getId(),
    //     nextRound->getId(),
    //     links::NEXT_ROUND
    // );
}



}