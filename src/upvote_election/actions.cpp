#include <dao.hpp>
#include "upvote_election/upvote_election.hpp"
#include "upvote_election/election_round.hpp"
#include "upvote_election/election_group.hpp"
#include "upvote_election/up_vote_vote.hpp"

#include <numeric>
#include <algorithm>

#include "badges/badges.hpp"

#include "upvote_election/common.hpp"
#include <upvote_election/random_number_generator.hpp>

#include "recurring_activity.hpp"

#include <member.hpp>

#include <eosio/crypto.hpp>


#ifdef USE_UPVOTE_ELECTIONS

namespace hypha {

    using upvote_election::UpvoteElection;
    using upvote_election::UpvoteElectionData;
    using upvote_election::ElectionRound;
    using upvote_election::ElectionRoundData;
    using upvote_election::ElectionGroup;
    using upvote_election::ElectionGroupData;
    using upvote_election::UpVoteVote;
    using upvote_election::UpVoteVoteData;

    namespace upvote_common = upvote_election::common;


    // we capture a checksum256 seed, then convert it to a uint32_t to use for our rng
    // we always store the last seed in the election document, so we can keep using it
    // our rng changes the seed on every operation
    static uint32_t checksum256_to_uint32(const eosio::checksum256& checksum) {
        // Hash the data using SHA-256
        auto hash = eosio::sha256(reinterpret_cast<char*>(checksum.extract_as_byte_array().data()), 32);

        // Extract the first 4 bytes and convert to uint32_t
        uint32_t result = 0;
        for (int i = 0; i < 4; ++i) {
            result = (result << 8) | hash.extract_as_byte_array()[i];
        }

        return result;
    }

    static uint32_t int_pow(uint32_t base, uint32_t exponent)
    {
        uint32_t result = 1;
        for (uint32_t i = 0; i < exponent; ++i)
        {
            result *= base;
        }
        return result;
    }

    // From Eden Code
    static uint32_t int_root(uint32_t x, uint32_t y)
    {
        // find z, such that $z^y \le x < (z+1)^y$
        //
        // hard coded limits based on the election constraints
        uint32_t low = 0, high = 12;
        while (high - low > 1)
        {
            uint32_t mid = (high + low) / 2;
            if (x < int_pow(mid, y))
            {
                high = mid;
            }
            else
            {
                low = mid;
            }
        }
        return low;
    }

    static std::size_t count_rounds(uint32_t num_members)
    {
        std::size_t result = 1;
        for (uint32_t i = 12; i <= num_members; i *= 4)
        {
            ++result;
        }
        return result;
    }

    static std::vector<uint32_t> get_group_sizes(uint32_t num_members, std::size_t num_rounds)
    {
        auto basic_group_size = int_root(num_members, num_rounds);
        if (basic_group_size == 3)
        {
            std::vector<uint32_t> result(num_rounds, 4);
            // result.front() is always 4, but for some reason, that causes clang to miscompile this.
            // TODO: look for UB...
            auto large_rounds =
                static_cast<std::size_t>(std::log(static_cast<double>(num_members) /
                    int_pow(result.front(), num_rounds - 1) / 3) /
                    std::log(1.25));
            result.back() = 3;
            eosio::check(large_rounds <= 1,
                "More that one large round is unexpected when the final group size is 3.");
            for (std::size_t i = result.size() - large_rounds - 1; i < result.size() - 1; ++i)
            {
                result[i] = 5;
            }
            return result;
        }
        else if (basic_group_size >= 6)
        {
            // 5,6,...,6,N
            std::vector<uint32_t> result(num_rounds, 6);
            result.front() = 5;
            auto divisor = int_pow(6, num_rounds - 1);
            result.back() = (num_members + divisor - 1) / divisor;
            return result;
        }
        else
        {
            // \lfloor \log_{(G+1)/G}\frac{N}{G^R} \rfloor
            auto large_rounds = static_cast<std::size_t>(
                std::log(static_cast<double>(num_members) / int_pow(basic_group_size, num_rounds)) /
                std::log((basic_group_size + 1.0) / basic_group_size));
            // x,x,x,x,x,x
            std::vector<uint32_t> result(num_rounds, basic_group_size + 1);
            std::fill_n(result.begin(), num_rounds - large_rounds, basic_group_size);
            return result;
        }
    }

