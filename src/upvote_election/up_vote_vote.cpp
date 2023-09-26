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

// std::optional<UpVoteVote> UpVoteVote::getFromRound(dao& dao, uint64_t roundId, uint64_t memberId)
// {
//     auto groups = dao.getGraph().getEdgesFrom(memberId, common::links::ELECTION_GROUP);

//     for (auto& group : groups) {
//         if (Edge::exists(dao.get_self(), group.getToNode(), roundId, links::ROUND)) {
//             return UpVoteVote(dao, group.getToNode());
//         }
//     }

//     return std::nullopt;
// }


// limit the members field to 1
// check the member is member of the same group
// This is one vote for multiple members in a large group - we won't have this anymore
// Vote group is tied to a single member

// We can change the meaning of this

// void UpVoteVote::castVotes(ElectionRound& round, std::vector<uint64_t> members)
// {
//     auto roundId = getRoundID();
//     auto power = round.getAccountPower(getOwner());

//     EOS_CHECK(
//         roundId == round.getId(),
//         "Missmatch between stored round id and round parameter"
//     );

//     auto contract = getDao().get_self();

//     //We need to first erase previous votes if any
//     // Because someone might change their vote
//     auto prevVotes = getDao().getGraph().getEdgesFrom(getId(), links::VOTE);

//     dao::election_vote_table elctn_t(contract, roundId);

//     for (auto& edge : prevVotes) {
//         auto memId = edge.getToNode();
//         auto voteEntry = elctn_t.get(memId, "Member entry doesn't exists");
//         elctn_t.modify(voteEntry, eosio::same_payer, [&](dao::ElectionVote& vote){
//             vote.total_amount -= power;
//         });
//         edge.erase();
//     }

//     for (auto memId : members) {
//         //Verify member is a candidate
//         EOS_CHECK(
//             round.isCandidate(memId),
//             "Member must be a candidate to be voted"
//         );

//         auto voteIt = elctn_t.find(memId);

//         if (voteIt != elctn_t.end()) {
//             elctn_t.modify(voteIt, eosio::same_payer, [&](dao::ElectionVote& vote){
//                 vote.total_amount += power;
//             });
//         }
//         else {
//             elctn_t.emplace(contract, [&](dao::ElectionVote& vote){
//                 vote.total_amount += power;
//                 vote.account_id = memId;
//             });
//         }

//         Edge(
//             contract,
//             contract,
//             getId(),
//             memId,
//             links::VOTE
//         );
//     }
// }

}

