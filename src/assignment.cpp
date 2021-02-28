#include <period.hpp>
#include <common.hpp>
#include <util.hpp>
#include <dao.hpp>
#include <document_graph/edge.hpp>
#include <document_graph/content_wrapper.hpp>
#include <member.hpp>
#include <assignment.hpp>
#include <time_share.hpp>

namespace hypha
{
    Assignment::Assignment(dao *dao, const eosio::checksum256 &hash) : Document(dao->get_self(), hash), m_dao{dao}
    {
        auto [idx, docType] = getContentWrapper().get(SYSTEM, TYPE);

        eosio::check(idx != -1, "Content item labeled 'type' is required for this document but not found.");
        eosio::check(docType->getAs<eosio::name>() == common::ASSIGNMENT,
                     "invalid document type. Expected: " + common::ASSIGNMENT.to_string() +
                         "; actual: " + docType->getAs<eosio::name>().to_string());
    }

    Assignment::Assignment(dao *dao, ContentWrapper &assignment) : Document(dao->get_self(), dao->get_self(), assignment.getContentGroups()), m_dao{dao}
    {
        // assignee must exist and be a DHO member
        name assignee = assignment.getOrFail(DETAILS, ASSIGNEE)->getAs<eosio::name>();

        eosio::check(Member::isMember(dao->get_self(), assignee), "only members can be assigned to assignments " + assignee.to_string());

        Document roleDocument(dao->get_self(), assignment.getOrFail(DETAILS, ROLE_STRING)->getAs<eosio::checksum256>());
        auto role = roleDocument.getContentWrapper();

        // role in the proposal must be of type: role
        eosio::check(role.getOrFail(SYSTEM, TYPE)->getAs<eosio::name>() == common::ROLE_NAME,
                     "role document hash provided in assignment proposal is not of type: role");

        // time_share_x100 is required and must be greater than zero and less than 100%
        int64_t timeShare = assignment.getOrFail(DETAILS, TIME_SHARE)->getAs<int64_t>();
        eosio::check(timeShare > 0, TIME_SHARE + string(" must be greater than zero. You submitted: ") + std::to_string(timeShare));
        eosio::check(timeShare <= 10000, TIME_SHARE + string(" must be less than or equal to 10000 (=100%). You submitted: ") + std::to_string(timeShare));

        // retrieve the minimum time_share from the role, if it exists, and check the assignment against it
        if (auto [idx, minTimeShare] = role.get(DETAILS, MIN_TIME_SHARE); minTimeShare)
        {
            eosio::check(timeShare >= minTimeShare->getAs<int64_t>(),
                         TIME_SHARE + string(" must be greater than or equal to the role configuration. Role value for ") +
                             MIN_TIME_SHARE + " is " + std::to_string(minTimeShare->getAs<int64_t>()) +
                             ", and you submitted: " + std::to_string(timeShare));
        }

        // deferred_x100 is required and must be greater than or equal to zero and less than or equal to 10000
        int64_t deferred = assignment.getOrFail(DETAILS, DEFERRED)->getAs<int64_t>();
        eosio::check(deferred >= 0, DEFERRED + string(" must be greater than or equal to zero. You submitted: ") + std::to_string(deferred));
        eosio::check(deferred <= 10000, DEFERRED + string(" must be less than or equal to 10000 (=100%). You submitted: ") + std::to_string(deferred));

        // retrieve the minimum deferred from the role, if it exists, and check the assignment against it
        if (auto [idx, minDeferred] = role.get(DETAILS, MIN_DEFERRED); minDeferred)
        {
            eosio::check(deferred >= minDeferred->getAs<int64_t>(),
                         DEFERRED + string(" must be greater than or equal to the role configuration. Role value for ") +
                             MIN_DEFERRED + " is " + std::to_string(minDeferred->getAs<int64_t>()) + ", and you submitted: " + std::to_string(deferred));
        }

        // START_PERIOD - number of periods the assignment is valid for
        auto detailsGroup = assignment.getGroupOrFail(DETAILS);
        if (auto [idx, startPeriod] = assignment.get(DETAILS, START_PERIOD); startPeriod)
        {
            eosio::check(std::holds_alternative<eosio::checksum256>(startPeriod->value),
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
            eosio::check(std::holds_alternative<int64_t>(periodCount->value),
                         "fatal error: expected to be an int64 type: " + periodCount->label);

            eosio::check(std::get<int64_t>(periodCount->value) < 26,
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
        return Member(*m_dao, Edge::get(m_dao->get_self(), getHash(), common::ASSIGNEE_NAME).getToNode());

    eosio::time_point Assignment::getApprovedTime()
    {
        if (auto [idx, legacyCreatedDate] = getContentWrapper().get(SYSTEM, "legacy_object_created_date"); legacyCreatedDate)
        {
            eosio::check(std::holds_alternative<eosio::time_point>(legacyCreatedDate->value), "fatal error: expected time_point type: " + legacyCreatedDate->label);
            return std::get<eosio::time_point>(legacyCreatedDate->value);
        }

        return getInitialTimeShare().getContentWrapper().getOrFail(DETAILS, TIME_SHARE_START_DATE)->getAs<time_point>();
    }

    bool Assignment::isClaimed(Period *period)
    {
        return Edge::exists(m_dao->get_self(), getHash(), period->getHash(), common::CLAIMED);
    }

    std::optional<Period> Assignment::getNextClaimablePeriod()
    {
        // Ensure that the claimed period is within the approved period count
        Period period(m_dao, getContentWrapper().getOrFail(DETAILS, START_PERIOD)->getAs<eosio::checksum256>());
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
        if (auto [idx, assetContent] = getContentWrapper().get(DETAILS, key); assetContent)
        {
            eosio::check(std::holds_alternative<eosio::asset>(assetContent->value), "fatal error: expected token type must be an asset value type: " + assetContent->label);
            return std::get<eosio::asset>(assetContent->value);
        }
        return eosio::asset{0, *symbol};
    }

    eosio::asset Assignment::getSalaryAmount(const eosio::symbol *symbol)
    {
        switch (symbol->code().raw())
        {
        case common::S_HUSD.code().raw():
            return getAsset(symbol, HUSD_SALARY_PER_PERIOD);

        case common::S_HVOICE.code().raw():
            return getAsset(symbol, HVOICE_SALARY_PER_PERIOD);

        case common::S_HYPHA.code().raw():
            return getAsset(symbol, HYPHA_SALARY_PER_PERIOD);
        }

        return eosio::asset{0, *symbol};
    }

    eosio::asset Assignment::getSalaryAmount(const eosio::symbol *symbol, Period *period)
    {
        switch (symbol->code().raw())
        {

        case common::S_SEEDS.code().raw():
            return calcDSeedsSalary(period);
        }

        return eosio::asset{0, *symbol};
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