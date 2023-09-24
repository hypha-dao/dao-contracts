#include <dao.hpp>
#include "upvote_election/upvote_election.hpp"
#include "upvote_election/election_round.hpp"
#include "upvote_election/vote_group.hpp"

#include <numeric>
#include <algorithm>

#include "badges/badges.hpp"

#include "upvote_election/common.hpp"
#include "recurring_activity.hpp"

#include <member.hpp>

#include <eosio/crypto.hpp>


#ifdef USE_UPVOTE_ELECTIONS

namespace hypha {

using upvote_election::UpvoteElection;
using upvote_election::UpvoteElectionData;
using upvote_election::ElectionRound;
using upvote_election::ElectionRoundData;
using upvote_election::VoteGroup;
using upvote_election::VoteGroupData;

namespace upvote_common = upvote_election::common;

static std::map<int64_t, ElectionRoundData> getRounds(ContentGroups& electionConfig, time_point& endDate) 
{    
    auto cw = ContentWrapper(electionConfig);
    //Store rounds groups sorted by their round id, in case they aren't sorted
    std::map<int64_t, ElectionRoundData> rounds;

    for (size_t i = 0; i < electionConfig.size(); ++i) {
        auto& group = electionConfig[i];
        if (cw.getGroupLabel(group) == upvote_common::groups::ROUND) {
            
            ElectionRoundData data;

            data.passing_count = cw.getOrFail(
                i,
                upvote_common::items::PASSING_AMOUNT
            ).second->getAs<int64_t>();

            EOS_CHECK(
                data.passing_count >= 1,
                "Passing count must be greater or equal to 1"
            )

            data.type = cw.getOrFail(
                i,
                upvote_common::items::ROUND_TYPE
            ).second->getAs<std::string>();

            EOS_CHECK(
                data.type == upvote_common::round_types::CHIEF || 
                data.type == upvote_common::round_types::HEAD || 
                data.type == upvote_common::round_types::DELEGATE,
                to_str("Invalid round type: ", data.type) 
            );

            auto roundId = cw.getOrFail(
                i,
                upvote_common::items::ROUND_ID
            ).second->getAs<int64_t>();
            
            //Let's check if the round was already defined
            EOS_CHECK(
                rounds.count(roundId) == 0,
                "Duplicated round entry in election_config"
            );

            auto duration = cw.getOrFail(
                i,
                upvote_common::items::ROUND_DURATION
            ).second->getAs<int64_t>();

            data.duration = duration;

            endDate += eosio::seconds(duration);

            rounds.insert({roundId, data});
        }
    }

    return rounds;
}

static constexpr int64_t getDelegatePower(int64_t roundId) {
    return roundId * 1 << roundId;
}

static void createRounds(dao& dao, UpvoteElection& election, std::map<int64_t, ElectionRoundData>& rounds, time_point startDate, time_point endDate) 
{
    std::unique_ptr<ElectionRound> prevRound;

    bool hasChief = false;

    for (auto& [roundId, roundData] : rounds) {
        
        roundData.delegate_power = getDelegatePower(roundId);

        roundData.start_date = startDate;

        startDate += eosio::seconds(roundData.duration);

        roundData.end_date = startDate;

        if (roundData.type == upvote_common::round_types::HEAD) {
            EOS_CHECK(
                prevRound && prevRound->getType() == upvote_common::round_types::CHIEF,
                "There has to be a Chief round previous to Head Delegate round"
            );
        }
        else {
            EOS_CHECK(
                !hasChief,
                to_str("Cannot create ", roundData.type, "type rounds after a Chief round")
            );

            hasChief = hasChief || roundData.type == upvote_common::round_types::CHIEF;
        }

        auto electionRound = std::make_unique<ElectionRound>(
            dao,
            election.getId(),
            roundData
        );

        if (prevRound) {
            
            EOS_CHECK(
                prevRound->getPassingCount() > electionRound->getPassingCount(),
                "Passing count has to be decremental"
            );

            prevRound->setNextRound(electionRound.get());
        }
        else {
            election.setStartRound(electionRound.get());
        }

        prevRound = std::move(electionRound);
    }

    EOS_CHECK(
        prevRound->getType() == upvote_common::round_types::CHIEF ||
        prevRound->getType() == upvote_common::round_types::HEAD,
        "Last round must be of type Chief or Head"
    )

    //Verify we have a chief round
    election.getChiefRound();

    //At the end both start date and end date should be the same
    EOS_CHECK(
        startDate == endDate,
        to_str("End date missmatch: ", startDate, " ", endDate)
    );
}

static void scheduleElectionUpdate(dao& dao, UpvoteElection& election, time_point date)
{
    if (date < eosio::current_time_point()) return;

    //Schedule a trx to close the proposal
    eosio::transaction trx;
    trx.actions.emplace_back(eosio::action(
        eosio::permission_level(dao.get_self(), eosio::name("active")),
        dao.get_self(),
        eosio::name("updateupvelc"),
        std::make_tuple(election.getId(), true)
    ));

    EOS_CHECK (
        date > eosio::current_time_point(),
        "Can only schedule for dates in the future"
    );

    constexpr auto aditionalDelaySec = 10;
    trx.delay_sec = (date - eosio::current_time_point()).to_seconds() + aditionalDelaySec;

    auto dhoSettings = dao.getSettingsDocument();

    auto nextID = dhoSettings->getSettingOrDefault("next_schedule_id", int64_t(0));

    trx.send(nextID, dao.get_self());

    dhoSettings->setSetting(Content{"next_schedule_id", nextID + 1});
}

static void assignDelegateBadges(dao& dao, uint64_t daoId, uint64_t electionId, const std::vector<uint64_t>& chiefDelegates, std::optional<uint64_t> headDelegate, eosio::transaction* trx = nullptr)
{
    //Generate proposals for each one of the delegates

    auto createAssignment = [&](const std::string& title, uint64_t member, uint64_t badge) {

        Member mem(dao, member);

        auto memAccount = mem.getAccount();

        auto action = eosio::action(
            eosio::permission_level(dao.get_self(), eosio::name("active")),
            dao.get_self(),
            eosio::name("propose"),
            std::make_tuple(
                daoId, 
                dao.get_self(), 
                common::ASSIGN_BADGE,
                ContentGroups{
                    ContentGroup{
                        Content{ CONTENT_GROUP_LABEL, DETAILS },
                        Content{ TITLE, title },
                        Content{ DESCRIPTION, title },
                        Content{ ASSIGNEE, memAccount },
                        Content{ BADGE_STRING, static_cast<int64_t>(badge) },
                        Content{ badges::common::items::UPVOTE_ELECTION_ID, static_cast<int64_t>(electionId) }
                }},
                true
            )
        );

        if (trx) {
            trx->actions.emplace_back(std::move(action));
        }
        else {
            action.send();
        }
    };

    auto chiefBadgeEdge = Edge::get(dao.get_self(), dao.getRootID(), upvote_common::links::CHIEF_DELEGATE);
    auto chiefBadge = TypedDocument::withType(dao, chiefBadgeEdge.getToNode(), common::BADGE_NAME);

    auto headBadgeEdge = Edge::get(dao.get_self(), dao.getRootID(), upvote_common::links::HEAD_DELEGATE);
    auto headBadge = TypedDocument::withType(dao, headBadgeEdge.getToNode(), common::BADGE_NAME);

    for (auto& chief : chiefDelegates) {
        createAssignment("Chief Delegate", chief, chiefBadge.getID());
    }

    if (headDelegate) {
        createAssignment("Head Delegate", *headDelegate, headBadge.getID());
    }
}

#ifdef EOS_BUILD
void dao::importelct(uint64_t dao_id, bool deferred)
{
    verifyDaoType(dao_id);
    checkAdminsAuth(dao_id);

    //Schedule a trx to archive and to crate new badges
    eosio::transaction trx;

    //Remove existing Head Delegate/Chief Delegate badges if any
    auto cleanBadgesOf = [&](const name& badgeEdge) {
        auto badgeId = Edge::get(get_self(), getRootID(), badgeEdge).getToNode();

        auto badgeAssignmentEdges = getGraph().getEdgesFrom(badgeId, common::ASSIGNMENT);

        //Filter out those that are not from the specified DAO
        auto badgeAssignments = std::vector<uint64_t>{};
        badgeAssignments.reserve(badgeAssignmentEdges.size());

        std::transform(
            badgeAssignmentEdges.begin(),
            badgeAssignmentEdges.end(),
            std::back_inserter(badgeAssignments),
            [](const Edge& edge){
                return edge.to_node;
            }
        );

        badgeAssignments.erase(
            std::remove_if(
                badgeAssignments.begin(),
                badgeAssignments.end(),
                [&](uint64_t id) { 
                    return !Edge::exists(get_self(), id, dao_id, common::DAO);
                }
            ),
            badgeAssignments.end()
        );

        for (auto& id : badgeAssignments) {
            auto doc = TypedDocument::withType(*this, id, common::ASSIGN_BADGE);

            auto cw = doc.getContentWrapper();

            cw.insertOrReplace(*cw.getGroupOrFail(SYSTEM), Content {
                "force_archive",
                1
            });

            cw.insertOrReplace(*cw.getGroupOrFail(DETAILS), Content {
                END_TIME,
                eosio::current_time_point()
            });

            doc.update();

            trx.actions.emplace_back(eosio::action(
                eosio::permission_level(get_self(), eosio::name("active")),
                get_self(),
                eosio::name("archiverecur"),
                std::make_tuple(id)
            ));
        }
    };

    cleanBadgesOf(upvote_common::links::HEAD_DELEGATE);
    cleanBadgesOf(upvote_common::links::CHIEF_DELEGATE);

    struct [[eosio::table("elect.state"), eosio::contract("genesis.eden")]] election_state_v0 {
        name lead_representative;
        std::vector<name> board;
        eosio::block_timestamp_type last_election_time;  
    };

    using election_state_singleton = eosio::singleton<"elect.state"_n, std::variant<election_state_v0>>;

    election_state_singleton election_s("genesis.eden"_n, 0);
    auto state = election_s.get();

    std::vector<uint64_t> chiefs;
    std::optional<uint64_t> head = 0;

    std::visit([&](election_state_v0& election){
        
        //If we want to prevent head del to be twice in the board array
        //we can use a simple find condition, for now it doesn't
        //matter if the head del is duplicated as we only assign one head 
        //variable
        // if (std::find(
        //     election.board.begin(), 
        //     election.board.end(), 
        //     election.lead_representative
        // ) == election.board.end()) {
        //     election.board.push_back(election.lead_representative);
        // }

        for (auto& mem : election.board) {
            if (mem) {
                auto member = getOrCreateMember(mem);

                //Make community member if not core or communnity member already
                if (!Member::isMember(*this, dao_id, mem) &&
                    !Member::isCommunityMember(*this, dao_id, mem)) {
                    Edge(get_self(), get_self(), dao_id, member.getID(), common::COMMEMBER);
                }

                if (mem == election.lead_representative) {
                    head = member.getID();
                }
                else {
                    chiefs.push_back(member.getID());
                }
            }
        }
    }, state);

    //Send election id as 0 meaning that election was done outside
    assignDelegateBadges(*this, dao_id, 0, chiefs, head, &trx);

    //Trigger all cleanup and propose actions
    if (deferred) {
        constexpr auto aditionalDelaySec = 5;
        trx.delay_sec = aditionalDelaySec;

        auto dhoSettings = getSettingsDocument();

        auto nextID = dhoSettings->getSettingOrDefault("next_schedule_id", int64_t(0));

        trx.send(nextID, get_self());

        dhoSettings->setSetting(Content{"next_schedule_id", nextID + 1});
    }
    else {
        for (auto& action : trx.actions) {
            action.send();
        }
    }
    
}
#endif


// Function to convert eosio::sha256 to a 64-bit uint
uint64_t sha256ToUint64(const eosio::checksum256& sha256Hash) {
    const uint64_t* hashData = reinterpret_cast<const uint64_t*>(sha256Hash.data());
    // Assuming sha256Hash is 32 bytes, take the first two 64-bit values
    return hashData[0] ^ hashData[1];
}

/// @brief Test random group creation
/// @param ids 
void dao::testgroupr1(uint32_t num_members, uint32_t seed) {
    require_auth(get_self());

    std::vector<uint64_t> ids(num_members); // Initialize a vector with 100 elements

    // Set each element's value to its index
    for (size_t i = 0; i < num_members; ++i) {
        ids[i] = static_cast<uint64_t>(i);
    }
    testgrouprng(ids, seed);

}

void dao::testgrouprng(std::vector<uint64_t> ids, uint32_t seed) {
    
    require_auth(get_self());
    
    // eosio::print(" ids: ");
    // for (const uint64_t& element : ids) {
    //     eosio::print(element, " ");
    // }

    auto randomIds = shuffleVector(ids, seed);

    // eosio::print(" random ids: ");
    // for (const uint64_t& element : randomIds) {
    //     eosio::print(element, " ");
    // }

    auto groups = createGroups(randomIds);

    // eosio::print(" groups: ");
    // for (uint32_t i = 0; i < groups.size(); ++i) {
    //     auto group = groups[i];
    //     eosio::print("group: ", i, "(", group.size(), "): ");
    //     for (const uint64_t& element : group) {
    //         eosio::print(element, " ");
    //     }
    // }

}

std::vector<uint64_t> dao::shuffleVector(std::vector<uint64_t>& ids, uint32_t seed) {
    struct MyRandomGenerator {
        using result_type = uint32_t;

        static constexpr result_type min() {
            return 0;  // Minimum possible random value
        }

        static constexpr result_type max() {
            return UINT32_MAX;  // Maximum possible random value
        }

        uint32_t seed;
        uint32_t value;

        MyRandomGenerator(uint32_t seed, uint32_t value) {
            this->seed = seed;
            this->value = value;
        }

        result_type operator()() {
            const uint64_t a = 1103515245;
            const uint64_t c = 12345;

            seed = (uint32_t)((a * seed + c) % 0x7fffffff);
            value = ((uint64_t)seed * max()) >> 31;

            return value;
        }
    };

    // Create a random number generator with the seed
    MyRandomGenerator rng(seed, 0);

    // Shuffle the vector using the RNG
    std::shuffle(ids.begin(), ids.end(), rng);

    return ids;

}

std::vector<std::vector<uint64_t>> dao::createGroups(const std::vector<uint64_t>& ids) {
    std::vector<std::vector<uint64_t>> groups;

    // Calculate the number of groups needed
    int numGroups = std::ceil(static_cast<double>(ids.size()) / 6);

    // Create the groups with an initial capacity of 6
    for (int i = 0; i < numGroups; ++i) {
        groups.push_back(std::vector<uint64_t>());
        groups.back().reserve(6);
    }

    // Initialize group iterators
    auto currentGroup = groups.begin();
    auto endGroup = groups.end();

    // Iterate over the IDs and distribute them into groups
    for (const auto& id : ids) {
        // Add the ID to the current group
        currentGroup->push_back(id);

        // Move to the next group (take turns)
        ++currentGroup;

        // Wrap back to the beginning of the groups if needed
        if (currentGroup == endGroup) {
            currentGroup = groups.begin();
        }
    }

    return groups;
}

void dao::startupelc(uint64_t election_id, bool reschedule) 
{
    UpvoteElection election(*this, election_id);

    auto status = election.getStatus();

    auto now = eosio::current_time_point();

    auto daoId = election.getDaoID();

    // 1 - randomize 
    std::vector<Edge> delegates = getGraph().getEdgesFrom(daoId, badges::common::links::DELEGATE);

    std::vector<uint64_t> delegateIDs(delegates.size());
    for (Edge& delegate : delegates) {
        delegateIDs.push_back(std::move(delegate.getToNode()));
    }
    
    // eosio::checksum256 checksum = /* Your checksum calculation here */;
    // Convert the checksum into a 32-bit integer seed
    uint32_t seed = 0; // checksumToUint64(checksum);


}

//Check if we need to update an ongoing elections status:
//upcoming -> ongoing
//ongoing -> finished
//or change the current round
void dao::updateupvelc(uint64_t election_id, bool reschedule)
{
    //eosio::require_auth();
    UpvoteElection election(*this, election_id);

    auto status = election.getStatus();

    auto now = eosio::current_time_point();

    auto daoId = election.getDaoID();

    auto setupCandidates = [&](uint64_t roundId, const std::vector<uint64_t>& members){
        election_vote_table elctn_t(get_self(), roundId);

        for (auto& memId : members) {
            elctn_t.emplace(get_self(), [memId](ElectionVote& vote) {
                vote.total_amount = 0;
                vote.account_id = memId;
            });
        }
    };

    if (status == upvote_common::upvote_status::UPCOMING) {
        auto start = election.getStartDate();

        //Let's update as we already started
        if (start <= now) {

            // startUpvoteElection();
            
            Edge::get(get_self(), daoId, election.getId(), upvote_common::links::UPCOMING_ELECTION).erase();
            
            Edge(get_self(), get_self(), daoId, election.getId(), upvote_common::links::ONGOING_ELECTION);

            election.setStatus(upvote_common::upvote_status::ONGOING);
            
            auto startRound = election.getStartRound();
            election.setCurrentRound(&startRound);

            //Setup all candidates
            // TODO: randomize this list
            auto delegates = getGraph().getEdgesFrom(daoId, badges::common::links::DELEGATE);

            std::vector<uint64_t> delegateIds;
            delegateIds.reserve(delegates.size());
            
            for (auto& delegate : delegates) {
                delegateIds.push_back(delegate.getToNode());
                // TODO
                // 
                // instead of adding delegates to the round, we add them to groups
                // and we add the groups to the round
                // A round and a group is the same data structure
                // only the round has groups, and the group has members.
                // 
                // 
                startRound.addCandidate(delegate.getToNode());
            }

            setupCandidates(startRound.getId(), delegateIds);

            scheduleElectionUpdate(*this, election, startRound.getEndDate());
        }
        else if (reschedule) {
            scheduleElectionUpdate(*this, election, start);
        }
    }
    else if (status == upvote_common::upvote_status::ONGOING) {
        auto currentRound = election.getCurrentRound();
        auto end = currentRound.getEndDate();
        if (end <= now) {
            
            auto winners = currentRound.getWinners();
            
            for (auto& winner : winners) {
                Edge(get_self(), get_self(), currentRound.getId(), winner, upvote_common::links::ROUND_WINNER);
            }

            Edge::get(get_self(), election_id, upvote_common::links::CURRENT_ROUND).erase();

            if (auto nextRound = currentRound.getNextRound()) {
                
                // set up the next round

                election.setCurrentRound(nextRound.get());

                setupCandidates(nextRound->getId(), winners);

                // round.addCandidate adds candidates
                for (auto& winner : winners) {
                    nextRound->addCandidate(winner);
                }

                scheduleElectionUpdate(*this, election, nextRound->getEndDate());
            }
            else {

                // The election has ended.

                Edge::get(get_self(), daoId, election.getId(), upvote_common::links::ONGOING_ELECTION).erase();

                Edge(get_self(), get_self(), daoId, election.getId(), upvote_common::links::PREVIOUS_ELECTION);
                
                //TODO: Setup head & chief badges
                
                if (currentRound.getType() == upvote_common::round_types::HEAD) {
                    //Get previous round for chief delegates
                    auto chiefs = election.getChiefRound().getWinners();

                    //Remove head delegate
                    chiefs.erase(
                        std::remove_if(
                            chiefs.begin(), 
                            chiefs.end(), 
                            [head = winners.at(0)](uint64_t id){ return id == head; }
                        ),
                        chiefs.end()
                    );

                    assignDelegateBadges(*this, daoId, election.getId(), chiefs, winners.at(0));
                }
                //No head delegate
                else {
                    assignDelegateBadges(*this, daoId, election.getId(), winners, std::nullopt);
                }
                

                election.setStatus(upvote_common::upvote_status::FINISHED);
            }
        }
        else if (reschedule){
            scheduleElectionUpdate(*this, election, end);
        }
    }
    else {
        EOS_CHECK(
            false,
            "Election already finished or canceled"
        )   
    }

    election.update();
}

void dao::cancelupvelc(uint64_t election_id)
{
    UpvoteElection election(*this, election_id);

    auto daoId = election.getDaoID();

    checkAdminsAuth(daoId);

    auto status = election.getStatus();

    bool isOngoing = status == upvote_common::upvote_status::ONGOING;

    EOS_CHECK(
        isOngoing ||
        status == upvote_common::upvote_status::UPCOMING,
        to_str("Cannot cancel election with ", status, " status")
    );

    if (isOngoing) {
        election.getCurrentRound().erase();
        Edge::get(get_self(), daoId, election.getId(), upvote_common::links::ONGOING_ELECTION).erase();
    } 
    else {
        Edge::get(get_self(), daoId, election.getId(), upvote_common::links::UPCOMING_ELECTION).erase();
    }

    election.setStatus(upvote_common::upvote_status::CANCELED);

    election.update();
}

// 
// void VoteGroup::castvote(ElectionRound& round, VoteGroup& group, name voter, uint64_t member) {
//     eosio::require_auth(voter);

//     //Verify round_id is the same as the current round
//     ElectionRound round(*this, round_id);

//     UpvoteElection election = round.getElection();

//     //Current round has to be defined
//     auto currentRound = election.getCurrentRound();

//     EOS_CHECK(
//         currentRound.getId() == round_id,
//         "You can only vote on the current round"
//     );

//     /// 
//     /// Get ElectionVoteGroup document for this memeber and this round

// }

void dao::castelctnvote(uint64_t round_id, name voter, std::vector<uint64_t> voted)
{
    eosio::require_auth(voter);

    //TODO: Cancel existing Delegate badges

    //Verify round_id is the same as the current round
    ElectionRound round(*this, round_id);

    UpvoteElection election = round.getElection();

    //Current round has to be defined
    auto currentRound = election.getCurrentRound();

    EOS_CHECK(
        currentRound.getId() == round_id,
        "You can only vote on the current round"
    );

    auto memberId = getMemberID(voter);

    // TODO: We can probably change this check - we get the 
    // auto groupId = getGroup(voter)
    // 

    EOS_CHECK(
        badges::hasVoterBadge(*this, election.getDaoID(), memberId) ||
        //Just enable voting to candidates if the round is not the first one
        (currentRound.isCandidate(memberId) && currentRound.getDelegatePower() > 0),
        "Only registered voters are allowed to perform this action"
    );

    auto votedEdge = Edge::getOrNew(get_self(), get_self(), round_id, memberId, eosio::name("voted"));

    if (voted.empty()) {
        votedEdge.erase();
        return;
    }

    if (auto voteGroup = VoteGroup::getFromRound(*this, round_id, memberId)) {
        voteGroup->castVotes(round, std::move(voted));
    }
    else {
        VoteGroup group(*this, memberId, VoteGroupData{
            .round_id = static_cast<int64_t>(round_id)
        });

        group.castVotes(round, std::move(voted));
    }
}

/*
election_config: [
    [
        { "label": "content_group_label", "value": ["string", "details"] },
        //Upvote start date-time
        { "label": "upvote_start_date_time", "value": ["timepoint", "..."] },
        //How much will chief/head Delegates hold their badges after upvote is finished
        { "label": "upvote_duration", "value": ["int64", 7776000] },
    ],
    [
        { "label":, "content_group_label", "value": ["string", "round"] },
        //Duration in seconds
        { "label": "duration", "value": ["int64", 9000] },
        //One of "delegate"|"chief"|"head"
        { "label": "type", "value": ["string", "delegate"] }, 
        //Number that indicates the order of this round, should start at 0 and increment by 1 for each round
        { "label": "round_id", "value": ["int64", 0] },
        //Number of candidates that will pass after the round is over
        { "label": "passing_count", "value": ["int64", 50] }

    ],
    [
        { "label":, "content_group_label", "value": ["string", "round"] },
        { "label": "duration", "value": ["int64", 9000] }, //Duration in seconds
        { "label": "type", "value": ["string", "chief"] },
        { "label": "round_id", "value": ["int64", 1] },
        { "label": "passing_count", "value": ["int64", 5] }
    ],
    [
        { "label":, "content_group_label", "value": ["string", "round"] },
        { "label": "duration", "value": ["int64", 9000] }, //Duration in seconds
        { "label": "type", "value": ["string", "head"] },
        { "label": "round_id", "value": ["int64", 1] },
        { "label": "passing_count", "value": ["int64", 1] }
    ]
]
*/
void dao::createupvelc(uint64_t dao_id, ContentGroups& election_config)
{
    verifyDaoType(dao_id);
    checkAdminsAuth(dao_id);

    auto cw = ContentWrapper(election_config);

    auto startDate = cw.getOrFail(
        DETAILS, 
        upvote_common::items::UPVOTE_STARTDATE
    )->getAs<time_point>();

    auto duration = cw.getOrFail(
        DETAILS,
        upvote_common::items::UPVOTE_DURATION
    )->getAs<int64_t>();

    time_point endDate = startDate;

    //Will calculate also what is the endDate for the upvote election
    auto rounds = getRounds(election_config, endDate);

    UpvoteElection upvoteElection(*this, dao_id, UpvoteElectionData{
        .start_date = startDate,
        .end_date = endDate,
        .status = upvote_common::upvote_status::UPCOMING,
        .duration = duration
    });

    createRounds(*this, upvoteElection, rounds, startDate, endDate);

    scheduleElectionUpdate(*this, upvoteElection, startDate);
}

void dao::editupvelc(uint64_t election_id, ContentGroups& election_config)
{
    //TODO: Change existing delegate badges
    //end time according to the new upvoteElection.getEndDate(), 
    //also reschedule for archive
    UpvoteElection upvoteElection(*this, election_id);
    auto daoId = upvoteElection.getDaoID();
    
    verifyDaoType(daoId);
    checkAdminsAuth(daoId);

    EOS_CHECK(
        upvoteElection.getStatus() == upvote_common::upvote_status::UPCOMING,
        "Only upcoming elections can be edited"
    );

    for (auto& rounds : upvoteElection.getRounds()) {
        rounds.erase();
    }

    auto cw = ContentWrapper(election_config);

    auto startDate = cw.getOrFail(
        DETAILS, 
        upvote_common::items::UPVOTE_STARTDATE
    )->getAs<time_point>();

    auto duration = cw.getOrFail(
        DETAILS,
        upvote_common::items::UPVOTE_DURATION
    )->getAs<int64_t>();

    time_point endDate = startDate;

    //Will calculate also what is the endDate for the upvote election
    auto rounds = getRounds(election_config, endDate);

    upvoteElection.setStartDate(startDate);
    upvoteElection.setEndDate(endDate);
    upvoteElection.setDuration(duration);
    upvoteElection.validate();
    upvoteElection.update();

    createRounds(*this, upvoteElection, rounds, startDate, endDate);

    scheduleElectionUpdate(*this, upvoteElection, startDate);
}

}


#endif