#include "upvote_election/election_group.hpp"
#include "upvote_election/upvote_election.hpp"
#include "upvote_election/common.hpp"

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

    //EOS_CHECK(member_ids.size() <= 6, "max 6 members in group"); // the last group is up to 11 members?!

    eosio::print(" adding election group ", getId(), " to round ", round_id);

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

    eosio::print(" link from  ", round_id, " to ", getId(), " ");

    // Create edge from the round to this group
    Edge(
        getDao().get_self(),
        getDao().get_self(),
        round_id,
        getId(),
        links::ELECTION_GROUP_LINK
    );

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
    
    // get the member count
    auto memcount = getMemberCount();
    uint64_t votesToWin = memcount * 2 / 3 + 1;

    // iterate over vote edges - calculate who has the most votes and find the "from" edge
    std::unordered_map<int64_t, int> voteCount;
    std::unordered_map<int64_t, bool> selfVotes;

    bool existingVote = false;
    int64_t majorityWinner = -1;

    for (auto& edge : voteEdges) {
        UpVoteVote upvote(getDao(), edge.getToNode());

        int64_t voted_id = upvote.getVotedId();
        int64_t voter_id = upvote.getVoterId();

        // If the voter has already voted before, we change their 
        // vote to the new voted "to"
        if (voter_id == from) {
            existingVote = true;
            upvote.setVotedId(to);
            upvote.update();
            voted_id = to;
        }

        voteCount[voted_id]++;

        // check for winner
        if (voteCount[voted_id] >= votesToWin) {
            majorityWinner = voted_id;
        }
        
        // keep track of self votes
        if (voted_id == voter_id) {
            selfVotes[voted_id] = true;
        }
    }

    if (!existingVote) {
        if (from == to) {
            selfVotes[from] = true;
        }
        // Create a new vote
        UpVoteVote vote(getDao(), getId(), UpVoteVoteData{
            .voter_id = from,
            .voted_id = to
        });

        voteCount[to]++;

        // check for winner
        if (voteCount[to] >= votesToWin) {
            majorityWinner = to;
        }
    }
    std::vector<Edge> winnerEdges = getDao().getGraph().getEdgesFrom(getId(), links::GROUP_WINNER);
    if (winnerEdges.size() > 0) {
        winnerEdges[0].erase();
    }

    if (majorityWinner > 0 && selfVotes[majorityWinner]) {
        setWinner(majorityWinner);
        Edge(getDao().get_self(), getDao().get_self(), getId(), majorityWinner, links::GROUP_WINNER);
    } else {
        setWinner(-1);
    }
    update();
    
}



}