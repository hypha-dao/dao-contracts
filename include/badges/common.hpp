#pragma once

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
    None,
};

namespace items {
    inline constexpr auto SYSTEM_BADGE_ID = "badge_id";
}

namespace links {
    inline constexpr auto TREASURY_BADGE = "treasurerbdg";
    inline constexpr auto ADMIN_BADGE = "adminbdg";
    inline constexpr auto ENROLLER_BADGE = "enrollerbdg";
    inline constexpr auto NORTH_STAR_BADGE = "northstarbdg";
}

}