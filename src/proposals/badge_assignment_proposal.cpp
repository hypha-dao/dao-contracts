
#include <eosio/name.hpp>
#include <eosio/crypto.hpp>

#include <proposals/badge_assignment_proposal.hpp>
#include <common.hpp>
#include <member.hpp>
#include <logger/logger.hpp>

namespace hypha
{

    void BadgeAssignmentProposal::proposeImpl(const name &proposer, ContentWrapper &badgeAssignment)
    {
        TRACE_FUNCTION()
        // assignee must exist and be a DHO member
        name assignee = badgeAssignment.getOrFail(DETAILS, ASSIGNEE)->getAs<eosio::name>();
        EOS_CHECK(Member::isMember(m_dao.get_self(), assignee), "only members can be earn badges " + assignee.to_string());

         // badge assignment proposal must link to a valid badge
        Document badgeDocument(m_dao.get_self(), badgeAssignment.getOrFail(DETAILS, BADGE_STRING)->getAs<eosio::checksum256>());
        auto badge = badgeDocument.getContentWrapper();
        EOS_CHECK(badge.getOrFail(SYSTEM, TYPE)->getAs<eosio::name>() == common::BADGE_NAME,
                     "badge document hash provided in assignment proposal is not of type badge");

        // START_PERIOD - number of periods the assignment is valid for
        auto detailsGroup = badgeAssignment.getGroupOrFail(DETAILS);
        if (auto [idx, startPeriod] = badgeAssignment.get(DETAILS, START_PERIOD); startPeriod)
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

    void BadgeAssignmentProposal::passImpl(Document &proposal)
    {
        TRACE_FUNCTION()
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
        TRACE_FUNCTION()
        return contentWrapper.getOrFail(DETAILS, TITLE)->getAs<std::string>();
    }

    name BadgeAssignmentProposal::getProposalType()
    {
        return common::ASSIGN_BADGE;
    }

} // namespace hypha