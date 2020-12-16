#pragma once
#include <eosio/name.hpp>
#include <eosio/crypto.hpp>
#include <period.hpp>
#include <document_graph/document.hpp>

namespace hypha
{
    class dao;

    class Assignment : public Document
    {
    public:
        Assignment(dao *dao, const eosio::checksum256 &hash);

        std::optional<Period> getNextClaimablePeriod ();
        bool isClaimed (Period* period);
        Member getAssignee();
        eosio::name &getType();
        eosio::time_point getApprovedTime();

        eosio::asset getSalaryAmount (const eosio::symbol* symbol, Period* period);
        eosio::asset getSalaryAmount (const eosio::symbol* symbol);

        eosio::asset calcDSeedsSalary (Period* period);
        eosio::asset calcLiquidSeedsSalary ();
        eosio::asset calcHusdSalary ();
        eosio::asset calcHyphaSalary ();

        dao *m_dao;
        ContentWrapper contentWrapper;

    private: 
        eosio::asset getAsset (const symbol* symbol, const std::string &key);
       
    };
} // namespace hypha