#include <proposals/proposal_factory.hpp>
#include <proposals/proposal.hpp>
#include <proposals/badge_proposal.hpp>
#include <proposals/badge_assignment_proposal.hpp>
#include <proposals/role_proposal.hpp>
#include <proposals/payout_proposal.hpp>
#include <proposals/assignment_proposal.hpp>
#include <proposals/attestation_proposal.hpp>
#include <proposals/edit_proposal.hpp>
#include <proposals/suspend_proposal.hpp>
#include <proposals/ass_extend_proposal.hpp>
#include <proposals/quest_start_proposal.hpp>
#include <proposals/quest_completion_proposal.hpp>
#include <proposals/policy_proposal.hpp>
#include <proposals/poll_proposal.hpp>
#include <proposals/circle_proposal.hpp>
#include <proposals/budget_proposal.hpp>
#include <logger/logger.hpp>

#include <common.hpp>

namespace hypha
{
    Proposal* ProposalFactory::Factory(dao& dao, uint64_t daoHash, const name &proposal_type)
    { 
        TRACE_FUNCTION()

        switch (proposal_type.value)
        {
        case common::BADGE_NAME.value:
            return new BadgeProposal(dao, daoHash);
        
        case common::ASSIGN_BADGE.value:
            return new BadgeAssignmentProposal(dao, daoHash);

        case common::ROLE_NAME.value:
            return new RoleProposal(dao, daoHash);
        
        case common::ASSIGNMENT.value:
            return new AssignmentProposal(dao, daoHash);

        case common::PAYOUT.value:
            return new PayoutProposal(dao, daoHash);

        case common::ATTESTATION.value:
            return new AttestationProposal(dao, daoHash);

        case common::SUSPEND.value: 
            return new SuspendProposal(dao, daoHash);

        case common::EDIT.value:
            return new EditProposal(dao, daoHash);

        case common::QUEST_START.value:
            return new QuestStartProposal(dao, daoHash);

        case common::QUEST_COMPLETION.value:
            return new QuestCompletionProposal(dao, daoHash);
            
        case common::POLICY.value:
            return new PolicyProposal(dao, daoHash);

        case common::POLL.value:
            return new PollProposal(dao, daoHash);

        case common::CIRCLE.value:
            return new CircleProposal(dao, daoHash);
        
        case common::BUDGET.value:
            return new BudgetProposal(dao, daoHash);
    
        // TODO: should be expanded to work with Badge Assignments as well
        case common::EXTENSION.value:
            return new AssignmentExtensionProposal(dao, daoHash);
        }

        EOS_CHECK(false, "Unknown proposal_type: " + proposal_type.to_string());
        return nullptr;
    }
} // namespace hypha
