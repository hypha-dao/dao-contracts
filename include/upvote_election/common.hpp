#pragma once

#include <eosio/name.hpp>

namespace hypha::upvote_election::common {

namespace round_types {
    constexpr auto DELEGATE = "delegate";
    constexpr auto CHIEF = "chief";
    constexpr auto HEAD = "head";
}

namespace groups {
    constexpr auto ROUND = "round";
}

namespace types {
    constexpr auto UPVOTE_ELECTION = eosio::name("upvt.electn");
    constexpr auto ELECTION_ROUND = eosio::name("electn.round");
    constexpr auto ELECTION_VOTE = eosio::name("election.vote");
    constexpr auto ELECTION_VOTE_GROUP = eosio::name("vote.group");
}

namespace upvote_status {
    constexpr auto UPCOMING = "upcoming";
    constexpr auto ONGOING = "ongoing";
    constexpr auto CANCELED = "canceled";
    constexpr auto FINISHED = "finished";
}

namespace links {
    constexpr auto UPCOMING_ELECTION = eosio::name("upcomingelct");
    constexpr auto ONGOING_ELECTION = eosio::name("ongoingelct");
    constexpr auto PREVIOUS_ELECTION = eosio::name("previouselct");
    constexpr auto ELECTION = eosio::name("election");
    constexpr auto START_ROUND = eosio::name("startround");
    constexpr auto CURRENT_ROUND = eosio::name("currentround");
    constexpr auto CHIEF_ROUND = eosio::name("chiefround");
    constexpr auto HEAD_ROUND = eosio::name("headround");
    constexpr auto ROUND = eosio::name("round");
    constexpr auto NEXT_ROUND = eosio::name("nextround");
    constexpr auto ROUND_CANDIDATE = eosio::name("candidate");
    constexpr auto ROUND_WINNER = eosio::name("winner");
    constexpr auto ELECTION_GROUP = eosio::name("elctngroup");
    constexpr auto VOTE = eosio::name("vote");
    constexpr auto CHIEF_DELEGATE = eosio::name("chiefdelegate");
    constexpr auto HEAD_DELEGATE = eosio::name("headdelegate");
}

namespace items {
    constexpr auto UPVOTE_STARTDATE = "upvote_start_date_time";
    constexpr auto UPVOTE_DURATION = "upvote_duration";
    constexpr auto ROUND_DURATION = "duration";
    constexpr auto PASSING_AMOUNT = "passing_count";
    constexpr auto ROUND_ID = "round_id";
    constexpr auto ROUND_TYPE = "type";
}

} // namespace hypha::upvote
