#include "recurring_activity.hpp"

#include "dao.hpp"

namespace hypha
{
    RecurringActivity::RecurringActivity(dao *dao, uint64_t id)
        :  Document(dao->get_self(), id),
        m_dao{dao},
        m_daoID{Edge::get(dao->get_self(), getID(), common::DAO).getToNode()},
        m_daoSettings{dao->getSettingsDocument(m_daoID)}
    {
    }

    Period RecurringActivity::getStartPeriod()
    {
        TRACE_FUNCTION()
        return(Period(
                   m_dao,
                   static_cast <uint64_t>(
                       getContentWrapper()
                       .getOrFail(DETAILS, START_PERIOD)
                       ->getAs <int64_t>()
                       )
                   ));
    }

    Period RecurringActivity::getLastPeriod()
    {
        auto start   = getStartPeriod();
        auto periods = getPeriodCount();

        return(start.getNthPeriodAfter(periods - 1));
    }

    Member RecurringActivity::getAssignee()
    {
        TRACE_FUNCTION()
        return(Member(*m_dao, Edge::get(m_dao->get_self(), getID(), common::ASSIGNEE_NAME).getToNode()));
    }

    eosio::time_point RecurringActivity::getApprovedTime()
    {
        TRACE_FUNCTION()
        auto cw = getContentWrapper();

        auto [detailsIdx, _] = cw.getGroup(DETAILS);

        EOS_CHECK(detailsIdx != -1, util::to_str("Missing details group for assignment [", getID(), "]"));

        if (auto [idx, legacyCreatedDate] = cw.get(SYSTEM, "legacy_object_created_date"); legacyCreatedDate)
        {
            EOS_CHECK(std::holds_alternative <eosio::time_point>(legacyCreatedDate->value), "fatal error: expected time_point type: " + legacyCreatedDate->label);
            return(std::get <eosio::time_point>(legacyCreatedDate->value));
        }
        //All assignments should eventually contain this item
        else if (auto [approvedIdx, approvedDate] = cw.get(SYSTEM, common::APPROVED_DATE);
                 approvedDate)
        {
            return(approvedDate->getAs <eosio::time_point>());
        }

        //Fallback for old assignments without time share document
        return(Edge::get(m_dao->get_self(), getAssignee().getID(), common::ASSIGNED).getCreated());
    }

    int64_t RecurringActivity::getPeriodCount()
    {
        TRACE_FUNCTION()
        return(getContentWrapper().getOrFail(DETAILS, PERIOD_COUNT)->getAs <int64_t>());
    }
} // namespace hypha
