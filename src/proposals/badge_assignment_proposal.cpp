
#include <eosio/name.hpp>
#include <eosio/crypto.hpp>

#include <proposals/badge_assignment_proposal.hpp>
#include <common.hpp>
#include <member.hpp>

namespace hypha
{

    void BadgeAssignmentProposal::proposeImpl(const name &proposer, ContentWrapper &badgeAssignment)
    {
        // assignee must exist and be a DHO member
        name assignee = badgeAssignment.getOrFail(DETAILS, ASSIGNEE)->getAs<eosio::name>();
        eosio::check(Member::isMember(m_dao.get_self(), assignee), "only members can be earn badges " + assignee.to_string());

        Document startPeriod(m_dao.get_self(), badgeAssignment.getOrFail(DETAILS, START_PERIOD)->getAs<eosio::checksum256>());
        int64_t numPeriods = badgeAssignment.getOrFail(DETAILS, PERIOD_COUNT)->getAs<int64_t>();
        eosio::check(numPeriods < 26, PERIOD_COUNT + string(" must be less than 26. You submitted: ") + std::to_string(numPeriods));

        // badge assignment proposal must link to a valid badge
        Document badgeDocument(m_dao.get_self(), badgeAssignment.getOrFail(DETAILS, BADGE_STRING)->getAs<eosio::checksum256>());
        auto badge = badgeDocument.getContentWrapper();

        // badge in the proposal must be of type: badge
        eosio::check(badge.getOrFail(SYSTEM, TYPE)->getAs<eosio::name>() == common::BADGE_NAME,
                     "badge document hash provided in assignment proposal is not of type badge");
    }

    void BadgeAssignmentProposal::passImpl(Document &proposal)
    {
        ContentWrapper contentWrapper = proposal.getContentWrapper();

        eosio::checksum256 assignee = Member::calcHash((contentWrapper.getOrFail(DETAILS, ASSIGNEE)->getAs<eosio::name>()));
        Document badge(m_dao.get_self(), contentWrapper.getOrFail(DETAILS, BADGE_STRING)->getAs<eosio::checksum256>());

        // update graph edges:
        //    member            ---- holdsbadge     ---->   badge
        //    member            ---- badgeassign    ---->   badge_assignment
        //    badge             ---- heldby         ---->   member
        //    badge             ---- assignment     ---->   badge_assignment
        //    badge_assignment  ---- badge          ---->   badge
        //    badge_assignment  ---- start          ---->   period

        // the assignee now HOLDS this badge, non-strict in case the member already has the badge
        Edge::write(m_dao.get_self(), m_dao.get_self(), assignee, badge.getHash(), common::HOLDS_BADGE);
        Edge::write(m_dao.get_self(), m_dao.get_self(), assignee, proposal.getHash(), common::ASSIGN_BADGE);
        Edge::write(m_dao.get_self(), m_dao.get_self(), badge.getHash(), assignee, common::HELD_BY);
        Edge::write(m_dao.get_self(), m_dao.get_self(), badge.getHash(), proposal.getHash(), common::ASSIGNMENT);
        Edge::write(m_dao.get_self(), m_dao.get_self(), proposal.getHash(), badge.getHash(), common::BADGE_NAME);

        Edge::write(m_dao.get_self(), m_dao.get_self(), proposal.getHash(),
                    contentWrapper.getOrFail(DETAILS, START_PERIOD)->getAs<eosio::checksum256>(), common::START);
    }

    std::string BadgeAssignmentProposal::getBallotContent(ContentWrapper &contentWrapper)
    {
        return contentWrapper.getOrFail(DETAILS, TITLE)->getAs<std::string>();
    }

    name BadgeAssignmentProposal::getProposalType()
    {
        return common::ASSIGN_BADGE;
    }

} // namespace hypha