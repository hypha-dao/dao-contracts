#pragma once

#include <eosio/name.hpp>

namespace hypha::badges::common {

enum class BadgeType 
{
    User = 1,
    System
};

enum class SystemBadgeType
{
    Treasurer = 1,
    Admin,
    Enroller,
    NorthStar,
    // Upvote Election badges are system badges but they work differently from other badges
    // These are only assigned through the upvote election process
    Voter, // Upvote Election Voter 
    Delegate, // Upvote Election Delegate
    ChiefDelegate, // Upvote Election Chief Delegate
    HeadDelegate, // Upvote Election Head Delegate
    None,
};

namespace items {
    inline constexpr auto SYSTEM_BADGE_ID = "badge_id";
    inline constexpr auto UPVOTE_ELECTION_ID = "election_id";
}

namespace links {
    inline constexpr auto TREASURY_BADGE = eosio::name("treasurerbdg");
    inline constexpr auto ADMIN_BADGE = eosio::name("adminbdg");
    inline constexpr auto ENROLLER_BADGE = eosio::name("enrollerbdg");
    inline constexpr auto NORTH_STAR_BADGE = eosio::name("northstarbdg");

    // Upvote Election badges
    inline constexpr auto VOTER = eosio::name("voter");
    inline constexpr auto DELEGATE = eosio::name("delegate"); 
    inline constexpr auto HEAD_DELEGATE = eosio::name("headdelegate"); 
    inline constexpr auto CHIEF_DELEGATE = eosio::name("chiefdelegate"); 
}

}