    // This creates rounds based on the election data - 
    // We can create a maximum number of rounds from this?
    // Or else just create 1 round at a time.
    // parsing extermal document
    static std::map<int64_t, ElectionRoundData> getRounds(ContentGroups& electionConfig, time_point& endDate)
    {
        auto cw = ContentWrapper(electionConfig);
        //Store rounds groups sorted by their round id, in case they aren't sorted
        std::map<int64_t, ElectionRoundData> rounds;

        for (size_t i = 0; i < electionConfig.size(); ++i) {
            auto& group = electionConfig[i];
            if (cw.getGroupLabel(group) == upvote_common::groups::ROUND) {

                ElectionRoundData data;

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

                data.round_duration = duration;

                endDate += eosio::seconds(duration);

                rounds.insert({ roundId, data });
            }
        }

        return rounds;
    }

    static constexpr int64_t getDelegatePower(int64_t roundId) {
        return roundId * 1 << roundId;
    }

    // move out this code - only create a start round
    // then add more rounds as needed
    // Analyze election round data and see what we need - start and end date for sure
    static void createRounds(dao& dao, UpvoteElection& election, std::map<int64_t, ElectionRoundData>& rounds, time_point startDate, time_point endDate)
    {
        std::unique_ptr<ElectionRound> prevRound;

        // bool hasChief = false; // I don't think we need to check rounds..

        for (auto& [roundId, roundData] : rounds) {

            // TODO: What is this?? 
            //roundData.delegate_power = getDelegatePower(roundId);

            roundData.start_date = startDate;

            // Nik: Let's not modify start date?
            //startDate += eosio::seconds(roundData.duration);

            roundData.end_date = startDate + eosio::seconds(roundData.round_duration);

            // if (roundData.type == upvote_common::round_types::HEAD) {
            //     EOS_CHECK(
            //         prevRound && prevRound->getType() == upvote_common::round_types::CHIEF,
            //         "There has to be a Chief round previous to Head Delegate round"
            //     );
            // }
            // else {
            //     EOS_CHECK(
            //         !hasChief,
            //         to_str("Cannot create ", roundData.type, "type rounds after a Chief round")
            //     );

            //     hasChief = hasChief || roundData.type == upvote_common::round_types::CHIEF;
            // }

            auto electionRound = std::make_unique<ElectionRound>(
                dao,
                election.getId(),
                roundData
            );

            if (prevRound) {

                // Nik: Not sure what this check is for - we're not using getPassingCount anymore anyway
                // EOS_CHECK(
                //     prevRound->getPassingCount() > electionRound->getPassingCount(),
                //     "Passing count has to be decremental"
                // );

                prevRound->setNextRound(electionRound.get());
            }
            else {
                election.setStartRound(electionRound.get());
            }

            prevRound = std::move(electionRound);
        }

        // EOS_CHECK(
        //     prevRound->getType() == upvote_common::round_types::CHIEF ||
        //     prevRound->getType() == upvote_common::round_types::HEAD,
        //     "Last round must be of type Chief or Head"
        // )

        //Verify we have a chief round
        // election.getChiefRound();

        //At the end both start date and end date should be the same
        EOS_CHECK(
            startDate == endDate,
            to_str("End date missmatch: ", startDate, " ", endDate)
        );
    }

