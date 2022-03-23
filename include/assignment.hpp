#pragma once

#include <string_view>

#include <eosio/name.hpp>
#include <eosio/crypto.hpp>
#include <period.hpp>
#include <member.hpp>
#include <recurring_activity.hpp>

namespace hypha
{
    class dao;
    class TimeShare;
    class Settings;

    class Assignment : public RecurringActivity
    {
        public:
            Assignment(dao *dao, uint64_t id);

            std::optional <Period> getNextClaimablePeriod();
            bool isClaimed(Period *period);

            TimeShare getInitialTimeShare();
            TimeShare getCurrentTimeShare();
            TimeShare getLastTimeShare();

            eosio::asset getRewardSalary();
            eosio::asset getVoiceSalary();
            eosio::asset getPegSalary();

            inline uint64_t getDaoID()
            {
                return(m_daoID);
            }

        private:
            eosio::asset getAsset(std::string_view key);
    };
} // namespace hypha
