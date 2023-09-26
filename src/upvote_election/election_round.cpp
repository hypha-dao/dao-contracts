#include "upvote_election/election_round.hpp"
#include "upvote_election/election_group.hpp"

#include <document_graph/edge.hpp>

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

    ElectionRound::ElectionRound(dao& dao, uint64_t id)
        : TypedDocument(dao, id, types::ELECTION_ROUND)
    {}

    ElectionRound::ElectionRound(dao& dao, uint64_t election_id, Data data)
        : TypedDocument(dao, types::ELECTION_ROUND)
    {
        EOS_CHECK(
            data.delegate_power >= 0,
            "Delegate Power must be greater or equal to 0"
        );

        validateStartDate(data.start_date);

        validateEndDate(data.start_date, data.end_date);

        auto cgs = convert(std::move(data));

        initializeDocument(dao, cgs);

        auto type = getType();

        if (type == round_types::CHIEF) {
            EOS_CHECK(
                !Edge::getIfExists(dao.get_self(), election_id, links::CHIEF_ROUND).first,
                "There is another chief delegate round already"
            )

            // TODO: I don't think we need to know what kinds of rounds there are.
            // We can estimate the maximum number of rounds but since any group may not have a winner
            // it could be over after round 1. 
                Edge(
                    getDao().get_self(),
                    getDao().get_self(),
                    election_id,
                    getId(),
                    links::CHIEF_ROUND
                );
        }
        else if (type == round_types::HEAD) {
            EOS_CHECK(
                !Edge::getIfExists(dao.get_self(), election_id, links::HEAD_ROUND).first,
                "There is another head delegate round already"
            )

                Edge(
                    getDao().get_self(),
                    getDao().get_self(),
                    election_id,
                    getId(),
                    links::HEAD_ROUND
                );

            // EOS_CHECK(
            //     getPassingCount() == 1,
            //     "There can be only 1 Head Delegate"
            // )
        }

        Edge(
            getDao().get_self(),
            getDao().get_self(),
            election_id,
            getId(),
            links::ROUND
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

    // bool ElectionRound::isCandidate(uint64_t accountId)
    // {
    //     return Edge::exists(
    //         getDao().get_self(),
    //         getId(),
    //         accountId,
    //         links::ROUND_CANDIDATE
    //     );
    // } 

    // int64_t ElectionRound::getAccountPower(uint64_t accountId)
    // {
    //     if (isCandidate(accountId)) {
    //         return std::max(int64_t{1}, getDelegatePower());
    //     }

    //     return 1;
    // }

    void ElectionRound::addElectionGroup(std::vector<uint64_t> accound_ids)
    {
        ElectionGroup electionGroup(
            getDao(),   
            getId(),    
            accound_ids,  
            ElectionGroupData{
                .member_count = accound_ids.size()
            }
        );

    }

}