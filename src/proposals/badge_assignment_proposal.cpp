
#include <eosio/name.hpp>
#include <eosio/crypto.hpp>

#include <proposals/badge_assignment_proposal.hpp>
#include <common.hpp>
#include <member.hpp>

namespace hypha
{

    void BadgeAssignmentProposal::propose_impl(const name &proposer, ContentWrapper &badgeAssignment)
    {
        // assignee must exist and be a DHO member
        name assignee = badgeAssignment.getOrFail(common::DETAILS, common::ASSIGNEE)->getAs<eosio::name>();
        eosio::check(Member::isMember(m_dao.get_self(), assignee), "only members can be earn badges " + assignee.to_string());

        // TODO: Additional input cleansing
        // start_period and end_period must be valid, no more than X periods in between

        // badge assignment proposal must link to a valid badge
        Document badgeDocument(m_dao.get_self(), badgeAssignment.getOrFail(common::DETAILS, common::BADGE_STRING)->getAs<eosio::checksum256>());
        auto badge = badgeDocument.getContentWrapper();

        // badge in the proposal must be of type: badge
        eosio::check(badge.getOrFail(common::SYSTEM, common::TYPE)->getAs<eosio::name>() == common::BADGE_NAME,
                     "badge document hash provided in assignment proposal is not of type badge");
    }

    void BadgeAssignmentProposal::pass_impl(Document &proposal)
    {
        ContentWrapper contentWrapper = proposal.getContentWrapper();

        eosio::checksum256 assignee = Member::getHash((contentWrapper.getOrFail(common::DETAILS, common::ASSIGNEE)->getAs<eosio::name>()));
        Document badge(m_dao.get_self(), contentWrapper.getOrFail(common::DETAILS, common::BADGE_STRING)->getAs<eosio::checksum256>());

        // update graph edges:
        //    member            ---- holdsbadge     ---->   badge
        //    member            ---- badgeassign    ---->   badge_assignment
        //    badge             ---- heldby         ---->   member
        //    badge             ---- assignment     ---->   badge_assignment
        //    badge_assignment  ---- badge          ---->   badge

        // the assignee now HOLDS this badge, non-strict in case the member already has the badge
        Edge::write(m_dao.get_self(), m_dao.get_self(), assignee, badge.getHash(), common::HOLDS_BADGE);
        Edge::write(m_dao.get_self(), m_dao.get_self(), assignee, proposal.getHash(), common::ASSIGN_BADGE);
        Edge::write(m_dao.get_self(), m_dao.get_self(), badge.getHash(), assignee, common::HELD_BY);
        Edge::write(m_dao.get_self(), m_dao.get_self(), badge.getHash(), proposal.getHash(), common::ASSIGNMENT);
        Edge::write(m_dao.get_self(), m_dao.get_self(), proposal.getHash(), badge.getHash(), common::BADGE_NAME);
    }

    std::string BadgeAssignmentProposal::GetBallotContent(ContentWrapper &contentWrapper)
    {
        return contentWrapper.getOrFail(common::DETAILS, common::TITLE)->getAs<std::string>();
    }

    name BadgeAssignmentProposal::GetProposalType()
    {
        return common::ASSIGN_BADGE;
    }

} // namespace hypha