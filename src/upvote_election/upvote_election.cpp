#include "upvote_election/upvote_election.hpp"

#include <document_graph/edge.hpp>

#include "upvote_election/common.hpp"
#include "upvote_election/election_round.hpp"
#include "upvote_election/from_bin.hpp"

#include <dao.hpp>

namespace hypha::upvote_election {

    using namespace upvote_election::common;

    UpvoteElection::UpvoteElection(dao& dao, uint64_t id)
        : TypedDocument(dao, id, types::UPVOTE_ELECTION)
    {}

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

        while ((next = rounds.back().getNextRound())) {
            rounds.emplace_back(getDao(), next->getId());
        }

        return rounds;
    }

    uint64_t UpvoteElection::addRound() 
    {

        eosio::time_point startDate;
        auto roundDuration = getRoundDuration();
        
        std::unique_ptr<ElectionRound> currentRound;
        auto [exists, currentRoundEdge] = Edge::getIfExists(getDao().get_self(), getId(), links::CURRENT_ROUND);
        if (!exists) {
            startDate = getStartDate();
        } else {
            currentRound = std::make_unique<ElectionRound>(
                getDao(),
                currentRoundEdge.getToNode()
            );
            
            startDate = currentRound->getEndDate();
        }
        auto endDate = startDate + eosio::seconds(roundDuration);

        ElectionRoundData roundData{
            .type = round_types::DELEGATE,
            .start_date = startDate,
            .end_date = endDate,
            .round_duration = roundDuration
        };

        ElectionRound round(
            getDao(),
            getId(),
            roundData
        );

        setEndDate(std::move(endDate));
        setEndDate(endDate);
        update();

        if (!exists) {
            eosio::print(" new round - exists: ", exists);
            setStartRound(&round);
        } else {
            eosio::print(" round exists: ", exists, " ", currentRound->getId());

            currentRound->setNextRound(&round);
        }
        return round.getId();
    }

    

    void UpvoteElection::setStartRound(ElectionRound* startRound) const
    {

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

    void UpvoteElection::updateSeed(eosio::input_stream& bytes)
        // This method is based on Edenia code
        // Modified to work with older eosio cdt libraries.
    {
        auto electionStartDate = getStartDate();
        eosio::time_point_sec electionStartDateSec = electionStartDate;

        // TODO: Change start and end time for the BTC block to the actual start and end times
        eosio::time_point_sec endTimeSec = electionStartDateSec;
        eosio::time_point_sec startTimeSec = electionStartDateSec - 86400; // 24 h before
        
        auto current = getSeed();

        eosio::check(bytes.remaining() >= 80, "Stream overrun");
        auto hash1 = eosio::sha256(bytes.pos, 80);
        auto hash2 = eosio::sha256(reinterpret_cast<char*>(hash1.extract_as_byte_array().data()), 32);
        // NOTE: OG Edenia code! I guess reverse because of endian issues.
        std::reverse((unsigned char*)&hash2, (unsigned char*)(&hash2 + 1));
        eosio::check(hash2 < current, "New seed block must have greater POW than previous seed.");
        
        uint32_t utc_seconds;
        bytes.skip(4 + 32 + 32);
        eosio::from_bin(utc_seconds, bytes);
        eosio::time_point_sec block_time(utc_seconds);
        bytes.skip(4 + 4);
        eosio::check(block_time >= endTimeSec, "Seed block is too early");
        eosio::check(block_time < startTimeSec, "Seed block is too late");

        setSeed(hash2);
        update();
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
            Edge::get(getDao().get_self(), getId(), links::ELECTION_ROUND).getToNode()
        );
    }

    void UpvoteElection::validate()
    {
        auto startDate = getStartDate();
        // validateStartDate(startDate);
    }

}