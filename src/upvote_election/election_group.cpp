#include "upvote_election/election_group.hpp"
#include "upvote_election/upvote_election.hpp"
#include "upvote_election/common.hpp"
#include "upvote_election/up_vote_vote.hpp"

#include <document_graph/edge.hpp>

#include "dao.hpp"

namespace hypha::upvote_election {

using namespace upvote_election::common;


ElectionGroup::ElectionGroup(dao& dao, uint64_t id)
    : TypedDocument(dao, id,  types::ELECTION_GROUP)
{}

ElectionGroup::ElectionGroup(dao& dao, uint64_t round_id, std::vector<uint64_t> member_ids, Data data)

//ElectionGroup::ElectionGroup(dao& dao, uint64_t id, Data data)
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

std::optional<hypha::Member> ElectionGroup::getWinner() {
    return std::nullopt;
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

void ElectionGroup::vote(uint64_t from, uint64_t to)
{

    // get all existing vote edges
    std::vector<Edge> voteEdges = getDao().getGraph().getEdgesFrom(getId(), links::UP_VOTE_VOTE);
    
    // make sure both members are part of this group

    // get the member count

    // iterate over vote edges - calculate who has the most votes and find the "from" edge
    UpVoteVote* vote = nullptr;

    for (auto& edge : voteEdges) {

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