#pragma once

#include <eosio/name.hpp>

namespace hypha::upvote_election::common {

namespace round_types {
    // TODO: We can remove round types - not needed
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
    inline constexpr auto ELECTION_VOTE_GROUP = eosio::name("vote.group");//??
}

namespace upvote_status {
    inline constexpr auto UPCOMING = "upcoming";
    inline constexpr auto ONGOING = "ongoing";
    inline constexpr auto CANCELED = "canceled";
    inline constexpr auto FINISHED = "finished";
}

namespace links {
    inline constexpr auto ELECTION = eosio::name("ue.election");
    inline constexpr auto UPCOMING_ELECTION = eosio::name("ue.upcoming");
    inline constexpr auto ONGOING_ELECTION = eosio::name("ue.ongoing");
    inline constexpr auto PREVIOUS_ELECTION = eosio::name("ue.previous");
    inline constexpr auto START_ROUND = eosio::name("ue.startrnd");
    inline constexpr auto CURRENT_ROUND = eosio::name("ue.currnd");
    inline constexpr auto ELECTION_ROUND = eosio::name("ue.round");
    inline constexpr auto ELECTION_ROUND_MEMBER = eosio::name("ue.rd.member");
    inline constexpr auto ELECTION_GROUP_LINK = eosio::name("ue.group.lnk");
    inline constexpr auto NEXT_ROUND = eosio::name("ue.nextrnd");
    inline constexpr auto ROUND_CANDIDATE = eosio::name("ue.candidate");
    inline constexpr auto ROUND_WINNER = eosio::name("ue.winner");
    inline constexpr auto GROUP_WINNER = eosio::name("ue.group.win");
    inline constexpr auto ELECTION_GROUP = eosio::name("ue.elctngrp");
    inline constexpr auto UP_VOTE_VOTE = eosio::name("ue.vote");
    inline constexpr auto UPVOTE_GROUP_WINNER = eosio::name("ue.winner");
    inline constexpr auto VOTE = eosio::name("vote"); // ?? 
}

namespace items {
    inline constexpr auto UPVOTE_STARTDATE = "upvote_start_date_time";
    inline constexpr auto UPVOTE_DURATION = "upvote_duration";
    inline constexpr auto ROUND_DURATION = "duration";
    inline constexpr auto ROUND_ID = "round_id";
    inline constexpr auto ROUND_TYPE = "type";
    inline constexpr auto WINNER = "winner"; // ?? delete?
}

} // namespace hypha::upvote