       // Requirements:
       // - Except for the last round, the group size shall be in [4,6]
       // - The last round has a minimum group size of 3
       // - The maximum group size shall be as small as possible
       // - The group sizes within a round shall have a maximum difference of 1
    static std::vector<std::vector<uint64_t>> createGroups(const std::vector<uint64_t>& ids, int minGroupSize) {
        std::vector<std::vector<uint64_t>> groups;

        // Calculate the number of groups needed - group size fills up to 6 max, except when there's
        // only 1 group, which goes up to 11
        int numGroups = ids.size() / minGroupSize;

        // Create the groups with an initial capacity of 4
        for (int i = 0; i < numGroups; ++i) {
            groups.push_back(std::vector<uint64_t>());
            groups.back().reserve(4);
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

            // Wrap back to the beginning of the groups
            if (currentGroup == endGroup) {
                currentGroup = groups.begin();
            }
        }

        return groups;
    }

    /// @brief Use the Edenia method to figure out number of rounds and group sizes
    /// @param ids a list of member Ids
    /// @return A list of groups of member Ids. 
    static std::vector<std::vector<uint64_t>> createGroupsEden(const std::vector<uint64_t>& ids) {
        // Edenia code for counting number of rounds and min group sizes for the rounds
        auto n = ids.size();
        auto rounds = count_rounds(n);
        std::vector<uint32_t> group_sizes = get_group_sizes(n, rounds);
        int minGroupsSize = group_sizes[0];
        return createGroups(ids, minGroupsSize);
    }

