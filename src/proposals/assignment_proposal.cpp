
#include <document_graph/content_wrapper.hpp>
#include <document_graph/document.hpp>

#include <cmath>

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
        
        EOS_CHECK(
            Member::isMember(m_dao.get_self(), m_daoHash, assignee), 
            "only members can be assigned to assignments " + assignee.to_string()
        );

        Document roleDocument(m_dao.get_self(), assignment.getOrFail(DETAILS, ROLE_STRING)->getAs<eosio::checksum256>());

        auto role = roleDocument.getContentWrapper();

        // role in the proposal must be of type: role
        EOS_CHECK(role.getOrFail(SYSTEM, TYPE)->getAs<eosio::name>() == common::ROLE_NAME,
                  "role document hash provided in assignment proposal is not of type: role");

        EOS_CHECK(
            m_daoHash == Edge::get(m_dao.get_self(), roleDocument.getHash(), common::DAO).getToNode(),
            util::to_str("Role must belong to: ", m_daoHash)
        )

        EOS_CHECK(
            role.getOrFail(DETAILS, common::STATE)->getAs<string>() == common::STATE_APPROVED,
            util::to_str("Role must be approved before applying to it.")
        )

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

            //TODO: Store the dao in the period document and validate it 
            // verifies the period as valid
            Period period(&m_dao, std::get<eosio::checksum256>(startPeriod->value));
        } else {
            // default START_PERIOD to next period
            ContentWrapper::insertOrReplace(*detailsGroup, Content{START_PERIOD, Period::current(&m_dao, m_daoHash).next().getHash()});
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
        Content usdSalaryPerPeriod(USD_SALARY_PER_PERIOD, adjustAsset(annual_usd_salary, getPhaseToYearRatio(m_daoSettings)));
        ContentWrapper::insertOrReplace(*detailsGroup, usdSalaryPerPeriod);

        //TODO: Normalize all the tokens to allow different precisions on each token
        //currently they all must be the same or the will not work correctly

        auto rewardPegVal = m_daoSettings->getOrFail<eosio::asset>(common::REWARD_TO_PEG_RATIO);

        SalaryConfig salaryConf {
            .periodSalary = normalizeToken(usdSalaryPerPeriod.getAs<asset>()) * (timeShare / 100.0),
            .rewardToPegRatio = normalizeToken(rewardPegVal),
            .deferredPerc = deferred / 100.0,
            .voiceMultipler = 2.0
        };

        AssetBatch salaries = calculateSalaries(salaryConf, AssetBatch{
            .reward = m_daoSettings->getOrFail<asset>(common::REWARD_TOKEN),
            .peg = m_daoSettings->getOrFail<asset>(common::PEG_TOKEN),
            .voice = m_daoSettings->getOrFail<asset>(common::VOICE_TOKEN)
        });

        // add remaining derived per period salary amounts to this document        
        if (salaries.peg.amount > 0) {
            Content pegSalaryPerPeriod(common::PEG_SALARY_PER_PERIOD, salaries.peg);
            ContentWrapper::insertOrReplace(*detailsGroup, pegSalaryPerPeriod);
        }

        if (salaries.reward.amount > 0) {
            Content rewardSalaryPerPeriod(common::REWARD_SALARY_PER_PERIOD, salaries.reward);
            ContentWrapper::insertOrReplace(*detailsGroup, rewardSalaryPerPeriod);
        }

        if (salaries.voice.amount > 0) {
            Content voiceSalaryPerPeriod(common::VOICE_SALARY_PER_PERIOD, salaries.voice);
            ContentWrapper::insertOrReplace(*detailsGroup, voiceSalaryPerPeriod);
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

        auto assignmentToRoleEdge = m_dao.getGraph().getEdgesFrom(proposal.getHash(), common::ROLE_NAME);
      
        EOS_CHECK(
          !assignmentToRoleEdge.empty(),
          to_str("Missing 'role' edge from assignment: ", proposal.getHash())
        )

        Document role(m_dao.get_self(), assignmentToRoleEdge.at(0).getToNode());

        // update graph edges:
        //  member          ---- assigned           ---->   role_assignment
        //  role_assignment ---- assignee           ---->   member
        //  role_assignment ---- role               ---->   role                This one already exists since postProposeImpl
        //  role            ---- role_assignment    ---->   role_assignment
        Edge::write(m_dao.get_self(), m_dao.get_self(), assignee, proposal.getHash(), common::ASSIGNED);
        Edge::write(m_dao.get_self(), m_dao.get_self(), proposal.getHash(), assignee, common::ASSIGNEE_NAME);
        Edge::write(m_dao.get_self(), m_dao.get_self(), role.getHash(), proposal.getHash(), common::ASSIGNMENT);
        
        //Start period edge
        // assignment ---- start ----> period
        eosio::checksum256 startPeriodHash = contentWrapper.getOrFail(DETAILS, START_PERIOD)->getAs<eosio::checksum256>();
        Edge::write(m_dao.get_self(), m_dao.get_self(), proposal.getHash(), startPeriodHash, common::START);

        Period startPeriod(&m_dao, startPeriodHash);

        //Initial time share for proposal
        int64_t initTimeShare = contentWrapper.getOrFail(DETAILS, TIME_SHARE)->getAs<int64_t>();
        
        //Set starting date to first period start_date
        TimeShare initTimeShareDoc(m_dao.get_self(), 
                                   m_dao.get_self(), 
                                   initTimeShare, 
                                   startPeriod.getStartTime(),
                                   proposal.getHash());

        Edge::write(m_dao.get_self(), m_dao.get_self(), proposal.getHash(), initTimeShareDoc.getHash(), common::INIT_TIME_SHARE);
        Edge::write(m_dao.get_self(), m_dao.get_self(), proposal.getHash(), initTimeShareDoc.getHash(), common::CURRENT_TIME_SHARE);
        Edge::write(m_dao.get_self(), m_dao.get_self(), proposal.getHash(), initTimeShareDoc.getHash(), common::LAST_TIME_SHARE);

        auto [detailsIdx, details] = contentWrapper.getGroup(DETAILS);

        contentWrapper.insertOrReplace(*details, Content{
          common::APPROVED_DEFERRED, 
          contentWrapper.getOrFail(detailsIdx, DEFERRED).second->getAs<int64_t>()
        });
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
} // namespace hypha