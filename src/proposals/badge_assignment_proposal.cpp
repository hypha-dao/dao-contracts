
#include <eosio/name.hpp>
#include <eosio/crypto.hpp>

#include <proposals/badge_assignment_proposal.hpp>
#include <common.hpp>
#include <member.hpp>
#include <logger/logger.hpp>

#include <badges/badges.hpp>

#include <recurring_activity.hpp>

namespace hypha
{

    void BadgeAssignmentProposal::proposeImpl(const name &proposer, ContentWrapper &badgeAssignment)
    {
        TRACE_FUNCTION()
        // assignee must exist and be a DHO member
        name assignee = badgeAssignment.getOrFail(DETAILS, ASSIGNEE)->getAs<eosio::name>();

        EOS_CHECK(
            Member::isMember(m_dao, m_daoID, assignee), 
            "only members can be assigned to badges " + assignee.to_string()
        );

         // badge assignment proposal must link to a valid badge
        Document badgeDocument(m_dao.get_self(), badgeAssignment.getOrFail(DETAILS, BADGE_STRING)->getAs<int64_t>());
        
        auto badge = badgeDocument.getContentWrapper();

        EOS_CHECK(badge.getOrFail(SYSTEM, TYPE)->getAs<eosio::name>() == common::BADGE_NAME,
                     "badge document hash provided in assignment proposal is not of type badge");

        //Verify DAO has access to this badge
        Edge::get(m_dao.get_self(), m_daoID, badgeDocument.getID(), common::BADGE_NAME);
        // EOS_CHECK(
        //     m_daoID == Edge::get(m_dao.get_self(), badgeDocument.getID (), common::DAO).getToNode(),
        //     to_str("Badge must belong to: ", m_daoID)
        // )

        EOS_CHECK(
            badge.getOrFail(DETAILS, common::STATE)->getAs<string>() == common::STATE_APPROVED,
            to_str("Badge must be approved before applying to it.")
        )

        // START_PERIOD - number of periods the assignment is valid for
        auto detailsGroup = badgeAssignment.getGroupOrFail(DETAILS);
        if (auto [idx, startPeriod] = badgeAssignment.get(DETAILS, START_PERIOD); startPeriod)
        {
            Period period(&m_dao, std::get<int64_t>(startPeriod->value));
        } else {
            // default START_PERIOD to next period
            ContentWrapper::insertOrReplace(
                *detailsGroup, 
                Content{
                    START_PERIOD, 
                    static_cast<int64_t>(Period::current(&m_dao, m_daoID).next().getID())
                }
            );
        }

        // PERIOD_COUNT - number of periods the assignment is valid for
        if (auto [idx, periodCount] = badgeAssignment.get(DETAILS, PERIOD_COUNT); periodCount)
        {
            EOS_CHECK(std::holds_alternative<int64_t>(periodCount->value),
                         "fatal error: expected to be an int64 type: " + periodCount->label);

            EOS_CHECK(std::get<int64_t>(periodCount->value) < 26, PERIOD_COUNT + 
                string(" must be less than 26. You submitted: ") + std::to_string(std::get<int64_t>(periodCount->value)));

        } else {
            // default PERIOD_COUNT to 13
            ContentWrapper::insertOrReplace(*detailsGroup, Content{PERIOD_COUNT, 13});
        }
    }

    void BadgeAssignmentProposal::postProposeImpl(Document &proposal) 
    {
        ContentWrapper contentWrapper = proposal.getContentWrapper();

        //    badge_assignment  ---- badge          ---->   badge
        Document badge(m_dao.get_self(), contentWrapper.getOrFail(DETAILS, BADGE_STRING)->getAs<int64_t>());
        Edge::write(m_dao.get_self(), m_dao.get_self(), proposal.getID (), badge.getID (), common::BADGE_NAME);

        //    badge_assignment  ---- start          ---->   period
        Document startPer(m_dao.get_self(), contentWrapper.getOrFail(DETAILS, START_PERIOD)->getAs<int64_t>());
        Edge::write(m_dao.get_self(), m_dao.get_self(), proposal.getID (), startPer.getID(), common::START);
    }

    void BadgeAssignmentProposal::passImpl(Document &proposal)
    {
        TRACE_FUNCTION()
        ContentWrapper contentWrapper = proposal.getContentWrapper();

        eosio::name assignee = contentWrapper.getOrFail(DETAILS, ASSIGNEE)->getAs<eosio::name>();
        Document assigneeDoc(m_dao.get_self(), m_dao.getMemberID(assignee));
        Document badge(m_dao.get_self(), contentWrapper.getOrFail(DETAILS, BADGE_STRING)->getAs<int64_t>());

        // update graph edges:
        //    member            ---- holdsbadge     ---->   badge
        //    badge             ---- heldby         ---->   member
        //    member            ---- assignbadge    ---->   badge_assignment
        //    badge             ---- assignment     ---->   badge_assignment

        // the assignee now HOLDS this badge, non-strict in case the member already has the badge
        //We use getOrNew since the user could have held this badge already
        Edge::getOrNew(m_dao.get_self(), m_dao.get_self(), assigneeDoc.getID(), badge.getID (), common::HOLDS_BADGE);
        Edge::getOrNew(m_dao.get_self(), m_dao.get_self(), badge.getID (), assigneeDoc.getID(), common::HELD_BY);
        Edge::write(m_dao.get_self(), m_dao.get_self(), assigneeDoc.getID(), proposal.getID (), common::ASSIGN_BADGE);
        Edge::write(m_dao.get_self(), m_dao.get_self(), badge.getID (), proposal.getID (), common::ASSIGNMENT);

        eosio::action(
            eosio::permission_level(m_dao.get_self(), "active"_n),
            m_dao.get_self(),
            "activatebdg"_n,
            std::make_tuple(proposal.getID())
        ).send();
    }

    void BadgeAssignmentProposal::publishImpl(Document& proposal)
    {
        TRACE_FUNCTION()

        //Check if assignee doesn't hold a system badge already
        auto badge = badges::getBadgeOf(m_dao, proposal.getID());

        name assignee = proposal.getContentWrapper()
                                .getOrFail(DETAILS, ASSIGNEE)
                                ->getAs<eosio::name>();
        
        badges::checkHoldsBadge(m_dao, badge, m_daoID, m_dao.getMemberID(assignee));
    }

    std::string BadgeAssignmentProposal::getBallotContent(ContentWrapper &contentWrapper)
    {
        TRACE_FUNCTION()
        return contentWrapper.getOrFail(DETAILS, TITLE)->getAs<std::string>();
    }

    name BadgeAssignmentProposal::getProposalType()
    {
        return common::ASSIGN_BADGE;
    }

} // namespace hypha