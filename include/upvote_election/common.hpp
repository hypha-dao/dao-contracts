#pragma once

#include <eosio/name.hpp>

namespace hypha::upvote_election::common {

namespace round_types {
    inline constexpr auto DELEGATE = "delegate";
    inline constexpr auto CHIEF = "chief";
    inline constexpr auto HEAD = "head";
}

namespace groups {
    inline constexpr auto ROUND = "round";
}

namespace types {
    inline constexpr auto UPVOTE_ELECTION = eosio::name("upvt.electn");
    inline constexpr auto ELECTION_ROUND = eosio::name("electn.round");
    inline constexpr auto ELECTION_GROUP = eosio::name("electn.group");
    inline constexpr auto ELECTION_VOTE = eosio::name("election.vote");
    inline constexpr auto ELECTION_UP_VOTE = eosio::name("upvt.vote");

    // TODO: Remove this
    inline constexpr auto ELECTION_VOTE_GROUP = eosio::name("vote.group");
}

namespace upvote_status {
    inline constexpr auto UPCOMING = "upcoming";
    inline constexpr auto ONGOING = "ongoing";
    inline constexpr auto CANCELED = "canceled";
    inline constexpr auto FINISHED = "finished";
}

namespace links {
    inline constexpr auto UPCOMING_ELECTION = eosio::name("upcomingelct");
    inline constexpr auto ONGOING_ELECTION = eosio::name("ongoingelct");
    inline constexpr auto PREVIOUS_ELECTION = eosio::name("previouselct");
    inline constexpr auto ELECTION = eosio::name("election");
    inline constexpr auto START_ROUND = eosio::name("startround");
    inline constexpr auto CURRENT_ROUND = eosio::name("currentround");
    inline constexpr auto CHIEF_ROUND = eosio::name("chiefround");
    inline constexpr auto ELECTION_ROUND_MEMBER = eosio::name("ue.rd.member");
    inline constexpr auto HEAD_ROUND = eosio::name("headround");
    inline constexpr auto ROUND = eosio::name("round");
    inline constexpr auto ELECTION_GROUP_LINK = eosio::name("ue.group.lnk");
    inline constexpr auto NEXT_ROUND = eosio::name("nextround");
    inline constexpr auto ROUND_CANDIDATE = eosio::name("candidate");
    inline constexpr auto ROUND_WINNER = eosio::name("winner");
    inline constexpr auto ELECTION_GROUP = eosio::name("elctngroup");
    inline constexpr auto UP_VOTE_VOTE = eosio::name("ue.vote");
    inline constexpr auto VOTE = eosio::name("vote");
    inline constexpr auto CHIEF_DELEGATE = eosio::name("chiefdelegate");
    inline constexpr auto HEAD_DELEGATE = eosio::name("headdelegate");
}

namespace items {
    inline constexpr auto UPVOTE_STARTDATE = "upvote_start_date_time";
    inline constexpr auto UPVOTE_DURATION = "upvote_duration";
    inline constexpr auto ROUND_DURATION = "duration";
    inline constexpr auto PASSING_AMOUNT = "passing_count";
    inline constexpr auto ROUND_ID = "round_id";
    inline constexpr auto ROUND_TYPE = "type";
}

} // namespace hypha::upvote
