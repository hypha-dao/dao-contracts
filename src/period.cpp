#include <period.hpp>
#include <common.hpp>
#include <util.hpp>
#include <dao.hpp>
#include <document_graph/edge.hpp>
#include <logger/logger.hpp>

namespace hypha
{
    Period::Period(dao *dao,
                   const eosio::time_point &start_time,
                   const std::string &label)
        : Document(dao->get_self(), dao->get_self(),
                   ContentGroups{
                       ContentGroup{
                           Content(CONTENT_GROUP_LABEL, DETAILS),
                           Content(START_TIME, start_time),
                           Content(LABEL, label)},
                       ContentGroup{
                           Content(CONTENT_GROUP_LABEL, SYSTEM),
                           Content(TYPE, common::PERIOD),
                           Content(NODE_LABEL, label)}}),
          m_dao{dao}
    {
    }

    Period::Period(dao *dao,
                   const eosio::time_point &start_time,
                   const std::string &label,
                   const std::string &readable,
                   const std::string &nodeLabel)
        : Document(dao->get_self(), dao->get_self(),
                   ContentGroups{
                       ContentGroup{
                           Content(CONTENT_GROUP_LABEL, DETAILS),
                           Content(START_TIME, start_time),
                           Content(LABEL, label)},
                       ContentGroup{
                           Content(CONTENT_GROUP_LABEL, SYSTEM),
                           Content(TYPE, common::PERIOD),
                           // useful when reporting errors and for readable payment memos, albeit a bit overkill
                           Content(READABLE_START_TIME, readable),
                           Content(READABLE_START_DATE, readable.substr(0, 11)),
                           Content(NODE_LABEL, nodeLabel)}}),
          m_dao{dao}
    {
    }

    Period::Period(dao *dao, const eosio::checksum256 &hash) : Document(dao->get_self(), hash), m_dao{dao}
    {
    }

    eosio::time_point Period::getStartTime()
    {
        return getContentWrapper().getOrFail(DETAILS, START_TIME)->getAs<eosio::time_point>();
    }

    std::string Period::getReadable()
    {
        return getContentWrapper().getOrFail(SYSTEM, READABLE_START_TIME)->getAs<std::string>();
    }

    std::string Period::getReadableDate()
    {
        return getContentWrapper().getOrFail(SYSTEM, READABLE_START_DATE)->getAs<std::string>();
    }

    std::string Period::getNodeLabel()
    {
        return getContentWrapper().getOrFail(SYSTEM, NODE_LABEL)->getAs<std::string>();
    }

    std::string Period::getLabel()
    {
        return getContentWrapper().getOrFail(DETAILS, LABEL)->getAs<std::string>();
    }

    eosio::time_point Period::getEndTime()
    {
        return next().getStartTime();
    }

    Period Period::createNext(const eosio::time_point &nextPeriodStart,
                              const std::string &label)
    {
        Period nextPeriod(m_dao, nextPeriodStart, label);
        Edge::write(m_dao->get_self(), m_dao->get_self(), getHash(), nextPeriod.getHash(), common::NEXT);
        return nextPeriod;
    }

    bool Period::isEnd()
    {
        auto [exists, period] = Edge::getIfExists(m_dao->get_self(), getHash(), common::NEXT);
        if (exists)
            return true;
        return false;
    }

    Period Period::asOf(dao *dao, eosio::time_point moment)
    {
        TRACE_FUNCTION()
        auto [exists, startEdge] = Edge::getIfExists(dao->get_self(), getRoot(dao->get_self()), common::START);
        EOS_CHECK(exists, "Root node does not have a 'start' edge.");
        Period period(dao, startEdge.getToNode());

        EOS_CHECK(period.getStartTime() < moment,
                     "start_period is in the future. No period found.");

        while (period.getEndTime() < moment)
        {
            period = period.next();
        }
        return period;
    }

    Period Period::current(dao *dao)
    {
        return asOf(dao, eosio::current_time_point());
    }

    Period Period::next()
    {
        TRACE_FUNCTION()
        auto [exists, period] = Edge::getIfExists(m_dao->get_self(), getHash(), common::NEXT);
        EOS_CHECK(exists, "End of calendar has been reached. Contact administrator to add more time periods.");
        return Period(m_dao, period.getToNode());
    }

    Period Period::getNthPeriodAfter(int64_t count) const
    {
        TRACE_FUNCTION()
        EOS_CHECK(
          count >= 0, 
          "Period::getNthPeriodAfter: Count has to be greater or equal to 0"
        );

        Period next = *this;

        while (count-- > 0)
        {
            if (auto [hasNext, edge] = Edge::getIfExists(m_dao->get_self(), next.getHash(), common::NEXT);
                hasNext) 
            {
                next = Period(m_dao, edge.getToNode());
            }
            else
            {
                EOS_CHECK(false, "End of calendar has been reached. Contact administrator to add more time periods.");
            }
        }

        return next;
    }

    int64_t Period::getPeriodCountTo(Period& other)
    {
      TRACE_FUNCTION()
      auto otherStartSec = other.getStartTime().sec_since_epoch();
      auto currentStartSec = getStartTime().sec_since_epoch();

      bool isAfterOther = currentStartSec > otherStartSec;

      auto startPeriod = isAfterOther ? other : *this;
      auto& endPeriod = isAfterOther ? *this : other;

      int64_t count = 0;

      while (startPeriod.getHash() != endPeriod.getHash()) {
        ++count;
        startPeriod = startPeriod.next();
      }

      if (isAfterOther) {
        count *= -1;
      }

      return count;
    }
} // namespace hypha