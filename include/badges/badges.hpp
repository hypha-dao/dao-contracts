/**
 * Temporary implementation for Badge functionalities,
 * due to the limitations of blockchain we need to save as
 * much space as possible so all Badge related functions will
 * be implemented in badge.hpp/badge.cpp files, and should
 * be eventually refactored into classes i.e. Badge (Interface) 
 * and AdminBadge as an specific implementation etc.
*/

#pragma once

#include <cstdint>
#include "common.hpp"

namespace hypha
{

class ContentWrapper;
class RecurringActivity;
class Document;

class dao;

namespace badges {

using common::BadgeType;
using common::SystemBadgeType;

struct BadgeInfo {
    BadgeType type;
    SystemBadgeType systemType;
};

//void validateBadge(ContentWrapper& badgeCW);

BadgeInfo getBadgeInfo(Document& badge);

void onBadgeActivated(dao& dao, RecurringActivity& badgeAssign);

void onBadgeArchived(dao& dao, RecurringActivity& badgeAssign);

Document getBadgeOf(dao& dao, uint64_t badgeAssignID);

bool hasNorthStarBadge(dao& dao, uint64_t daoID, uint64_t memberID);

void checkHoldsBadge(dao& dao, Document& badge, uint64_t daoID, uint64_t memberID);

}

} // namespace hypha
