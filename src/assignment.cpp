#include <period.hpp>
#include <common.hpp>
#include <util.hpp>
#include <dao.hpp>
#include <document_graph/edge.hpp>
#include <document_graph/content_wrapper.hpp>
#include <member.hpp>
#include <assignment.hpp>
#include <time_share.hpp>
#include <document_graph/util.hpp>
#include <logger/logger.hpp>

namespace hypha
{
    Assignment::Assignment(dao *dao, const eosio::checksum256 &hash) : Document(dao->get_self(), hash), m_dao{dao}
    {
        TRACE_FUNCTION()
        auto [idx, docType] = getContentWrapper().get(SYSTEM, TYPE);

        EOS_CHECK(idx != -1, "Content item labeled 'type' is required for this document but not found.");
        EOS_CHECK(docType->getAs<eosio::name>() == common::ASSIGNMENT,
                     "invalid document type. Expected: " + common::ASSIGNMENT.to_string() +
                         "; actual: " + docType->getAs<eosio::name>().to_string());
    }

    Assignment::Assignment(dao *dao, ContentWrapper &assignment) : Document(dao->get_self(), dao->get_self(), assignment.getContentGroups()), m_dao{dao}
    {
        TRACE_FUNCTION()
        // assignee must exist and be a DHO member
        name assignee = assignment.getOrFail(DETAILS, ASSIGNEE)->getAs<eosio::name>();

        EOS_CHECK(Member::isMember(dao->get_self(), assignee), "only members can be assigned to assignments " + assignee.to_string());

        Document roleDocument(dao->get_self(), assignment.getOrFail(DETAILS, ROLE_STRING)->getAs<eosio::checksum256>());
        auto role = roleDocument.getContentWrapper();

        // role in the proposal must be of type: role
        EOS_CHECK(role.getOrFail(SYSTEM, TYPE)->getAs<eosio::name>() == common::ROLE_NAME,
                     "role document hash provided in assignment proposal is not of type: role");

        // time_share_x100 is required and must be greater than zero and less than 100%
        int64_t timeShare = assignment.getOrFail(DETAILS, TIME_SHARE)->getAs<int64_t>();
        EOS_CHECK(timeShare > 0, TIME_SHARE + string(" must be greater than zero. You submitted: ") + std::to_string(timeShare));
        EOS_CHECK(timeShare <= 10000, TIME_SHARE + string(" must be less than or equal to 10000 (=100%). You submitted: ") + std::to_string(timeShare));

        // retrieve the minimum time_share from the role, if it exists, and check the assignment against it
        if (auto [idx, minTimeShare] = role.get(DETAILS, MIN_TIME_SHARE); minTimeShare)
        {
            EOS_CHECK(timeShare >= minTimeShare->getAs<int64_t>(),
                         TIME_SHARE + string(" must be greater than or equal to the role configuration. Role value for ") +
                             MIN_TIME_SHARE + " is " + std::to_string(minTimeShare->getAs<int64_t>()) +
                             ", and you submitted: " + std::to_string(timeShare));
        }

        // deferred_x100 is required and must be greater than or equal to zero and less than or equal to 10000
        int64_t deferred = assignment.getOrFail(DETAILS, DEFERRED)->getAs<int64_t>();
        EOS_CHECK(deferred >= 0, DEFERRED + string(" must be greater than or equal to zero. You submitted: ") + std::to_string(deferred));
        EOS_CHECK(deferred <= 10000, DEFERRED + string(" must be less than or equal to 10000 (=100%). You submitted: ") + std::to_string(deferred));

        // retrieve the minimum deferred from the role, if it exists, and check the assignment against it
        if (auto [idx, minDeferred] = role.get(DETAILS, MIN_DEFERRED); minDeferred)
        {
            EOS_CHECK(deferred >= minDeferred->getAs<int64_t>(),
                         DEFERRED + string(" must be greater than or equal to the role configuration. Role value for ") +
                             MIN_DEFERRED + " is " + std::to_string(minDeferred->getAs<int64_t>()) + ", and you submitted: " + std::to_string(deferred));
        }

        // START_PERIOD - number of periods the assignment is valid for
        auto detailsGroup = assignment.getGroupOrFail(DETAILS);
        if (auto [idx, startPeriod] = assignment.get(DETAILS, START_PERIOD); startPeriod)
        {
            EOS_CHECK(std::holds_alternative<eosio::checksum256>(startPeriod->value),
                         "fatal error: expected to be a checksum256 type: " + startPeriod->label);

            // verifies the period as valid
            Period period(m_dao, std::get<eosio::checksum256>(startPeriod->value));
        }
        else
        {
            // default START_PERIOD to next period
            ContentWrapper::insertOrReplace(*detailsGroup, Content{START_PERIOD, Period::current(m_dao).next().getHash()});
        }

        // PERIOD_COUNT - number of periods the assignment is valid for
        if (auto [idx, periodCount] = assignment.get(DETAILS, PERIOD_COUNT); periodCount)
        {
            EOS_CHECK(std::holds_alternative<int64_t>(periodCount->value),
                         "fatal error: expected to be an int64 type: " + periodCount->label);

            EOS_CHECK(std::get<int64_t>(periodCount->value) < 26,
                         PERIOD_COUNT + string(" must be less than 26. You submitted: ") + std::to_string(std::get<int64_t>(periodCount->value)));
        }
        else
        {
            // default PERIOD_COUNT to 13
            ContentWrapper::insertOrReplace(*detailsGroup, Content{PERIOD_COUNT, 13});
        }
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
        //For assignemnts approved/claimed post adjust commitment
        else if (auto [hasTimeShare, edge] = Edge::getIfExists(m_dao->get_self(), getHash(), common::INIT_TIME_SHARE);
                 hasTimeShare)
        {
          return getInitialTimeShare().
                 getContentWrapper().
                 getOrFail(DETAILS, TIME_SHARE_START_DATE)->getAs<eosio::time_point>();
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

        auto approvedTime = getApprovedTime().sec_since_epoch();
        auto currentTime = eosio::current_time_point().sec_since_epoch();

        while (counter < periodCount)
        {

            auto startTime = period.getStartTime().sec_since_epoch();
            auto endTime = period.getEndTime().sec_since_epoch();
            if ((startTime >= approvedTime || // if period comes after assignment creation
                 approvedTime < endTime) &&  //Or if it was approved in the middle of a period
                 endTime <= currentTime &&   // if period has lapsed
                !isClaimed(&period))         // and not yet claimed
            {
                return std::optional<Period>{period};
            }
            period = period.next();
            counter++;
        }
        return std::nullopt;
    }

    eosio::asset Assignment::getAsset(const symbol *symbol, const std::string &key)
    {
        TRACE_FUNCTION()
        if (auto [idx, assetContent] = getContentWrapper().get(DETAILS, key); assetContent)
        {
            EOS_CHECK(std::holds_alternative<eosio::asset>(assetContent->value), "fatal error: expected token type must be an asset value type: " + assetContent->label);
            return std::get<eosio::asset>(assetContent->value);
        }
        return eosio::asset{0, *symbol};
    }

    eosio::asset Assignment::getSalaryAmount(const eosio::symbol *symbol)
    {
        TRACE_FUNCTION()
        switch (symbol->code().raw())
        {
        case common::S_PEG.code().raw():
            return getAsset(symbol, HUSD_SALARY_PER_PERIOD);

        case common::S_VOICE.code().raw():
            return getAsset(symbol, HVOICE_SALARY_PER_PERIOD);

        case common::S_REWARD.code().raw():
            return getAsset(symbol, HYPHA_SALARY_PER_PERIOD);
        }

        return eosio::asset{0, *symbol};
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