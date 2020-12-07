
#include <document_graph/content_wrapper.hpp>
#include <document_graph/document.hpp>

#include <proposals/assignment_proposal.hpp>
#include <member.hpp>
#include <common.hpp>
#include <dao.hpp>
#include <util.hpp>

namespace hypha
{

    void AssignmentProposal::propose_impl(const name &proposer, ContentWrapper &assignment)
    {
        // assignee must exist and be a DHO member
        name assignee = assignment.getOrFail(common::DETAILS, common::ASSIGNEE)->getAs<eosio::name>();
        eosio::check(Member::isMember(m_dao.get_self(), assignee), "only members can be assigned to assignments " + assignee.to_string());

        // TODO: Additional input cleansing
        // start_period and end_period must be valid, no more than X periods in between

        Document roleDocument(m_dao.get_self(), assignment.getOrFail(common::DETAILS, common::ROLE_STRING)->getAs<eosio::checksum256>());
        auto role = roleDocument.getContentWrapper();

        // role in the proposal must be of type: role
        eosio::check (role.getOrFail(common::SYSTEM, common::TYPE)->getAs<eosio::name>() == common::ROLE_NAME, 
            "role document hash provided in assignment proposal is not of type: role");

        // time_share_x100 is required and must be greater than zero and less than 100%
        int64_t timeShare = assignment.getOrFail(common::DETAILS, common::TIME_SHARE)->getAs<int64_t>();
        eosio::check(timeShare > 0, common::TIME_SHARE + " must be greater than zero. You submitted: " + std::to_string(timeShare));
        eosio::check(timeShare <= 10000, common::TIME_SHARE + " must be less than or equal to 10000 (=100%). You submitted: " + std::to_string(timeShare));

        // retrieve the minimum time_share from the role, if it exists, and check the assignment against it
        if (auto [idx, minTimeShare] = role.get(common::DETAILS, common::MIN_TIME_SHARE); minTimeShare) 
        {
            eosio::check(timeShare >= minTimeShare->getAs<int64_t>(), 
                common::TIME_SHARE + " must be greater than or equal to the role configuration. Role value for " + 
                common::MIN_TIME_SHARE + " is " + std::to_string(minTimeShare->getAs<int64_t>()) + 
                ", and you submitted: " + std::to_string(timeShare));
        }

        // deferred_x100 is required and must be greater than or equal to zero and less than or equal to 10000
        int64_t deferred = assignment.getOrFail(common::DETAILS, common::DEFERRED)->getAs<int64_t>();
        eosio::check(deferred >= 0, common::DEFERRED + " must be greater than or equal to zero. You submitted: " + std::to_string(deferred));
        eosio::check(deferred <= 10000, common::DEFERRED + " must be less than or equal to 10000 (=100%). You submitted: " + std::to_string(deferred));

        // retrieve the minimum deferred from the role, if it exists, and check the assignment against it
        if (auto [idx, minDeferred] = role.get(common::DETAILS, common::MIN_DEFERRED); minDeferred)         
        {
            eosio::check(deferred >= minDeferred->getAs<int64_t>(), 
                common::DEFERRED + " must be greater than or equal to the role configuration. Role value for " + 
                common::MIN_DEFERRED + " is " + std::to_string(minDeferred->getAs<int64_t>()) + ", and you submitted: " + std::to_string(deferred));
        }

        // start_period and end_period are required and must be greater than or equal to zero, and end_period >= start_period
        int64_t start_period = assignment.getOrFail(common::DETAILS, common::START_PERIOD)->getAs<int64_t>();
        eosio::check(start_period >= 0, common::START_PERIOD + " must be greater than or equal to zero. You submitted: " + std::to_string(start_period));
        int64_t end_period = assignment.getOrFail(common::DETAILS, common::END_PERIOD)->getAs<int64_t>();
        eosio::check(end_period >= 0, common::END_PERIOD + " must be greater than or equal to zero. You submitted: " + std::to_string(end_period));
        eosio::check(end_period >= start_period, common::END_PERIOD + " must be greater than or equal to " + common::START_PERIOD +
                                              ". You submitted: " + common::START_PERIOD + ": " + std::to_string(start_period) +
                                              " and " + common::END_PERIOD + ": " + std::to_string(end_period));

        asset annual_usd_salary = role.getOrFail(common::DETAILS, common::ANNUAL_USD_SALARY)->getAs<eosio::asset>();

        //**************************
        // we must add calculations into the contentGroups for this assignment proposal
        // need to implement the pass by reference logic
        //**************************

        // add the USD period pay amount (this is used to calculate SEEDS at time of salary claim)
        Content usdSalaryPerPeriod (common::USD_SALARY_PER_PERIOD, adjustAsset(annual_usd_salary, common::PHASE_TO_YEAR_RATIO));
        
        // add remaining derived per period salary amounts to this document
        Content husdSalaryPerPeriod (common::HUSD_SALARY_PER_PERIOD, calculateHusd(annual_usd_salary, timeShare, deferred));
        Content hyphaSalaryPerPeriod (common::HYPHA_SALARY_PER_PERIOD, calculateHypha(annual_usd_salary, timeShare, deferred));
        Content hvoiceSalaryPerPeriod (common::HVOICE_SALARY_PER_PERIOD, calculateHvoice(annual_usd_salary, timeShare));

        auto detailsGroup = assignment.getGroupOrFail(common::DETAILS);
        ContentWrapper::insertOrReplace(*detailsGroup, usdSalaryPerPeriod);
        ContentWrapper::insertOrReplace(*detailsGroup, husdSalaryPerPeriod);
        ContentWrapper::insertOrReplace(*detailsGroup, hyphaSalaryPerPeriod);
        ContentWrapper::insertOrReplace(*detailsGroup, hvoiceSalaryPerPeriod);
    }

