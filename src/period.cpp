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
                   ContentGroups{
                       ContentGroup{
                           Content(CONTENT_GROUP_LABEL, DETAILS),
                           Content(START_TIME, start_time),
                           Content(LABEL, label)},
                       ContentGroup{
                           Content(CONTENT_GROUP_LABEL, SYSTEM),
                           Content(TYPE, common::PERIOD)}}),
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

    std::optional<eosio::time_point> Period::getEndTime()
    {
        auto nextPeriod = next();
        if (nextPeriod == std::nullopt)
        {
            // eosio::print ("getEndTime(): next period does NOT exists");
            return std::nullopt;
        }
        return std::optional<eosio::time_point>(nextPeriod.value().getStartTime());
    }

    Period Period::createNext(const eosio::time_point &nextPeriodStart,
                              const std::string &label)
    {
        Period nextPeriod(m_dao, nextPeriodStart, label);
        Edge::write(m_dao->get_self(), m_dao->get_self(), getHash(), nextPeriod.getHash(), common::NEXT);
        return nextPeriod;
    }

    std::optional<Period> Period::next()
    {
        // eosio::print (" check if edge exists: " + readableHash(getHash()) + "\n");
        auto [exists, period] = Edge::getIfExists(m_dao->get_self(), getHash(), common::NEXT);
        if (exists)
        {
            // eosio::print ("next period exists: " + readableHash(period.to_node) + "\n");
            return std::optional<Period>{Period(m_dao, period.to_node)};
        }
        // eosio::print ("next(): next period does NOT exists");
        return std::nullopt;
    }
} // namespace hypha