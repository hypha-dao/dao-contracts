#pragma once
#include <eosio/name.hpp>
#include <eosio/crypto.hpp>
#include <period.hpp>
#include <member.hpp>
#include <document_graph/document.hpp>

namespace hypha
{
    class dao;
    class TimeShare;
    
    class Assignment : public Document
    {
    public:
        Assignment(dao *dao, const eosio::checksum256 &hash);
        Assignment(dao *dao, ContentWrapper &assignment);

        std::optional<Period> getNextClaimablePeriod ();
        bool isClaimed (Period* period);
        Period getStartPeriod();
        Period getLastPeriod();
        Member getAssignee();
        eosio::name &getType();
        eosio::time_point getApprovedTime();
        int64_t getPeriodCount();

        eosio::asset getSalaryAmount (const eosio::symbol* symbol);

        TimeShare getInitialTimeShare();
        TimeShare getCurrentTimeShare();
        TimeShare getLastTimeShare();

        eosio::asset calcLiquidSeedsSalary ();
        eosio::asset calcHusdSalary ();
        eosio::asset calcHyphaSalary ();
        eosio::asset getHyphaSalary();

        dao *m_dao;

    private: 
        eosio::asset getAsset (const eosio::symbol* symbol, const std::string &key);
    };
} // namespace hypha