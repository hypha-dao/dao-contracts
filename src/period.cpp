#include <period.hpp>
#include <common.hpp>
#include <util.hpp>
#include <dao.hpp>
#include <document_graph/edge.hpp>

namespace hypha
{
    Period::Period(dao *dao,
                   const eosio::time_point &start_time,
                   const std::string &label)
        : Document(dao->get_self(), dao->get_self(),
                   std::vector<ContentGroup>{
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

    Period::Period(dao *dao, const eosio::checksum256 &hash) : Document(dao->get_self(), hash), m_dao{dao}
    {
    }

    eosio::time_point Period::getStartTime()
    {
        return getContentWrapper().getOrFail(DETAILS, START_TIME)->getAs<eosio::time_point>();
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

    Period Period::current(dao *dao)
    {
        auto [exists, startEdge] = Edge::getIfExists(dao->get_self(), getRoot(dao->get_self()), common::START);
        eosio::check(exists, "Root node does not have a 'start' edge.");
        Period period (dao, startEdge.getToNode());

        eosio::check(period.getStartTime() < eosio::current_time_point(), 
            "start_period is in the future. No current period found.");

        while (period.getEndTime() < eosio::current_time_point()) {
            period = period.next();
        }
        return period;
    }

    Period Period::next()
    {
        auto [exists, period] = Edge::getIfExists(m_dao->get_self(), getHash(), common::NEXT);
        eosio::check(exists, "End of calendar has been reached. Contact administrator to add more time periods.");
        return Period(m_dao, period.getToNode());
    }
} // namespace hypha