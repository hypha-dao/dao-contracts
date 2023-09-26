#include "upvote_election/upvote_election.hpp"

#include <document_graph/edge.hpp>

#include "upvote_election/common.hpp"
#include "upvote_election/election_round.hpp"

#include <dao.hpp>

namespace hypha::upvote_election {

using namespace upvote_election::common;

UpvoteElection::UpvoteElection(dao& dao, uint64_t id)
    : TypedDocument(dao, id, types::UPVOTE_ELECTION)
{}

static void validateStartDate(const time_point& startDate)
{
    //Only valid if start date is in the future
    EOS_CHECK(
        startDate > eosio::current_time_point(),
        _s("Election start date must be in the future")
    )
}

static void validateEndDate(const time_point& startDate, const time_point& endDate)
{
    //Only valid if start date is in the future
    EOS_CHECK(
        endDate > startDate,
        to_str("End date must happen after start date: ", startDate, " to ", endDate)
    );
}

UpvoteElection::UpvoteElection(dao& dao, uint64_t dao_id, Data data)
    : TypedDocument(dao, types::UPVOTE_ELECTION)
{
    auto cgs = convert(std::move(data));

    initializeDocument(dao, cgs);

    validate();

    //Also check there are no ongoing elections
    //TODO: Might want to check if there is an ongoing election, it will
    //finish before the start date so then it is valid
    EOS_CHECK(
        dao.getGraph().getEdgesFrom(dao_id, links::ONGOING_ELECTION).empty(),
        _s("There is an ongoing election for this DAO")
    )

    //Validate that there are no other upcoming elections
    EOS_CHECK(
        dao.getGraph().getEdgesFrom(dao_id, links::UPCOMING_ELECTION).empty(),
        _s("DAOs can only have 1 upcoming election")
    )

    Edge(
        getDao().get_self(),
        getDao().get_self(),
        getId(),
        dao_id,
        hypha::common::DAO
    );

    Edge(
        getDao().get_self(),
        getDao().get_self(),
        dao_id,
        getId(),
        links::ELECTION
    );

    Edge(
        getDao().get_self(),
        getDao().get_self(),
        dao_id,
        getId(),
        links::UPCOMING_ELECTION
    );
}

uint64_t UpvoteElection::getDaoID() const
{
    return Edge::get(
        getDao().get_self(),
        getId(),
        hypha::common::DAO
    ).getToNode();
}

UpvoteElection UpvoteElection::getUpcomingElection(dao& dao, uint64_t dao_id)
{
    return UpvoteElection(
        dao,
        Edge::get(dao.get_self(), dao_id, links::UPCOMING_ELECTION).getToNode()
    );
}

std::vector<ElectionRound> UpvoteElection::getRounds() const
{
    std::vector<ElectionRound> rounds;
    
    auto start = Edge::get(
        getDao().get_self(),
        getId(),
        links::START_ROUND
    ).getToNode();

    rounds.emplace_back(getDao(), start);

    std::unique_ptr<ElectionRound> next;

    while((next = rounds.back().getNextRound())) {
        rounds.emplace_back(getDao(), next->getId());
    }

    // a getter should never throw?!
    // EOS_CHECK(
    //     rounds.size() >= 2,
    //     "There has to be at least 2 election rounds"
    // );

    return rounds;
}

void UpvoteElection::setStartRound(ElectionRound* startRound) const
{
   //TODO: Check if there is no start round already

   Edge(
       getDao().get_self(),  
       getDao().get_self(),
       getId(),
       startRound->getId(),
       links::START_ROUND
   );
}

void UpvoteElection::setCurrentRound(ElectionRound* currenttRound) const
{
    Edge(
       getDao().get_self(),  
       getDao().get_self(),
       getId(),
       currenttRound->getId(),
       links::CURRENT_ROUND
   );
}

ElectionRound UpvoteElection::getStartRound() const
{
    return ElectionRound(
        getDao(),
        Edge::get(getDao().get_self(), getId(), links::START_ROUND).getToNode()
    );
}

ElectionRound UpvoteElection::getCurrentRound() const
{
    return ElectionRound(
        getDao(),
        Edge::get(getDao().get_self(), getId(), links::CURRENT_ROUND).getToNode()
    );
}

ElectionRound UpvoteElection::getChiefRound() const
{
    return ElectionRound(
        getDao(),
        Edge::get(getDao().get_self(), getId(), links::CHIEF_ROUND).getToNode()
    );
}

void UpvoteElection::validate() 
{
    auto startDate = getStartDate();
    validateStartDate(startDate);
    validateEndDate(startDate, getEndDate());
}

}