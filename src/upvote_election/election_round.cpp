#include "upvote_election/election_round.hpp"
#include "upvote_election/election_group.hpp"

#include <document_graph/edge.hpp>
// #include <document_graph/content_wrapper.hpp>
// #include <document_graph/content.hpp>

#include "upvote_election/common.hpp"

#include "upvote_election/upvote_election.hpp"

#include "dao.hpp"

namespace hypha::upvote_election {

    // SET VALUE EXAMPLE - use accessors magically created with the DECLARE_DOCUMENT Macro
    //
        // DECLARE_DOCUMENT(
        //     Data,
        //     PROPERTY(credit, eosio::asset, Credit, USE_GETSET),
        //     PROPERTY(type, std::string, Type, USE_GETSET)
        // )
    ////
    //// ... code
    //     setCredit(std::move(newCredit)); // credit is asset type
    //     update();
    //// ...
    // get 
    //     float offerDisc = 1.0f - offer.getDiscountPercentage() / 10000.f;
    //
    //
    // the magical setters are defined to acess the content group details and get/set
    // they don't seem to call update, so update needs to be called.
    //
    // #define USE_GET_SET_DEC(name, type, getSet)\
    // const type& get##getSet() {\
    //     return getContentWrapper()\
    //           .getOrFail(DETAILS, #name)\
    //           ->getAs<type>();\
    // }\
    // void set##getSet(type value) {\
    //     auto cw = getContentWrapper();\
    //     cw.insertOrReplace(\
    //         *cw.getGroupOrFail(DETAILS),\
    //         Content{ #name, std::move(value) }\
    //     );\
    // }



    using namespace upvote_election::common;

    // static void validateStartDate(const time_point& startDate)
    // {
    //     //Only valid if start date is in the future
    //     EOS_CHECK(
    //         startDate > eosio::current_time_point(),
    //         _s("Election start date must be in the future")
    //     )
    // }

    // static void validateEndDate(const time_point& startDate, const time_point& endDate)
    // {
    //     //Only valid if start date is in the future
    //     EOS_CHECK(
    //         endDate > startDate,
    //         to_str("End date must happen after start date: ", startDate, " to ", endDate)
    //     );
    // }

    ElectionRound::ElectionRound(dao& dao, uint64_t id)
        : TypedDocument(dao, id, types::ELECTION_ROUND)
    {}

    ElectionRound::ElectionRound(dao& dao, uint64_t election_id, Data data)
        : TypedDocument(dao, types::ELECTION_ROUND)
    {
        // EOS_CHECK(
        //     data.delegate_power >= 0,
        //     "Delegate Power must be greater or equal to 0"
        // );

        // validateStartDate(data.start_date);

        // validateEndDate(data.start_date, data.end_date);

        auto cgs = convert(std::move(data));

        initializeDocument(dao, cgs);

        auto type = getType();

        // eosio::print(" add edge - election id: ", election_id, " -> round id: ", getId(), " name: ", links::ELECTION_ROUND);

        Edge(
            getDao().get_self(),
            getDao().get_self(),
            election_id,
            getId(),
            links::ELECTION_ROUND
        );

        Edge(
            getDao().get_self(),
            getDao().get_self(),
            getId(),
            election_id,
            links::ELECTION
        );
    }

    UpvoteElection ElectionRound::getElection() {
        return UpvoteElection(
            getDao(),
            Edge::get(getDao().get_self(), getId(), links::ELECTION).getToNode()
        );
    }

    std::vector<uint64_t> ElectionRound::getWinners()
    {
        std::vector<uint64_t> winners;

        std::vector<Edge> groupEdges = getDao().getGraph().getEdgesFrom(getId(), links::ELECTION_GROUP_LINK);
        for (auto& edge : groupEdges) {
            ElectionGroup group(getDao(), edge.getToNode());
            int64_t winner = group.getWinner();
            if (winner != -1) {
                winners.push_back(winner);
            }
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

    void ElectionRound::addElectionGroup(std::vector<uint64_t> accound_ids, int64_t winner)
    {
        ElectionGroup electionGroup(
            getDao(),   
            getId(),    
            accound_ids,  
            ElectionGroupData{
                .member_count = accound_ids.size(),
                .winner = winner
            }
        );

        if (winner != -1) {
            Edge(getDao().get_self(), getDao().get_self(), electionGroup.getId(), winner, links::GROUP_WINNER);
        }
    }

}