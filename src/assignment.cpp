#include <assignment.hpp>

#include <math.h>

#include <period.hpp>
#include <common.hpp>
#include <util.hpp>
#include <dao.hpp>
#include <document_graph/edge.hpp>
#include <document_graph/content_wrapper.hpp>
#include <member.hpp>
#include <time_share.hpp>
#include <document_graph/util.hpp>
#include <logger/logger.hpp>

namespace hypha
{
    Assignment::Assignment(dao *dao, uint64_t id)
        : RecurringActivity(dao, id)
    {
        TRACE_FUNCTION()
        auto [idx, docType] = getContentWrapper().get(SYSTEM, TYPE);

        EOS_CHECK(idx != -1, "Content item labeled 'type' is required for this document but not found.");
        EOS_CHECK(docType->getAs<eosio::name>() == common::ASSIGNMENT,
                  "invalid document type. Expected: " + common::ASSIGNMENT.to_string() +
                  "; actual: " + docType->getAs<eosio::name>().to_string());
    }


    bool Assignment::isClaimed(Period *period)
    {
        TRACE_FUNCTION()
        return(Edge::exists(m_dao->get_self(), getID(), period->getID(), common::CLAIMED));
    }


    std::optional<Period> Assignment::getNextClaimablePeriod()
    {
        TRACE_FUNCTION()
        // Ensure that the claimed period is within the approved period count
        Period period = getStartPeriod();
        int64_t periodCount = getContentWrapper().getOrFail(DETAILS, PERIOD_COUNT)->getAs<int64_t>();
        int64_t counter     = 0;

        auto currentTime = eosio::current_time_point().sec_since_epoch();

        while (counter < periodCount)
        {
            auto endTime = period.getEndTime().sec_since_epoch();
            if ((endTime <= currentTime) && // if period has lapsed
                !isClaimed(&period))        // and not yet claimed
            {
                return(std::optional<Period> { period });
            }
            period = period.next();
            counter++;
        }
        return(std::nullopt);
    }


    eosio::asset Assignment::getRewardSalary()
    {
        auto cw = getContentWrapper();

        asset usdPerPeriod = cw
                                .getOrFail(DETAILS, USD_SALARY_PER_PERIOD)->getAs<eosio::asset>();

        int64_t initialTimeshare = getInitialTimeShare()
                                      .getContentWrapper()
                                      .getOrFail(DETAILS, TIME_SHARE)->getAs<int64_t>();

        double usdPerPeriodCommitmentAdjusted = normalizeToken(usdPerPeriod) * (initialTimeshare / 100.0);

        double deferred = static_cast<float>(cw.getOrFail(DETAILS, DEFERRED)->getAs<int64_t>()) / 100.0;

        auto rewardToPegVal = normalizeToken(m_daoSettings->getOrFail<eosio::asset>(common::REWARD_TO_PEG_RATIO));

        auto rewardPerPeriod = usdPerPeriodCommitmentAdjusted * deferred / rewardToPegVal;

        auto rewardToken = m_daoSettings->getOrFail<eosio::asset>(common::REWARD_TOKEN);

        return(denormalizeToken(rewardPerPeriod, rewardToken));
    }


    eosio::asset Assignment::getVoiceSalary()
    {
        return(getContentWrapper()
                  .getOrFail(DETAILS, common::VOICE_SALARY_PER_PERIOD)->getAs<eosio::asset>());
    }


    eosio::asset Assignment::getPegSalary()
    {
        return(getContentWrapper()
                  .getOrFail(DETAILS, common::PEG_SALARY_PER_PERIOD)->getAs<eosio::asset>());
    }


    TimeShare Assignment::getInitialTimeShare()
    {
        Edge initialEdge = Edge::get(m_dao->get_self(), getID(), common::INIT_TIME_SHARE);

        return(TimeShare(m_dao->get_self(), initialEdge.getToNode()));
    }


    TimeShare Assignment::getCurrentTimeShare()
    {
        Edge currentEdge = Edge::get(m_dao->get_self(), getID(), common::CURRENT_TIME_SHARE);

        return(TimeShare(m_dao->get_self(), currentEdge.getToNode()));
    }


    TimeShare Assignment::getLastTimeShare()
    {
        Edge lastEdge = Edge::get(m_dao->get_self(), getID(), common::LAST_TIME_SHARE);

        return(TimeShare(m_dao->get_self(), lastEdge.getToNode()));
    }
} // namespace hypha
