
#include <document_graph/content_wrapper.hpp>
#include <document_graph/document.hpp>

#include <proposals/assignment_proposal.hpp>
#include <member.hpp>
#include <common.hpp>
#include <dao.hpp>
#include <util.hpp>
#include <time_share.hpp>
#include <logger/logger.hpp>

namespace hypha
{

    void AssignmentProposal::proposeImpl(const name &proposer, ContentWrapper &assignment)
    {
        TRACE_FUNCTION()
        // assignee must exist and be a DHO member
        name assignee = assignment.getOrFail(DETAILS, ASSIGNEE)->getAs<eosio::name>();
        EOS_CHECK(Member::isMember(m_dao.get_self(), assignee), "only members can be assigned to assignments " + assignee.to_string());

        Document roleDocument(m_dao.get_self(), assignment.getOrFail(DETAILS, ROLE_STRING)->getAs<eosio::checksum256>());
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
            Period period(&m_dao, std::get<eosio::checksum256>(startPeriod->value));
        } else {
            // default START_PERIOD to next period
            ContentWrapper::insertOrReplace(*detailsGroup, Content{START_PERIOD, Period::current(&m_dao).next().getHash()});
        }

        // PERIOD_COUNT - number of periods the assignment is valid for
        if (auto [idx, periodCount] = assignment.get(DETAILS, PERIOD_COUNT); periodCount)
        {
            EOS_CHECK(std::holds_alternative<int64_t>(periodCount->value),
                         "fatal error: expected to be an int64 type: " + periodCount->label);

            EOS_CHECK(std::get<int64_t>(periodCount->value) < 26, PERIOD_COUNT + 
                string(" must be less than 26. You submitted: ") + std::to_string(std::get<int64_t>(periodCount->value)));

        } else {
            // default PERIOD_COUNT to 13
            ContentWrapper::insertOrReplace(*detailsGroup, Content{PERIOD_COUNT, 13});
        }

        asset annual_usd_salary = role.getOrFail(DETAILS, ANNUAL_USD_SALARY)->getAs<eosio::asset>();

        // add the USD period pay amount (this is used to calculate SEEDS at time of salary claim)
        Content usdSalaryPerPeriod(USD_SALARY_PER_PERIOD, adjustAsset(annual_usd_salary, common::PHASE_TO_YEAR_RATIO));
        ContentWrapper::insertOrReplace(*detailsGroup, usdSalaryPerPeriod);

        // add remaining derived per period salary amounts to this document
        auto husd = calculateHusd(annual_usd_salary, timeShare, deferred);
        if (husd.amount > 0) {
            Content husdSalaryPerPeriod(HUSD_SALARY_PER_PERIOD, husd);
            ContentWrapper::insertOrReplace(*detailsGroup, husdSalaryPerPeriod);
        }

        auto hypha = calculateHypha(annual_usd_salary, timeShare, deferred);
        if (hypha.amount > 0) {
            Content hyphaSalaryPerPeriod(HYPHA_SALARY_PER_PERIOD, hypha);
            ContentWrapper::insertOrReplace(*detailsGroup, hyphaSalaryPerPeriod);
        }

