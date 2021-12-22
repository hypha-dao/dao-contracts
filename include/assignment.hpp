#pragma once

#include <string_view>

#include <eosio/name.hpp>
#include <eosio/crypto.hpp>
#include <period.hpp>
#include <member.hpp>
#include <document_graph/document.hpp>

namespace hypha
{
    class dao;
    class TimeShare;
    class Settings;

    class Assignment : public Document
    {
    public:
        Assignment(dao *dao, const eosio::checksum256 &hash);
        
        std::optional<Period> getNextClaimablePeriod ();
        bool isClaimed (Period* period);
        Period getStartPeriod();
        Period getLastPeriod();
        Member getAssignee();
        eosio::name &getType();
        eosio::time_point getApprovedTime();
        int64_t getPeriodCount();

        TimeShare getInitialTimeShare();
        TimeShare getCurrentTimeShare();
        TimeShare getLastTimeShare();

        eosio::asset getRewardSalary();
        eosio::asset getVoiceSalary();
        eosio::asset getPegSalary();

        dao *m_dao;

        //inline eosio::checksum256 getDaoHash() { return m_daoHash; }
        inline uint64_t getDaoID() { return m_daoID; }
    private: 
        //eosio::checksum256 m_daoHash;
        uint64_t m_daoID;
        Settings* m_daoSettings;
        eosio::asset getAsset (std::string_view key);
    };
} // namespace hypha