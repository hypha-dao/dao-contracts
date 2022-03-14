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

        Period(dao *dao, uint64_t id);

        eosio::time_point getStartTime();
        eosio::time_point getEndTime();
        std::string getLabel();
        std::string getReadable();
        std::string getReadableDate();

        std::string getNodeLabel();

        Period createNext(const eosio::time_point &nextPeriodStart,
                          const std::string &label);

        Period next();

        Period getNthPeriodAfter(int64_t count) const;
        int64_t getPeriodCountTo(Period& other);
        Period getPeriodUntil(eosio::time_point moment);

        static Period asOf(dao *dao, uint64_t daoID, eosio::time_point moment);
        
        static Period current(dao *dao, uint64_t daoID);
        bool isEnd();

        dao *m_dao;
    };
} // namespace hypha