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
    Voter,
    Delegate,
    ChiefDelegate,
    HeadDelegate,
    None,
};

namespace items {
    inline constexpr auto SYSTEM_BADGE_ID = "badge_id";
    inline constexpr auto UPVOTE_ELECTION_ID = "election_id";
}

namespace links {
    // inline constexpr auto TREASURY_BADGE = "treasurerbdg";
    // inline constexpr auto ADMIN_BADGE = "adminbdg";
    // inline constexpr auto ENROLLER_BADGE = "enrollerbdg";
    // inline constexpr auto NORTH_STAR_BADGE = "northstarbdg";
    inline constexpr auto VOTER = eosio::name("voter");
    inline constexpr auto DELEGATE = eosio::name("delegate"); 
    inline constexpr auto HEAD_DELEGATE = eosio::name("headdelegate"); 
    inline constexpr auto CHIEF_DELEGATE = eosio::name("chiefdelegate"); 
}

}