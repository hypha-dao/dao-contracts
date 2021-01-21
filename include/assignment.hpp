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
        Member getAssignee();
        eosio::name &getType();
        eosio::time_point getApprovedTime();

        eosio::asset getSalaryAmount (const eosio::symbol* symbol, Period* period);
        eosio::asset getSalaryAmount (const eosio::symbol* symbol);

        TimeShare getInitialTimeShare();
        TimeShare getCurrentTimeShare();
        TimeShare getLastTimeShare();

        eosio::asset calcDSeedsSalary (Period* period);
        eosio::asset calcLiquidSeedsSalary ();
        eosio::asset calcHusdSalary ();
        eosio::asset calcHyphaSalary ();

        dao *m_dao;
        // ContentWrapper contentWrapper;

    private: 
        eosio::asset getAsset (const symbol* symbol, const std::string &key);
       
    };
} // namespace hypha