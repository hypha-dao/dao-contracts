#include "recurring_activity.hpp"

#include "dao.hpp"

namespace hypha
{

RecurringActivity::RecurringActivity(dao *dao, uint64_t id)
:  Document(dao->get_self(), id),
   m_dao{dao},
   m_daoID{Edge::get(dao->get_self(), getID(), common::DAO).getToNode()},
   m_daoSettings{dao->getSettingsDocument(m_daoID)}
{}

Period RecurringActivity::getStartPeriod()
{
    TRACE_FUNCTION()
    return Period(
        m_dao, 
        m_dao->getGraph().getEdgesFromOrFail(
            getID(),
            common::START
        )[0].getToNode()
    );
}

Period RecurringActivity::getLastPeriod() 
{
    auto start = getStartPeriod();
    auto periods = getPeriodCount();
    return start.getNthPeriodAfter(periods-1);
}

Member RecurringActivity::getAssignee()
{
    TRACE_FUNCTION()

    if (auto [idx, assignee] = getContentWrapper().get(DETAILS, ASSIGNEE);
        assignee) {
        return Member(*m_dao, m_dao->getMemberID(assignee->getAs<name>()));
    }
    else {
        return Member(*m_dao, Edge::get(m_dao->get_self(), getID(), common::ASSIGNEE_NAME).getToNode());
    }
}

eosio::time_point RecurringActivity::getApprovedTime()
{
    TRACE_FUNCTION()
    auto cw = getContentWrapper();

    auto [detailsIdx, _] = cw.getGroup(DETAILS);

    EOS_CHECK(detailsIdx != -1, util::to_str("Missing details group for assignment [", getID(), "]"));

    if (auto [idx, legacyCreatedDate] = cw.get(SYSTEM, "legacy_object_created_date"); legacyCreatedDate)
    {
        EOS_CHECK(std::holds_alternative<eosio::time_point>(legacyCreatedDate->value), "fatal error: expected time_point type: " + legacyCreatedDate->label);
        return std::get<eosio::time_point>(legacyCreatedDate->value);
    }
    //All assignments should eventually contain this item 
    else if (auto [approvedIdx, approvedDate] = cw.get(SYSTEM, common::APPROVED_DATE);
                approvedDate)
    {
        return approvedDate->getAs<eosio::time_point>();
    }
            
    //Fallback for old assignments without time share document
    return Edge::get(m_dao->get_self(), getAssignee().getID(), common::ASSIGNED).getCreated();
}

int64_t RecurringActivity::getPeriodCount()
{
    TRACE_FUNCTION()
    return getContentWrapper().getOrFail(DETAILS, PERIOD_COUNT)->getAs<int64_t>();
}

void RecurringActivity::scheduleArchive()
{
    //Schedule a trx to close the proposal
    eosio::transaction trx;
    trx.actions.emplace_back(eosio::action(
        eosio::permission_level(m_dao->get_self(), "active"_n),
        m_dao->get_self(),
        "archiverecur"_n,
        std::make_tuple(getID())
    ));

    auto expiration = getLastPeriod().getEndTime().sec_since_epoch();

    constexpr auto aditionalDelaySec = 60;
    trx.delay_sec = (expiration - eosio::current_time_point().sec_since_epoch()) + aditionalDelaySec;

    auto dhoSettings = m_dao->getSettingsDocument();

    auto nextID = dhoSettings->getSettingOrDefault("next_schedule_id", int64_t(0));

    trx.send(nextID, m_dao->get_self());

    dhoSettings->setSetting(Content{"next_schedule_id", nextID + 1});
}

} // namespace hypha
