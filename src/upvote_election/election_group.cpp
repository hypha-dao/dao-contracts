#include "upvote_election/election_group.hpp"
#include "upvote_election/upvote_election.hpp"
#include "upvote_election/common.hpp"

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

/*
UpvoteElection ElectionRound::getElection() {
    return UpvoteElection(
        getDao(),
        Edge::get(getDao().get_self(), getId(), links::ELECTION).getToNode()
    );
}

std::vector<uint64_t> ElectionRound::getWinners()
{
    std::vector<uint64_t> winners;

    dao::election_vote_table elctn_t(getDao().get_self(), getId());

    auto byAmount = elctn_t.get_index<"byamount"_n>();

    auto beg = byAmount.rbegin();

    auto idx = 0;

    while (idx < getPassingCount() && beg != byAmount.rend()) {
        winners.push_back(beg->account_id);
        ++idx;
        ++beg;
    }

    return winners;
}

void ElectionRound::setNextRound(ElectionRound* nextRound) const
{
    EOS_CHECK(
        !getNextRound(),
        "Election Round already has next round set"
    );

    Edge(
        getDao().get_self(),
        getDao().get_self(),
        getId(),
        nextRound->getId(),
        links::NEXT_ROUND
    );
}

std::unique_ptr<ElectionRound> ElectionRound::getNextRound() const
{
    if (auto [exists, edge] = Edge::getIfExists(getDao().get_self(), getId(), links::NEXT_ROUND);
        exists) {
        return std::make_unique<ElectionRound>(getDao(), edge.getToNode());    
    }
    
    return {};
}

bool ElectionRound::isCandidate(uint64_t accountId)
{
    return Edge::exists(
        getDao().get_self(),
        getId(),
        accountId,
        links::ROUND_CANDIDATE
    );
} 

int64_t ElectionRound::getAccountPower(uint64_t accountId)
{
    if (isCandidate(accountId)) {
        return std::max(int64_t{1}, getDelegatePower());
    }

    return 1;
}

void ElectionRound::addCandidate(uint64_t accountId)
{
    Edge(
        getDao().get_self(),
        getDao().get_self(),
        getId(),
        accountId,
        links::ROUND_CANDIDATE
    );
}
*/

}