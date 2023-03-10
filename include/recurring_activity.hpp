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
    /**
     * @brief Used to wrap functionality of Assignments & Badge Assignments
     * 
     */
    class RecurringActivity : public Document
    {
    public:
        RecurringActivity(dao *dao, uint64_t id);
        Period getStartPeriod();
        Period getLastPeriod();
        Member getAssignee();
        eosio::time_point getApprovedTime();
        int64_t getPeriodCount();

        dao *m_dao;

        inline uint64_t getDaoID() { return m_daoID; }
        void scheduleArchive();

        static bool isRecurringActivity(Document& doc);

        eosio::time_point getStartDate();
        eosio::time_point getEndDate();
    protected: 
        uint64_t m_daoID;
        Settings* m_daoSettings;
    };
} // namespace hypha