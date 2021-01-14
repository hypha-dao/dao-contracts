#pragma once
#include <eosio/name.hpp>
#include <eosio/crypto.hpp>

#include <document_graph/document.hpp>

namespace hypha
{
    class dao;

    class Period : public Document
    {
    public:
        Period(dao *dao,
               const eosio::time_point &start_time,
               const std::string &label);

        Period(dao *dao,
               const eosio::time_point &start_time,
               const std::string &label,
               const std::string &readable,
               const std::string &nodeLabel);

        Period(dao *dao, const eosio::checksum256 &hash);

        eosio::time_point getStartTime();
        eosio::time_point getEndTime();
        std::string getLabel();
        std::string getReadable();
        std::string getReadableDate();

        std::string getNodeLabel();

        Period createNext(const eosio::time_point &nextPeriodStart,
                          const std::string &label);

        Period next();

        static Period asOf(dao *dao, eosio::time_point moment);
        static Period current(dao *dao);
        bool isEnd();

        dao *m_dao;
    };
} // namespace hypha