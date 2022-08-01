#include "time_share.hpp"

#include <dao.hpp>
#include <common.hpp>

namespace hypha {

TimeShare::TimeShare(name contract, name creator, int64_t timeShare, time_point startDate, uint64_t assignment) 
: Document(contract, creator, constructContentGroups(timeShare, startDate, assignment))
{
  
}

TimeShare::TimeShare(name contract, uint64_t id)
: Document(contract, id)
{
  
}

std::optional<TimeShare> TimeShare::getNext()
{
  if (auto [exists, edge] = Edge::getIfExists(contract, getID(), common::NEXT_TIME_SHARE);
      exists) {
    return TimeShare(contract, edge.getToNode());
  }

  return std::nullopt;
}

time_point TimeShare::getStartDate()
{
  return getContentWrapper()
         .getOrFail(DETAILS, TIME_SHARE_START_DATE)
         ->getAs<time_point>();
}

ContentGroups TimeShare::constructContentGroups(int64_t timeShare, time_point startDate, uint64_t assignment) 
{
  return {
    ContentGroup {
      Content(CONTENT_GROUP_LABEL, DETAILS),
      Content(TIME_SHARE, timeShare),
      Content(TIME_SHARE_START_DATE, startDate),
      Content(ASSIGNMENT_STRING, static_cast<int64_t>(assignment))
    },
    ContentGroup {
      Content(CONTENT_GROUP_LABEL, SYSTEM),
      Content(TYPE, common::TIME_SHARE_LABEL),
      Content(NODE_LABEL, common::TIME_SHARE_LABEL.to_string()),
    }
  };
}

}