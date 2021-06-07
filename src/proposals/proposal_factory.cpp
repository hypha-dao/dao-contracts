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
#include <logger/logger.hpp>

#include <common.hpp>

namespace hypha
{
    Proposal* ProposalFactory::Factory(dao& dao, const name &proposal_type)
    { 
        TRACE_FUNCTION()

        switch (proposal_type.value)
        {
        case common::BADGE_NAME.value:
            return new BadgeProposal(dao);
        
        case common::ASSIGN_BADGE.value:
            return new BadgeAssignmentProposal(dao);

        case common::ROLE_NAME.value:
            return new RoleProposal(dao);
        
        case common::ASSIGNMENT.value:
            return new AssignmentProposal(dao);

        case common::PAYOUT.value:
            return new PayoutProposal(dao);

        case common::ATTESTATION.value:
            return new AttestationProposal(dao);

        case common::SUSPEND.value: 
            return new SuspendProposal(dao);

        case common::EDIT.value:
            return new EditProposal(dao);
    
        // TODO: should be expanded to work with Badge Assignments as well
        case common::EXTENSION.value:
            return new AssignmentExtensionProposal(dao);
        }

        EOS_CHECK(false, "Unknown proposal_type: " + proposal_type.to_string());
        return nullptr;
    }
} // namespace hypha