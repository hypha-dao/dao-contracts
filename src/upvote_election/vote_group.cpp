#include "upvote_election/vote_group.hpp"

#include "upvote_election/common.hpp"

#include "upvote_election/election_round.hpp"

#include "dao.hpp"

// this document keeps all votes a member has made in a round
// this will need to change since each member can only vote once in a round now, not as many as they want


namespace hypha::upvote_election {

using namespace upvote_election::common;

VoteGroup::VoteGroup(dao& dao, uint64_t id)
    : TypedDocument(dao, id, types::ELECTION_VOTE_GROUP)
{}

VoteGroup::VoteGroup(dao& dao, uint64_t memberId, Data data)
    : TypedDocument(dao, types::ELECTION_VOTE_GROUP)
{
    auto cgs = convert(std::move(data));

    initializeDocument(dao, cgs);

    Edge(
        getDao().get_self(),
        getDao().get_self(),
        memberId,
        getId(),
        links::ELECTION_GROUP
    );

    Edge(
        getDao().get_self(),
        getDao().get_self(),
        //memberId,
        getId(),
        getRoundID(),
        links::ROUND
        //name(getRoundID())
    );
}

uint64_t VoteGroup::getOwner()
{
    return Edge::getTo(
        getDao().get_self(), 
        getId(), 
        links::ELECTION_GROUP
    ).getFromNode();
}

std::optional<VoteGroup> VoteGroup::getFromRound(dao& dao, uint64_t roundId, uint64_t memberId)
{
    auto groups = dao.getGraph().getEdgesFrom(memberId, common::links::ELECTION_GROUP);

    for (auto& group : groups) {
        if (Edge::exists(dao.get_self(), group.getToNode(), roundId, links::ROUND)) {
            return VoteGroup(dao, group.getToNode());
        }
    }

    return std::nullopt;
}


// limit the members field to 1
// check the member is member of the same group
// This is one vote for multiple members in a large group - we won't have this anymore
// Vote group is tied to a single member

// We can change the meaning of this

void VoteGroup::castVotes(ElectionRound& round, std::vector<uint64_t> members)
{
    auto roundId = getRoundID();
    auto power = round.getAccountPower(getOwner());

    EOS_CHECK(
        roundId == round.getId(),
        "Missmatch between stored round id and round parameter"
    );

    auto contract = getDao().get_self();

    //We need to first erase previous votes if any
    // Because someone might change their vote
    auto prevVotes = getDao().getGraph().getEdgesFrom(getId(), links::VOTE);

    dao::election_vote_table elctn_t(contract, roundId);

    for (auto& edge : prevVotes) {
        auto memId = edge.getToNode();
        auto voteEntry = elctn_t.get(memId, "Member entry doesn't exists");
        elctn_t.modify(voteEntry, eosio::same_payer, [&](dao::ElectionVote& vote){
            vote.total_amount -= power;
        });
        edge.erase();
    }

    for (auto memId : members) {
        //Verify member is a candidate
        EOS_CHECK(
            round.isCandidate(memId),
            "Member must be a candidate to be voted"
        );

        auto voteIt = elctn_t.find(memId);

        if (voteIt != elctn_t.end()) {
            elctn_t.modify(voteIt, eosio::same_payer, [&](dao::ElectionVote& vote){
                vote.total_amount += power;
            });
        }
        else {
            elctn_t.emplace(contract, [&](dao::ElectionVote& vote){
                vote.total_amount += power;
                vote.account_id = memId;
            });
        }

        Edge(
            contract,
            contract,
            getId(),
            memId,
            links::VOTE
        );
    }
}

}

