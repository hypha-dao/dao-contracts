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
    inline constexpr auto UPVOTE_ELECTION = "upvt.electn"_n;
    inline constexpr auto ELECTION_ROUND = "electn.round"_n;
    inline constexpr auto ELECTION_VOTE = "election.vote"_n;
    inline constexpr auto ELECTION_VOTE_GROUP = "vote.group"_n;
}

namespace upvote_status {
    inline constexpr auto UPCOMING = "upcoming";
    inline constexpr auto ONGOING = "ongoing";
    inline constexpr auto CANCELED = "canceled";
    inline constexpr auto FINISHED = "finished";
}

namespace links {
    inline constexpr auto UPCOMING_ELECTION = "upcomingelct"_n;
    inline constexpr auto ONGOING_ELECTION = "ongoingelct"_n;
    inline constexpr auto PREVIOUS_ELECTION = "previouselct"_n;
    inline constexpr auto ELECTION = "election"_n;
    inline constexpr auto START_ROUND = "startround"_n;
    inline constexpr auto CURRENT_ROUND = "currentround"_n;
    inline constexpr auto CHIEF_ROUND = "chiefround"_n;
    inline constexpr auto HEAD_ROUND = "headround"_n;
    inline constexpr auto ROUND = "round"_n;
    inline constexpr auto NEXT_ROUND = "nextround"_n;
    inline constexpr auto ROUND_CANDIDATE = "candidate"_n;
    inline constexpr auto ROUND_WINNER = "winner"_n;
    inline constexpr auto ELECTION_GROUP = "elctngroup"_n;
    inline constexpr auto VOTE = "vote"_n; 
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
