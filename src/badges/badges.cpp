#include <badges/badges.hpp>

#include <map>
#include <functional>

#include <document_graph/edge.hpp>
#include <document_graph/document.hpp>

#include <recurring_activity.hpp>

#include <common.hpp>
#include <dao.hpp>

#include <algorithm>

#include <treasury/treasury.hpp>
#include <treasury/common.hpp>

namespace hypha::badges
{

// void validateBadge(ContentWrapper& badgeCW)
// {

// }

BadgeInfo getBadgeInfo(Document& badge) {
    auto badgeCW = badge.getContentWrapper();

    if (auto [_, systemIDItem] = badgeCW.get(SYSTEM, common::items::SYSTEM_BADGE_ID); 
        systemIDItem) {
      auto id = systemIDItem->getAs<int64_t>();

      auto validBadges = std::array{
        SystemBadgeType::Treasurer,
        SystemBadgeType::Admin,
        SystemBadgeType::NorthStar,
        SystemBadgeType::Enroller
      };

      EOS_CHECK(
        std::any_of(
            validBadges.begin(), 
            validBadges.end(), 
            [id](SystemBadgeType systemType){ 
                return static_cast<int64_t>(systemType) == id; 
            }
        ),
        to_str("System ID is not valid, doc: ", badge.getID(), " id: ", id)
      );

      return BadgeInfo {
        .type = BadgeType::System,
        .systemType = static_cast<SystemBadgeType>(id)
      };
    }

    return BadgeInfo {
        .type = BadgeType::User,
        .systemType = SystemBadgeType::None
    };
}

void onBadgeActivated(dao& dao, RecurringActivity& badgeAssign)
{
    auto badgeDoc = getBadgeOf(dao, badgeAssign.getID());
    auto [type, systemType] = getBadgeInfo(badgeDoc);

    auto assignee = badgeAssign.getAssignee().getAccount();

    auto mem = dao.getOrCreateMember(assignee);
    
    mem.checkMembershipOrEnroll(badgeAssign.getDaoID());

    if (type == BadgeType::System){
        switch (systemType)
        {
        case SystemBadgeType::Admin: {
            //Activate admin badge
            Edge(
                dao.get_self(), 
                dao.get_self(), 
                badgeAssign.getDaoID(), 
                mem.getID(), 
                hypha::common::ADMIN
            );
        } break;
        case SystemBadgeType::Treasurer: {
            auto treasury = treasury::Treasury::getFromDaoID(dao, badgeAssign.getDaoID());
            treasury.addTreasurer(mem.getID());
        } break;
        case SystemBadgeType::NorthStar: {
            Edge(
                dao.get_self(), 
                dao.get_self(), 
                badgeAssign.getDaoID(), 
                mem.getID(), 
                hypha::common::NORTH_STAR_HOLDER
            );
        } break;
        case SystemBadgeType::Enroller: {
            Edge(
                dao.get_self(), 
                dao.get_self(), 
                badgeAssign.getDaoID(), 
                mem.getID(), 
                hypha::common::ENROLLER
            );
        } break;
        default:
            EOS_CHECK(false, "Invalid system badge type");
            break;
        }
    }
}

void onBadgeArchived(dao& dao, RecurringActivity& badgeAssign)
{
    auto badgeDoc = getBadgeOf(dao, badgeAssign.getID());
    auto [type, systemType] = getBadgeInfo(badgeDoc);

    auto assignee = badgeAssign.getAssignee().getAccount();

    auto memID = dao.getMemberID(assignee);

    if (type == BadgeType::System){
        switch (systemType)
        {
        case SystemBadgeType::Admin: {
            Edge::get(
                dao.get_self(), 
                badgeAssign.getDaoID(), 
                memID, 
                hypha::common::ADMIN
            ).erase();
        } break;
        case SystemBadgeType::Treasurer: {
            auto treasury = treasury::Treasury::getFromDaoID(dao, badgeAssign.getDaoID());
            treasury.removeTreasurer(memID);
        } break;
        case SystemBadgeType::NorthStar: {
            Edge::get(
                dao.get_self(), 
                badgeAssign.getDaoID(), 
                memID, 
                hypha::common::NORTH_STAR_HOLDER
            ).erase();
        } break;
        case SystemBadgeType::Enroller: {
            Edge::get(
                dao.get_self(), 
                badgeAssign.getDaoID(), 
                memID, 
                hypha::common::ENROLLER
            ).erase();
        } break;
        default:
            EOS_CHECK(false, "Invalid system badge type");
            break;
        }
    }
}

Document getBadgeOf(dao& dao, uint64_t badgeAssignID)
{
    Edge badgeEdge = Edge::get(
        dao.get_self(), 
        badgeAssignID, 
        hypha::common::BADGE_NAME
    );

    return Document(dao.get_self(), badgeEdge.getToNode());
}

static bool hasAdminBadge(dao& dao, uint64_t daoID, uint64_t memberID) {
    return Edge::exists(dao.get_self(), daoID, memberID, hypha::common::ADMIN);
}

static bool hasEnrollerBadge(dao& dao, uint64_t daoID, uint64_t memberID) {
    return Edge::exists(dao.get_self(), daoID, memberID, hypha::common::ENROLLER);
}

static bool hasTreasurerBadge(dao& dao, uint64_t daoID, uint64_t memberID) {
    return Edge::exists(dao.get_self(), daoID, memberID, treasury::common::links::TREASURER);
}

bool hasNorthStarBadge(dao& dao, uint64_t daoID, uint64_t memberID)
{
    return Edge::exists(dao.get_self(), daoID, memberID, hypha::common::NORTH_STAR_HOLDER);
}

void checkHoldsBadge(dao& dao, Document& badge, uint64_t daoID, uint64_t memberID)
{    
    auto [type, systemType] = getBadgeInfo(badge);

    if (type == BadgeType::User) return;

    using HasBadgeFunc = decltype(&hasTreasurerBadge);

    std::map<SystemBadgeType, HasBadgeFunc> hasBadgeMap {
      { SystemBadgeType::Admin, hasAdminBadge },
      { SystemBadgeType::Enroller, hasEnrollerBadge },
      { SystemBadgeType::Treasurer, hasTreasurerBadge },
      { SystemBadgeType::NorthStar, hasNorthStarBadge }
    };

    EOS_CHECK(
        hasBadgeMap.count(systemType),
        to_str("Invalid System Badge Type:", static_cast<uint64_t>(systemType))
    )

    EOS_CHECK(
        hasBadgeMap[systemType](dao, daoID, memberID),
        "User already holds badge"
    );
}

} // namespace hypha::badges
