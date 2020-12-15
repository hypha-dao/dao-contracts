#pragma once
#include <eosio/name.hpp>
#include <eosio/crypto.hpp>

#include <document_graph/document.hpp>

// #include <dao.hpp>

namespace hypha
{
    class dao;

    class Period : Document
    {
    public:
        // Period(dao &dao);

        Period(dao &dao,
               const eosio::time_point &start_time,
               const std::string &label);

        Period(dao &dao, const eosio::checksum256 &hash);

        eosio::time_point getStartTime();
        std::optional<eosio::time_point> getEndTime();
        std::string getLabel();

        const eosio::checksum256 &getHash();

        Period createNext(const eosio::time_point &nextPeriodStart,
                          const std::string &label);

        std::optional<Period> next();

        dao &m_dao;
        // Period* nextPeriod;
    };
} // namespace hypha