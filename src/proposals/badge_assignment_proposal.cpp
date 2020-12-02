#include <proposals/badge_assignment_proposal.hpp>
#include <document_graph/content_group.hpp>
#include <common.hpp>

namespace hypha
{

    ContentGroups BadgeAssignmentProposal::propose_impl(const name &proposer, ContentGroups &contentGroups)
    {
        ContentWrapper badgeAssignment (contentGroups);

        // assignee must exist and be a DHO member
        name assignee = badgeAssignment.getContent(common::DETAILS, common::ASSIGNEE).getAs<eosio::name>();
        verify_membership(assignee);

        // TODO: Additional input cleansing
        // start_period and end_period must be valid, no more than X periods in between

        // badge assignment proposal must link to a valid badge
        Document badgeDocument (m_dao.get_self(), badgeAssignment.getContent(common::DETAILS, common::BADGE_STRING).getAs<eosio::checksum256>());
        ContentWrapper badge (badgeDocument.getContentGroups());

        // badge in the proposal must be of type: badge
        eosio::check (badge.getContent(common::SYSTEM, common::TYPE).getAs<eosio::name>() == common::BADGE_NAME, 
            "badge document hash provided in assignment proposal is not of type badge");
 
        return contentGroups;
    }

    Document BadgeAssignmentProposal::pass_impl(Document proposal)
    {
        // need to create edges here
        // TODO: create edges
        return proposal;
    }

    string BadgeAssignmentProposal::GetBallotContent(ContentGroups contentGroups)
    {
        return ContentWrapper::getContent (contentGroups, common::DETAILS, common::TITLE).getAs<std::string>();
    }

    name BadgeAssignmentProposal::GetProposalType()
    {
        return common::ASSIGN_BADGE;
    }

} // namespace hypha