    static void initRound(UpvoteElection& election, ElectionRound& round, uint32_t seed, std::vector<uint64_t> delegateIds) {

        // Create a random number generator with the seed
        UERandomGenerator rng(seed, 0);

        auto randomIds = delegateIds;

        std::shuffle(randomIds.begin(), randomIds.end(), rng);

        election.setRunningSeed(rng.seed);
        election.update();

        auto groups = createGroupsEden(randomIds);

        for (auto& groupMembers : groups) {

            eosio::print(" add gr: ", groupMembers.size(), ": ");
            for (uint64_t m : groupMembers) {
                eosio::print(m, ", ");
            }

            round.addElectionGroup(groupMembers);
        }

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
            std::make_tuple(election.getId(), true, false)
        ));

        EOS_CHECK(
            date > eosio::current_time_point(),
            "Can only schedule for dates in the future"
        );

        constexpr auto aditionalDelaySec = 10;
        trx.delay_sec = (date - eosio::current_time_point()).to_seconds() + aditionalDelaySec;

        auto dhoSettings = dao.getSettingsDocument();

        auto nextID = dhoSettings->getSettingOrDefault("next_schedule_id", int64_t(0));

        trx.send(nextID, dao.get_self());

        dhoSettings->setSetting(Content{ "next_schedule_id", nextID + 1 });
    }

    static void assignDelegateBadges(
        dao& dao, 
        uint64_t daoId, 
        uint64_t electionId, 
        const std::vector<uint64_t>& chiefDelegates, 
        uint64_t headDelegate, 
        bool deleteExistingBadges,
        eosio::transaction* trx = nullptr)
    {
        // TODO This removes and adds badges. They're all pretty expensive operations but at most there's 11 badges
        // So it should work in all cases. If not we could pack it into deferred transactions. 

        // Delete existing if required
        if (deleteExistingBadges) {
            // Note: This must happen before we assign new badges, since new and old badge holders
            // might be the same

            // Note: This might not be needed since badges actually expire when their period is over.

            //Remove existing Head Delegate/Chief Delegate badges if any
            auto cleanBadgesOf = [&](const name& badgeEdge) {
                auto badgeId = Edge::get(dao.get_self(), dao.getRootID(), badgeEdge).getToNode();

                auto badgeAssignmentEdges = dao.getGraph().getEdgesFrom(badgeId, common::ASSIGNMENT);

                if (badgeAssignmentEdges.size() == 0) {
                    eosio::print(" no existing badge holders ", badgeEdge);
                    return;
                }

                //Filter out those that are not from the specified DAO
                auto badgeAssignments = std::vector<uint64_t>{};
                badgeAssignments.reserve(badgeAssignmentEdges.size());

                std::transform(
                    badgeAssignmentEdges.begin(),
                    badgeAssignmentEdges.end(),
                    std::back_inserter(badgeAssignments),
                    [](const Edge& edge) {
                        return edge.to_node;
                    }
                );

                badgeAssignments.erase(
                    std::remove_if(
                        badgeAssignments.begin(),
                        badgeAssignments.end(),
                        [&](uint64_t id) {
                            return !Edge::exists(dao.get_self(), id, daoId, common::DAO);
                        }
                    ),
                    badgeAssignments.end()
                );

                for (auto& id : badgeAssignments) {
                    auto doc = TypedDocument::withType(dao, id, common::ASSIGN_BADGE);

                    auto cw = doc.getContentWrapper();

                    cw.insertOrReplace(*cw.getGroupOrFail(SYSTEM), Content{
                        "force_archive",
                        1
                        });

                    cw.insertOrReplace(*cw.getGroupOrFail(DETAILS), Content{
                        END_TIME,
                        eosio::current_time_point()
                        });

                    doc.update();

                    auto action = eosio::action(
                        eosio::permission_level(dao.get_self(), eosio::name("active")),
                        dao.get_self(),
                        eosio::name("archiverecur"),
                        std::make_tuple(id)
                    );

                    if (trx) {
                        trx->actions.emplace_back(std::move(action));
                    }
                    else {
                        action.send();
                    }
                }
            };

            cleanBadgesOf(badges::common::links::HEAD_DELEGATE);
            cleanBadgesOf(badges::common::links::CHIEF_DELEGATE);

        }

        // Generate proposals for each one of the delegates
        // Note: These are auto-approved and instantly on.
        auto createAssignment = [&](const std::string& title, uint64_t member, uint64_t badge) {

            Member mem(dao, member);

            auto memAccount = mem.getAccount();

            // TODO: I wonder if we could call this directly
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
                    } },
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

        auto chiefBadgeEdge = Edge::get(dao.get_self(), dao.getRootID(), badges::common::links::CHIEF_DELEGATE);
        auto chiefBadgeId = chiefBadgeEdge.getToNode();
        // auto chiefBadge = TypedDocument::withType(dao, chiefBadgeId, common::BADGE_NAME);

        auto headBadgeEdge = Edge::get(dao.get_self(), dao.getRootID(), badges::common::links::HEAD_DELEGATE);
        auto headBadgeId = headBadgeEdge.getToNode();
        // auto headBadge = TypedDocument::withType(dao, headBadgeId, common::BADGE_NAME);

        for (auto& chief : chiefDelegates) {
            if (chief != headDelegate) {
                createAssignment("Chief Delegate", chief, chiefBadgeId);
            }
        }

        if (headDelegate) {
            createAssignment("Head Delegate", headDelegate, headBadgeId);
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
                [](const Edge& edge) {
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

                cw.insertOrReplace(*cw.getGroupOrFail(SYSTEM), Content{
                    "force_archive",
                    1
                    });

                cw.insertOrReplace(*cw.getGroupOrFail(DETAILS), Content{
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

        cleanBadgesOf(badges::common::links::HEAD_DELEGATE);
        cleanBadgesOf(badges::common::links::CHIEF_DELEGATE);

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

        std::visit([&](election_state_v0& election) {

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
        assignDelegateBadges(*this, dao_id, 0, chiefs, head, false, &trx);

        //Trigger all cleanup and propose actions
        if (deferred) {
            constexpr auto aditionalDelaySec = 5;
            trx.delay_sec = aditionalDelaySec;

            auto dhoSettings = getSettingsDocument();

            auto nextID = dhoSettings->getSettingOrDefault("next_schedule_id", int64_t(0));

            trx.send(nextID, get_self());

            dhoSettings->setSetting(Content{ "next_schedule_id", nextID + 1 });
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
    // void dao::testgroupr1(uint32_t num_members, uint32_t seed) {


    //     require_auth(get_self());

    //     std::vector<uint64_t> ids(num_members); // Initialize a vector with 100 elements

    //     // Set each element's value to its index
    //     for (size_t i = 0; i < num_members; ++i) {
    //         ids[i] = static_cast<uint64_t>(i);
    //     }
    //     testgrouprng(ids, seed);



    // }

    // void dao::testround(uint64_t dao_id) {
    //     require_auth(get_self());
        
    //     auto edge = Edge::get(get_self(), dao_id, upvote_common::links::ELECTION);
        
    //     auto electionId = edge.getToNode();
        
    //     eosio::print(" election id: ", electionId, " ::: ");

    //     UpvoteElection upvoteElection(*this, electionId);

    //     upvoteElection.addRound();

    //     for (uint32_t n = 0; n < num_members; ++n) {
    //         auto rounds = count_rounds(n);

    //         std::vector<uint32_t> group_sizes = get_group_sizes(n, rounds);

    //         eosio::print(" N: ", n, " rounds: ", rounds, " g_sizes: ");

    //         for (uint32_t s : group_sizes) {
    //             eosio::print(s, " ");
    //         }

    //         eosio::print(" ::: ");


    //     }


    // }


    // void dao::testgrouprng(std::vector<uint64_t> ids, uint32_t seed) {

    //     require_auth(get_self());

    //     eosio::print(" ids: ");
    //     for (const uint64_t& element : ids) {
    //         eosio::print(element, " ");
    //     }

    //     UERandomGenerator rng(seed, 0);

    //     auto randomIds = ids;

    //     std::shuffle(randomIds.begin(), randomIds.end(), rng);

    //     eosio::print(" random ids: ");
    //     for (const uint64_t& element : randomIds) {
    //         eosio::print(element, " ");
    //     }

    //     //// Eden defines min group size as 4, but depending on 
    //     //// the numbers, Edenia code does something slightly different
    //     // int minGroupsSize = 4;

    //     //// use Edenia code to figure out group size
    //     auto n = ids.size();
    //     auto rounds = count_rounds(n);
    //     std::vector<uint32_t> group_sizes = get_group_sizes(n, rounds);
    //     int minGroupsSize = group_sizes[0];
    //     auto groups = createGroups(randomIds, minGroupsSize);

    //     eosio::print(" groups min size: ", minGroupsSize);
    //     for (uint32_t i = 0; i < groups.size(); ++i) {
    //         auto group = groups[i];
    //         eosio::print(" group: ", i, "(", group.size(), "): ");
    //         for (const uint64_t& element : group) {
    //             eosio::print(element, " ");
    //         }
    //     }

    // }

    // 36 / 6 = 6 => 1 round, 1 HD round
    size_t numrounds(size_t num_delegates) {
        size_t rounds = 1;
        if (num_delegates > 12) {
            size_t numGroups = std::ceil(static_cast<double>(num_delegates) / 6);

        }
        return rounds;
    }

    // From EDEN contracts - decides group max size by num groups and num participants
    //    struct election_round_config
    //    {
    //       uint16_t num_participants;
    //       uint16_t num_groups;
    //       constexpr uint8_t group_max_size() const
    //       {
    //          return (num_participants + num_groups - 1) / num_groups;
    //       }
    //       constexpr uint16_t num_short_groups() const
    //       {
    //          return group_max_size() * num_groups - num_participants;
    //       }

    //       constexpr uint32_t num_large_groups() const { return num_groups - num_short_groups(); }
    //       constexpr uint32_t group_min_size() const { return group_max_size() - 1; }
    //       uint32_t member_index_to_group(uint32_t idx) const;
    //       uint32_t group_to_first_member_index(uint32_t idx) const;
    //       // invariants:
    //       // num_groups * group_max_size - num_short_groups = num_participants
    //       // group_max_size <= 12
    //       // num_short_groups < num_groups
    //    };



    //Check if we need to update an ongoing elections status:
    // upcoming -> ongoing
    // ongoing -> finished
    // or change the current round
    // reschedule flag: Whether or not to put another deferred transaction to try again (should be true in normal use)
    // force flag: force update of status - for debugging, not used in normal use - true requires self authorization
    void dao::updateupvelc(uint64_t election_id, bool reschedule, bool force)
    {
        if (force) {
            require_auth(get_self());
        }
        //eosio::require_auth();
        UpvoteElection election(*this, election_id);

        auto status = election.getStatus();

        auto now = eosio::current_time_point();

        auto daoId = election.getDaoID();

        if (status == upvote_common::upvote_status::UPCOMING) {
            auto start = election.getStartDate();

                    std::string timeStrNow = eosio::time_point_sec(now).to_string();
                    std::string timeStrStart = eosio::time_point_sec(start).to_string();

            eosio::print(" upcoming ", timeStrNow, " ", timeStrStart);

            //Let's update as we already started
            if (start <= now || force) {
                // START....
                eosio::print(" -> starting ");

                Edge::get(get_self(), daoId, election.getId(), upvote_common::links::UPCOMING_ELECTION).erase();

                Edge(get_self(), get_self(), daoId, election.getId(), upvote_common::links::ONGOING_ELECTION);

                election.setStatus(upvote_common::upvote_status::ONGOING);

                auto startRound = election.getStartRound();
                election.setCurrentRound(&startRound);
                election.update();

                //Setup all candidates
                auto delegates = getGraph().getEdgesFrom(daoId, badges::common::links::DELEGATE);

                std::vector<uint64_t> delegateIds;
                delegateIds.reserve(delegates.size());

                for (auto& delegate : delegates) {
                    delegateIds.push_back(delegate.getToNode());
                }
                auto initialSeed = election.getSeed();
                uint32_t seed = checksum256_to_uint32(initialSeed);
                eosio::print(" init sd: ", initialSeed, " seed ", seed);

                initRound(election, startRound, seed, delegateIds);

                scheduleElectionUpdate(*this, election, startRound.getEndDate());
            }
            else if (reschedule) {
                eosio::print(" -> not ready, rescheduling ");

                scheduleElectionUpdate(*this, election, start);
            }
        }
        else if (status == upvote_common::upvote_status::ONGOING) {

            eosio::print(" ongoing ");

            auto currentRound = election.getCurrentRound();
            auto end = currentRound.getEndDate();
            if (end <= now || force) {
                eosio::print(" current round ended ");

                auto winners = currentRound.getWinners();

                for (auto& winner : winners) {
                    eosio::print(winner, ", "); // DEBUG REMOVE
                    Edge(get_self(), get_self(), currentRound.getId(), winner, upvote_common::links::ROUND_WINNER);
                }


                int32_t seed = election.getRunningSeed();

                if (winners.size() > 11) {

                    // set up the next round
                    auto round = election.addRound();
                    eosio::print(" -> next round with mem ", winners.size(), " rd: ", round.getId(), " ");
                    
                    Edge::get(get_self(), election_id, upvote_common::links::CURRENT_ROUND).erase();
                    election.setCurrentRound(&round);
                    election.update();
                    
                    initRound(election, round, seed, winners);


                    scheduleElectionUpdate(*this, election, round.getEndDate());
                }
                else {
                    eosio::print(" election ended. ");

                    // The election has ended.
                    Edge::get(get_self(), election_id, upvote_common::links::CURRENT_ROUND).erase();

                    // delete ongoing election link
                    Edge::get(get_self(), daoId, election.getId(), upvote_common::links::ONGOING_ELECTION).erase();

                    // add previous election link
                    Edge(get_self(), get_self(), daoId, election.getId(), upvote_common::links::PREVIOUS_ELECTION);

                    // select head
                    UERandomGenerator rng(seed, 0);
                    auto headIndex = rng() % winners.size();
                    auto headDelegate = winners[headIndex];

                    eosio::print(" head del ix: ", headIndex, " winners size: ", winners.size(), " head del: ", headDelegate, " ");

                    assignDelegateBadges(*this, daoId, election.getId(), winners, headDelegate, true);

                    election.setStatus(upvote_common::upvote_status::FINISHED);
                }
            }
            else if (reschedule) {
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

        // TODO
        // canceling an ongoing election - debug feature?
        bool isOngoing = status == upvote_common::upvote_status::ONGOING;

        EOS_CHECK(
            isOngoing ||
            status == upvote_common::upvote_status::UPCOMING,
            to_str("Cannot cancel election with ", status, " status")
        );

        if (isOngoing) {
            /// TODO get all rounds, then delete them all
            Edge::get(get_self(), daoId, election.getId(), upvote_common::links::ONGOING_ELECTION).erase();
        }
        else {
            Edge::get(get_self(), daoId, election.getId(), upvote_common::links::UPCOMING_ELECTION).erase();
        }

        election.setStatus(upvote_common::upvote_status::CANCELED);

        election.update();
    }

      void dao::uesubmitseed(uint64_t dao_id, eosio::checksum256 seed, name account) {
        eosio::require_auth(account);
        
        auto edge = Edge::get(get_self(), dao_id, upvote_common::links::UPCOMING_ELECTION);
        
        auto electionId = edge.getToNode();

        UpvoteElection upvoteElection(*this, electionId);

        upvoteElection.setSeed(seed);

        upvoteElection.update();


      }

    void dao::castupvote(uint64_t round_id, uint64_t group_id, name voter, uint64_t voted_id)
    {
        // Check auth
        eosio::require_auth(voter);
        auto memberId = getMemberID(voter);

        // Check the round is valid
        ElectionRound round(*this, round_id);
        UpvoteElection election = round.getElection();
        auto currentRound = election.getCurrentRound();

        EOS_CHECK(
            currentRound.getId() == round_id,
            "You can only vote on the current round"  + std::to_string(currentRound.getId())
        );

        // Check the group membership of voter and voted
        ElectionGroup group(*this, group_id);

        EOS_CHECK(
            group.isElectionRoundMember(memberId),
            "Only members of the group can vote."
        );

        EOS_CHECK(
            group.isElectionRoundMember(voted_id),
            "Only members of the group can be voted for."
        );

        group.vote(memberId, voted_id);

    }


    /*
    election_config: [
        [
            { "label": "content_group_label", "value": ["string", "details"] },
            //Upvote start date-time
            { "label": "upvote_start_date_time", "value": ["timepoint", "..."] },
            //How much will chief/head Delegates hold their badges after upvote is finished
            { "label": "upvote_duration", "value": ["int64", 7776000] },
            // Round Duration in seconds
            { "label": "round_duration", "value": ["int64", 3600] },

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

        ],
        [
            { "label":, "content_group_label", "value": ["string", "round"] },
            { "label": "duration", "value": ["int64", 9000] }, //Duration in seconds
            { "label": "type", "value": ["string", "chief"] },
            { "label": "round_id", "value": ["int64", 1] },
        ],
        [
            { "label":, "content_group_label", "value": ["string", "round"] },
            { "label": "duration", "value": ["int64", 9000] }, //Duration in seconds
            { "label": "type", "value": ["string", "head"] },
            { "label": "round_id", "value": ["int64", 1] },
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

        auto round_duration = cw.getOrFail(
            DETAILS,
            upvote_common::items::ROUND_DURATION
        )->getAs<int64_t>();

        time_point endDate = startDate + eosio::seconds(round_duration);

        UpvoteElection upvoteElection(*this, dao_id, UpvoteElectionData{
            .start_date = startDate,
            .end_date = endDate,
            .status = upvote_common::upvote_status::UPCOMING,
            .duration = duration,
            .round_duration = round_duration
            });

        // add start round
        auto roundId = upvoteElection.addRound();

        scheduleElectionUpdate(*this, upvoteElection, startDate);
    }

    // TODO combine this with create - so we extract the data only in one place

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

        auto startRound = upvoteElection.getStartRound();
        startRound.erase();
        upvoteElection.addRound();

        scheduleElectionUpdate(*this, upvoteElection, startDate);
    }

}


#endif