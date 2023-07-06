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


namespace badges_links = badges::common::links;
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
        SystemBadgeType::Enroller,
        SystemBadgeType::Voter,
        SystemBadgeType::Delegate,
        SystemBadgeType::ChiefDelegate,
        SystemBadgeType::HeadDelegate
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

    //Previously we enrolled member automatically, for now it will be disabled
    //as this could incorrectly enroll a community member
    //mem.checkMembershipOrEnroll(badgeAssign.getDaoID()); 

    auto hasMembership = [&](SystemBadgeType& type){

        bool isMember = Member::isMember(dao, badgeAssign.getDaoID(), assignee);
        
        //Let's check for community as well if the Badge is self approve type
        if (!isMember && isSelfApproveBadge(type)) {
            isMember = Member::isCommunityMember(dao, badgeAssign.getDaoID(), assignee);
        }
        
        return isMember;
    };
    
    EOS_CHECK(
        hasMembership(systemType),
        to_str(assignee, " must have a valid membership to activate Badge")
    );

    auto createLink = [&](eosio::name link){ 
        Edge(
            dao.get_self(), 
            dao.get_self(), 
            badgeAssign.getDaoID(), 
            mem.getID(), 
            link
        );
    };

    if (type == BadgeType::System){
        switch (systemType)
        {
        case SystemBadgeType::Admin: {
            //Activate admin badge
            createLink(hypha::common::ADMIN);
            Edge(
                dao.get_self(), 
                dao.get_self(), 
                mem.getID(), 
                badgeAssign.getID(), 
                common::links::ADMIN_BADGE
            );
        } break;
#ifdef USE_TREASURY
        case SystemBadgeType::Treasurer: {
            auto treasury = treasury::Treasury::getFromDaoID(dao, badgeAssign.getDaoID());
            treasury.addTreasurer(mem.getID());
        } break;
#endif
        case SystemBadgeType::NorthStar: {
            createLink(hypha::common::NORTH_STAR_HOLDER);
        } break;
        case SystemBadgeType::Enroller: {
            createLink(hypha::common::ENROLLER);
            Edge(
                dao.get_self(), 
                dao.get_self(), 
                mem.getID(), 
                badgeAssign.getID(), 
                common::links::ENROLLER_BADGE
            );
        } break;
        case SystemBadgeType::Voter: {
            createLink(badges_links::VOTER);
        } break;
        case SystemBadgeType::Delegate: {
            createLink(badges_links::DELEGATE);
        } break;
        case SystemBadgeType::HeadDelegate: {
            createLink(badges_links::HEAD_DELEGATE);
        } break;
        case SystemBadgeType::ChiefDelegate: {
            createLink(badges_links::CHIEF_DELEGATE);
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

    auto removeLink = [&](eosio::name link){ 
        Edge::get(
            dao.get_self(), 
            badgeAssign.getDaoID(), 
            memID, 
            link
        ).erase();
    };

    if (type == BadgeType::System){
        switch (systemType)
        {
        case SystemBadgeType::Admin: {
            removeLink(hypha::common::ADMIN);
            if (Edge::exists(dao.get_self(), memID, badgeAssign.getID(), common::links::ADMIN_BADGE)) {
                Edge::get(dao.get_self(), memID, badgeAssign.getID(), common::links::ADMIN_BADGE).erase();
            }
        } break;
#ifdef USE_TREASURY
        case SystemBadgeType::Treasurer: {
            auto treasury = treasury::Treasury::getFromDaoID(dao, badgeAssign.getDaoID());
            treasury.removeTreasurer(memID);
        } break;
#endif
        case SystemBadgeType::NorthStar: {
            removeLink(hypha::common::NORTH_STAR_HOLDER);
        } break;
        case SystemBadgeType::Enroller: {
            removeLink(hypha::common::ENROLLER);
            if (Edge::exists(dao.get_self(), memID, badgeAssign.getID(), common::links::ENROLLER_BADGE)) {
                Edge::get(dao.get_self(), memID, badgeAssign.getID(), common::links::ENROLLER_BADGE).erase();
            }
        } break;
        case SystemBadgeType::Voter: {
            removeLink(badges_links::VOTER);
        } break;
        case SystemBadgeType::Delegate: {
            removeLink(badges_links::DELEGATE);
        } break;
        case SystemBadgeType::HeadDelegate: {
            removeLink(badges_links::HEAD_DELEGATE);
        } break;
        case SystemBadgeType::ChiefDelegate: {
            removeLink(badges_links::CHIEF_DELEGATE);
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

static bool hasAdminBadge(dao& dao, uint64_t daoID, uint64_t memberID) 
{
    return Edge::exists(dao.get_self(), daoID, memberID, hypha::common::ADMIN);
}

static bool hasEnrollerBadge(dao& dao, uint64_t daoID, uint64_t memberID) 
{
    return Edge::exists(dao.get_self(), daoID, memberID, hypha::common::ENROLLER);
}

static bool hasTreasurerBadge(dao& dao, uint64_t daoID, uint64_t memberID) 
{
    return Edge::exists(dao.get_self(), daoID, memberID, treasury::common::links::TREASURER);
}

bool hasNorthStarBadge(dao& dao, uint64_t daoID, uint64_t memberID)
{
    return Edge::exists(dao.get_self(), daoID, memberID, hypha::common::NORTH_STAR_HOLDER);
}

bool hasVoterBadge(dao& dao, uint64_t daoID, uint64_t memberID)
{
    return Edge::exists(dao.get_self(), daoID, memberID, badges_links::VOTER);
}

bool hasDelegateBadge(dao& dao, uint64_t daoID, uint64_t memberID)
{
    return Edge::exists(dao.get_self(), daoID, memberID, badges_links::DELEGATE);
}

bool hasHeadDelegateBadge(dao& dao, uint64_t daoID, uint64_t memberID)
{
    return Edge::exists(dao.get_self(), daoID, memberID, badges_links::HEAD_DELEGATE);
}

bool hasChiefDelegateBadge(dao& dao, uint64_t daoID, uint64_t memberID)
{
    return Edge::exists(dao.get_self(), daoID, memberID, badges_links::CHIEF_DELEGATE);
}

bool isSelfApproveBadge(SystemBadgeType systemType)
{

    switch (systemType) {
        case SystemBadgeType::Voter:
        case SystemBadgeType::Delegate:
        case SystemBadgeType::ChiefDelegate:
        case SystemBadgeType::HeadDelegate:
        return true;
        default:
        return false;
    }

    return false;
};

void checkHoldsBadge(dao& dao, Document& badge, uint64_t daoID, uint64_t memberID)
{    
    auto [type, systemType] = getBadgeInfo(badge);

    if (type == BadgeType::User) return;

    using HasBadgeFunc = decltype(&hasTreasurerBadge);

    std::map<SystemBadgeType, HasBadgeFunc> hasBadgeMap {
      { SystemBadgeType::Admin, hasAdminBadge },
      { SystemBadgeType::Enroller, hasEnrollerBadge },
      { SystemBadgeType::Treasurer, hasTreasurerBadge },
      { SystemBadgeType::NorthStar, hasNorthStarBadge },
      { SystemBadgeType::Voter, hasVoterBadge },
      { SystemBadgeType::Delegate, hasDelegateBadge },
      { SystemBadgeType::HeadDelegate, hasHeadDelegateBadge },
      { SystemBadgeType::ChiefDelegate, hasChiefDelegateBadge }
    };

    EOS_CHECK(
        hasBadgeMap.count(systemType),
        to_str("Invalid System Badge Type:", static_cast<uint64_t>(systemType))
    )
    
    //User should not have the given Badge
    EOS_CHECK(
        !hasBadgeMap[systemType](dao, daoID, memberID),
        "User already holds badge"
    );
}

} // namespace hypha::badges