    void AssignmentProposal::pass_impl(Document &proposal)
    {
        ContentWrapper contentWrapper = proposal.getContentWrapper();

        eosio::checksum256 assignee = Member::getHash(contentWrapper.getOrFail(common::DETAILS, common::ASSIGNEE)->getAs<eosio::name>());
        Document role(m_dao.get_self(), contentWrapper.getOrFail(common::DETAILS, common::ROLE_STRING)->getAs<eosio::checksum256>());

        // update graph edges:
        //  member          ---- assigned           ---->   role_assignment
        //  role_assignment ---- assignee           ---->   member
        //  role_assignment ---- role               ---->   role
        //  role            ---- role_assignment    ---->   role_assignment
        Edge::write (m_dao.get_self(), m_dao.get_self(), assignee, proposal.getHash(), common::ASSIGNED);
        Edge::write (m_dao.get_self(), m_dao.get_self(), proposal.getHash(), assignee, common::ASSIGNEE_NAME);
        Edge::write (m_dao.get_self(), m_dao.get_self(), proposal.getHash(), role.getHash(), common::ROLE_NAME);
        Edge::write (m_dao.get_self(), m_dao.get_self(), role.getHash(), proposal.getHash(), common::ASSIGNMENT);

        // TODO: what about periods?
    }

    std::string AssignmentProposal::GetBallotContent (ContentWrapper &contentWrapper)
    {
        return contentWrapper.getOrFail(common::DETAILS, common::TITLE)->getAs<std::string>();
    }

    name AssignmentProposal::GetProposalType()
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
        return asset{nonDeferredTimeShareAdjUsdPerPeriod.amount, common::S_HUSD};
    }

    asset AssignmentProposal::calculateHypha(const asset &annualUsd, const int64_t &timeShare, const int64_t &deferred)
    {
        // calculate HYPHA phase salary amount
        asset deferredTimeShareAdjUsdPerPeriod = adjustAsset(calculateTimeShareUsdPerPeriod(annualUsd, timeShare), (float)(float)deferred / (float)100);

        float hypha_deferral_coeff = (float) m_dao.getSettingOrFail<int64_t>(common::HYPHA_DEFERRAL_FACTOR) / (float) 100;

        return adjustAsset(asset{deferredTimeShareAdjUsdPerPeriod.amount, common::S_HYPHA}, hypha_deferral_coeff);
    }

    asset AssignmentProposal::calculateHvoice(const asset &annualUsd, const int64_t &timeShare)
    {
        return asset{calculateTimeShareUsdPerPeriod(annualUsd, timeShare).amount * 2, common::S_HVOICE};
    }
} // namespace hypha