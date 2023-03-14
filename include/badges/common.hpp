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
    constexpr auto SYSTEM_BADGE_ID = "badge_id";
    constexpr auto UPVOTE_ELECTION_ID = "election_id";
}

namespace links {
    // constexpr auto TREASURY_BADGE = "treasurerbdg";
    // constexpr auto ADMIN_BADGE = "adminbdg";
    // constexpr auto ENROLLER_BADGE = "enrollerbdg";
    // constexpr auto NORTH_STAR_BADGE = "northstarbdg";
    constexpr auto VOTER = eosio::name("voter");
    constexpr auto DELEGATE = eosio::name("delegate"); 
    constexpr auto HEAD_DELEGATE = eosio::name("headdelegate"); 
    constexpr auto CHIEF_DELEGATE = eosio::name("chiefdelegate"); 
}

}