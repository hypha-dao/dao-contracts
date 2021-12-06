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
    Assignment::Assignment(dao *dao, const eosio::checksum256 &hash) 
    : Document(dao->get_self(), hash),
      m_dao{dao},
      m_daoHash{Edge::get(dao->get_self(), hash, common::DAO).getToNode()},
      m_daoSettings{dao->getSettingsDocument(m_daoHash)}
    {
        TRACE_FUNCTION()
        auto [idx, docType] = getContentWrapper().get(SYSTEM, TYPE);

        EOS_CHECK(idx != -1, "Content item labeled 'type' is required for this document but not found.");
        EOS_CHECK(docType->getAs<eosio::name>() == common::ASSIGNMENT,
                     "invalid document type. Expected: " + common::ASSIGNMENT.to_string() +
                         "; actual: " + docType->getAs<eosio::name>().to_string());
    }

    // ContentGroups Assignment::defaultContent (const eosio::name &member)
    // {
    //     return ContentGroups{
    //         ContentGroup{
    //             Content(CONTENT_GROUP_LABEL, DETAILS),
    //             Content(MEMBER_STRING, member)},
    //         ContentGroup{
    //             Content(CONTENT_GROUP_LABEL, SYSTEM),
    //             Content(TYPE, common::MEMBER),
    //             Content(NODE_LABEL, member.to_string())}};
    // }

    Member Assignment::getAssignee()
    { 
        TRACE_FUNCTION()
        return Member(*m_dao, Edge::get(m_dao->get_self(), getHash(), common::ASSIGNEE_NAME).getToNode());
    }

    int64_t Assignment::getPeriodCount() 
    {
        TRACE_FUNCTION()
        return getContentWrapper().getOrFail(DETAILS, PERIOD_COUNT)->getAs<int64_t>();
    }

    // TODO: move to proposal class (needs some further design)
    eosio::time_point Assignment::getApprovedTime()
    { 
        TRACE_FUNCTION()
        auto cw = getContentWrapper();

        auto [detailsIdx, _] = cw.getGroup(DETAILS);

        EOS_CHECK(detailsIdx != -1, "Missing details group for assignment [" + readableHash(getHash()) + "]");

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
        return Edge::get(m_dao->get_self(), getAssignee().getHash(), common::ASSIGNED).getCreated();
    }

    bool Assignment::isClaimed(Period *period)
    {
        TRACE_FUNCTION()
        return Edge::exists(m_dao->get_self(), getHash(), period->getHash(), common::CLAIMED);
    }

    std::optional<Period> Assignment::getNextClaimablePeriod()
    {
        TRACE_FUNCTION()
        // Ensure that the claimed period is within the approved period count
        Period period = getStartPeriod();
        int64_t periodCount = getContentWrapper().getOrFail(DETAILS, PERIOD_COUNT)->getAs<int64_t>();
        int64_t counter = 0;

        auto currentTime = eosio::current_time_point().sec_since_epoch();

        while (counter < periodCount)
        {
            auto endTime = period.getEndTime().sec_since_epoch();
            if (endTime <= currentTime &&   // if period has lapsed
                !isClaimed(&period))         // and not yet claimed
            {
                return std::optional<Period>{period};
            }
            period = period.next();
            counter++;
        }
        return std::nullopt;
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
    
      return denormalizeToken(rewardPerPeriod, rewardToken);
    }

    eosio::asset Assignment::getVoiceSalary()
    {
        return getContentWrapper()
               .getOrFail(DETAILS, common::VOICE_SALARY_PER_PERIOD)->getAs<eosio::asset>();
    }

    eosio::asset Assignment::getPegSalary()
    {
        return getContentWrapper()
               .getOrFail(DETAILS, common::PEG_SALARY_PER_PERIOD)->getAs<eosio::asset>();
    }

    Period Assignment::getStartPeriod()
    {
      TRACE_FUNCTION()
      return Period(m_dao, getContentWrapper().getOrFail(DETAILS, START_PERIOD)->getAs<eosio::checksum256>());
    }
    
    Period Assignment::getLastPeriod()
    {
      auto start = getStartPeriod();
      auto periods = getPeriodCount();
      return start.getNthPeriodAfter(periods-1);
    }

    TimeShare Assignment::getInitialTimeShare() 
    {
      Edge initialEdge = Edge::get(m_dao->get_self(), getHash(), common::INIT_TIME_SHARE);
      return TimeShare(m_dao->get_self(), initialEdge.getToNode());
    }
    
    TimeShare Assignment::getCurrentTimeShare() 
    {
      Edge currentEdge = Edge::get(m_dao->get_self(), getHash(), common::CURRENT_TIME_SHARE);

      return TimeShare(m_dao->get_self(), currentEdge.getToNode());
    }
    
    TimeShare Assignment::getLastTimeShare() 
    {
      Edge lastEdge = Edge::get(m_dao->get_self(), getHash(), common::LAST_TIME_SHARE);

      return TimeShare(m_dao->get_self(), lastEdge.getToNode());
    }
} // namespace hypha