        auto hvoice = calculateHvoice(annual_usd_salary, timeShare);
        if (hvoice.amount > 0) {
            Content hvoiceSalaryPerPeriod(HVOICE_SALARY_PER_PERIOD, hvoice);
            ContentWrapper::insertOrReplace(*detailsGroup, hvoiceSalaryPerPeriod);
        }
    }

    void AssignmentProposal::postProposeImpl(Document &proposal)
    {
        TRACE_FUNCTION()
        Edge::write(m_dao.get_self(), m_dao.get_self(), proposal.getHash(),
                    proposal.getContentWrapper().getOrFail(DETAILS, ROLE_STRING)->getAs<eosio::checksum256>(),
                    common::ROLE_NAME);
    }

    void AssignmentProposal::passImpl(Document &proposal)
    {
        TRACE_FUNCTION()
        ContentWrapper contentWrapper = proposal.getContentWrapper();
        eosio::checksum256 assignee = Member::calcHash(contentWrapper.getOrFail(DETAILS, ASSIGNEE)->getAs<eosio::name>());
        Document role(m_dao.get_self(), contentWrapper.getOrFail(DETAILS, ROLE_STRING)->getAs<eosio::checksum256>());

        // update graph edges:
        //  member          ---- assigned           ---->   role_assignment
        //  role_assignment ---- assignee           ---->   member
        //  role_assignment ---- role               ---->   role
        //  role            ---- role_assignment    ---->   role_assignment
        Edge::write(m_dao.get_self(), m_dao.get_self(), assignee, proposal.getHash(), common::ASSIGNED);
        Edge::write(m_dao.get_self(), m_dao.get_self(), proposal.getHash(), assignee, common::ASSIGNEE_NAME);
        Edge::write(m_dao.get_self(), m_dao.get_self(), role.getHash(), proposal.getHash(), common::ASSIGNMENT);
        Edge::write(m_dao.get_self(), m_dao.get_self(), proposal.getHash(), role.getHash(), common::ROLE_NAME);

        //Start period edge
        // assignment ---- start ----> period
        eosio::checksum256 startPeriod = contentWrapper.getOrFail(DETAILS, START_PERIOD)->getAs<eosio::checksum256>();
        Edge::write(m_dao.get_self(), m_dao.get_self(), proposal.getHash(), startPeriod, common::START);

        //Initial time share for proposal
        int64_t initTimeShare = contentWrapper.getOrFail(DETAILS, TIME_SHARE)->getAs<int64_t>();
        
        //Set starting date to approval date.
        TimeShare initTimeShareDoc(m_dao.get_self(), 
                                   m_dao.get_self(), 
                                   initTimeShare, 
                                   contentWrapper.getOrFail(SYSTEM, common::APPROVED_DATE)->getAs<eosio::time_point>(),
                                   proposal.getHash());

        Edge::write(m_dao.get_self(), m_dao.get_self(), proposal.getHash(), initTimeShareDoc.getHash(), common::INIT_TIME_SHARE);
        Edge::write(m_dao.get_self(), m_dao.get_self(), proposal.getHash(), initTimeShareDoc.getHash(), common::CURRENT_TIME_SHARE);
        Edge::write(m_dao.get_self(), m_dao.get_self(), proposal.getHash(), initTimeShareDoc.getHash(), common::LAST_TIME_SHARE);
    }

    std::string AssignmentProposal::getBallotContent(ContentWrapper &contentWrapper)
    {
        TRACE_FUNCTION()
        return contentWrapper.getOrFail(DETAILS, TITLE)->getAs<std::string>();
    }

    name AssignmentProposal::getProposalType()
    {
        return common::ASSIGNMENT;
    }

    asset AssignmentProposal::calculateTimeShareUsdPerPeriod(const asset &annualUsd, const int64_t &timeShare)
    {
        asset commitment_adjusted_usd_annual = adjustAsset(annualUsd, (float)(float)timeShare / (float)100);
        return adjustAsset(commitment_adjusted_usd_annual, common::PHASE_TO_YEAR_RATIO);
    }

    asset AssignmentProposal::calculateHusd(const asset &annualUsd, const int64_t &timeShare, const int64_t &deferred)
    {
        // calculate HUSD salary amount
        // 1. normalize annual salary to the time commitment of this proposal
        // 2. multiply (1) by 0.02026 to calculate a single moon phase; avg. phase is 7.4 days, 49.36 phases per year
        // 3. multiply (2) by 1 - deferral perc
        asset nonDeferredTimeShareAdjUsdPerPeriod = adjustAsset(calculateTimeShareUsdPerPeriod(annualUsd, timeShare), (float)1 - ((float)deferred / (float)100));

        // convert symbol from USD to HUSD
        return asset{nonDeferredTimeShareAdjUsdPerPeriod.amount, common::S_PEG};
    }

    asset AssignmentProposal::calculateHypha(const asset &annualUsd, const int64_t &timeShare, const int64_t &deferred)
    {
        TRACE_FUNCTION()
        // calculate HYPHA phase salary amount
        asset deferredTimeShareAdjUsdPerPeriod = adjustAsset(calculateTimeShareUsdPerPeriod(annualUsd, timeShare), (float)(float)deferred / (float)100);

        float hypha_deferral_coeff = (float)m_dao.getSettingOrFail<int64_t>(HYPHA_DEFERRAL_FACTOR) / (float)100;

        return adjustAsset(asset{deferredTimeShareAdjUsdPerPeriod.amount, common::S_REWARD}, hypha_deferral_coeff);
    }

    asset AssignmentProposal::calculateHvoice(const asset &annualUsd, const int64_t &timeShare)
    {
        return asset{calculateTimeShareUsdPerPeriod(annualUsd, timeShare).amount * 2, common::S_VOICE};
    }
} // namespace hypha