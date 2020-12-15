#include <period.hpp>
#include <common.hpp>
#include <util.hpp>
#include <dao.hpp>
#include <document_graph/edge.hpp>

namespace hypha
{
    // Period::Period(dao &dao) : m_dao{dao} {}

    Period::Period(dao &dao, 
                   const eosio::time_point &start_time,
                   const std::string &label)
        : m_dao{dao}
    {
        Content startDate(START_TIME, start_time);
        Content labelContent(LABEL, label);

        ContentGroup cg{};
        cg.push_back(startDate);
        cg.push_back(labelContent);

        ContentGroups cgs{};
        cgs.push_back(cg);

        Document(m_dao.get_self(), m_dao.get_self(), cgs);
    }

    Period::Period(dao &dao, const eosio::checksum256 &hash) : m_dao{dao}
    {
        Document (m_dao.get_self(), hash);
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
            return std::nullopt;
        }        
        return std::optional<eosio::time_point>(nextPeriod->getStartTime());
    }   

    Period Period::createNext(const eosio::time_point &nextPeriodStart,
                               const std::string &label)
    {
        Period nextPeriod(m_dao, nextPeriodStart, label);
        Edge::write(m_dao.get_self(), m_dao.get_self(), getHash(), nextPeriod.getHash(), common::NEXT);
        return nextPeriod;
    }

    std::optional<Period> Period::next()
    {
        auto [exists, period] = Edge::getIfExists(m_dao.get_self(), getHash(), common::NEXT);
        if (exists)
        {
            return std::optional<Period>{Period(m_dao, period.to_node)};
        }
        return std::nullopt;
    }
} // namespace hypha