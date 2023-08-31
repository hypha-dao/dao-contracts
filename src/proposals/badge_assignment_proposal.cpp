
#include <eosio/name.hpp>
#include <eosio/crypto.hpp>

#include <proposals/badge_assignment_proposal.hpp>
#include <common.hpp>
#include <member.hpp>
#include <logger/logger.hpp>

#include <badges/badges.hpp>

#include <recurring_activity.hpp>

#include <upvote_election/upvote_election.hpp>
#include <upvote_election/common.hpp>

namespace hypha
{

    Document BadgeAssignmentProposal::getBadgeDoc(ContentGroups& cgs) const
    {
        auto cw = ContentWrapper(cgs);
        auto badgeId = cw.getOrFail(DETAILS, BADGE_STRING)->getAs<int64_t>();
        return Document(m_dao.get_self(), badgeId);
    }

    bool BadgeAssignmentProposal::checkMembership(const eosio::name& proposer, ContentGroups &contentGroups)
    {
        // TODO: Check scope item in badge document, will be used to define what type of user 
        // can apply for the badge i.e. 'CORE', 'COMMUNITY' & 'CORE_AND_COMMUNITY'
        if (Proposal::checkMembership(proposer, contentGroups)) {
            return true;
        }

        auto badge = getBadgeDoc(contentGroups);
        auto badgeInfo = badges::getBadgeInfo(badge);

        if (badgeInfo.type == badges::BadgeType::System && 
            (badgeInfo.systemType == badges::SystemBadgeType::Voter || 
             badgeInfo.systemType == badges::SystemBadgeType::Delegate)) {
            return Member::isCommunityMember(m_dao, m_daoID, proposer);
        }

        return false;
    }

    void BadgeAssignmentProposal::proposeImpl(const name &proposer, ContentWrapper &badgeAssignment)
    {
        TRACE_FUNCTION()
        // assignee must exist and be a DHO member
        name assignee = badgeAssignment.getOrFail(DETAILS, ASSIGNEE)->getAs<eosio::name>();

         // badge assignment proposal must link to a valid badge
        Document badgeDocument = getBadgeDoc(badgeAssignment.getContentGroups());
        
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

        //Check for custom system badges
        auto badgeInfo = badges::getBadgeInfo(badgeDocument);

        auto detailsGroup = badgeAssignment.getGroupOrFail(DETAILS);

        //Check for auto-approved badges (Only valid for Admin and Enroller)
        if (auto [_, isAutoApprove] = badgeAssignment.get(DETAILS, common::AUTO_APPROVE); 
            isAutoApprove && isAutoApprove->getAs<int64_t>() == 1) {
            //Only admins of the DAO are allowed to create this type of proposals
            //m_dao.checkAdminsAuth(m_daoID);
            EOS_CHECK(
                proposer == m_dao.get_self(),
                "Only contract is allowed to perform this action"
            );

            EOS_CHECK(
                badgeInfo.systemType == badges::SystemBadgeType::Admin ||
                badgeInfo.systemType == badges::SystemBadgeType::Enroller,
                "This type of badge cannot be self approved"
            );

            selfApprove = true;
            return;
        }

        //Voter badge and Delegate badge has to be auto approved
        if (badges::isSelfApproveBadge(badgeInfo.systemType)) {
            
            selfApprove = true;
            //TODO: Should we check for basic types (?)
            time_point start = eosio::current_time_point();
            time_point end;

            if (badgeInfo.systemType == badges::SystemBadgeType::Voter) {
                //Setup 1 year duration
                const auto year = eosio::days(365);
                end = start + year;
            }
            else {
                if (badgeInfo.systemType == badges::SystemBadgeType::HeadDelegate ||
                    badgeInfo.systemType == badges::SystemBadgeType::ChiefDelegate) {
                    //We have to specify the election id
                    auto election = badgeAssignment.getOrFail(DETAILS, badges::common::items::UPVOTE_ELECTION_ID)->getAs<int64_t>();

                    int64_t duration = 0;
                    
                    //0 value 45means that the upvote election was imported
                    if (election) {
                        upvote_election::UpvoteElection upvoteElection(m_dao, election);
                        //Set start as when the election is finished
                        start = upvoteElection.getEndDate();
                        duration = upvoteElection.getDuration();
                    }
                    else {
                        duration = m_daoSettings->getOrFail<int64_t>(upvote_election::common::items::UPVOTE_DURATION);
                    }

                    EOS_CHECK(
                        duration > 0,
                        "Delegate Badge duration must be a positive amount"
                    );

                    //Proposer must be contract
                    EOS_CHECK(
                        proposer == m_dao.get_self(),
                        "Only contract is allowed to perform this action"
                    );

                    end = start + eosio::seconds(duration);
                }
                //Delegate type
                else {
                    //Fail if there is no upcoming election
                    auto upvoteElection = upvote_election::UpvoteElection::getUpcomingElection(m_dao, m_daoID);

                    EOS_CHECK(
                        upvoteElection.getStatus() == upvote_election::common::upvote_status::UPCOMING,
                        "You can only apply to Delegate badges when there is an upcomming election"
                    )
                    
                    //For now we will set start date to current time
                    //otherwise delegate edge won't be created until election starts
                    //but we need it before to showcase in UI how many candidates
                    //have already applied
                    end = upvoteElection.getEndDate();
                }
            }
            
            EOS_CHECK(
                start.elapsed.count() && end.elapsed.count() &&
                start < end,
                to_str("Invalid start and end dates: ", start, " ", end)
            );
            
            ContentWrapper::insertOrReplace(
                *detailsGroup, 
                Content{
                    START_TIME, 
                    start
                }
            );

            ContentWrapper::insertOrReplace(
                *detailsGroup, 
                Content{
                    END_TIME, 
                    end
                }
            );

            //Since the follwoing items are just required for other smart/custom badges
            //we should return, otherwise we would error out as those items are not
            //provided for upvote badges.
            return;
        }

        // START_PERIOD - number of periods the assignment is valid for
        if (auto [idx, startPeriod] = badgeAssignment.get(DETAILS, START_PERIOD); startPeriod) {
            Document period = TypedDocument::withType(
                m_dao,
                static_cast<uint64_t>(startPeriod->getAs<int64_t>()),
                common::PERIOD
            );
            
            auto calendarId = Edge::get(m_dao.get_self(), period.getID(), common::CALENDAR).getToNode();

            //TODO Period: Remove since period refactor will no longer point to DAO
            EOS_CHECK(
                Edge::exists(m_dao.get_self(), m_daoID, calendarId, common::CALENDAR),
                "Period must belong to the DAO"
            );
        } 
        else {
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
        Document badge = getBadgeDoc(proposal.getContentGroups());
        Edge::write(m_dao.get_self(), m_dao.get_self(), proposal.getID (), badge.getID (), common::BADGE_NAME);

        //For this specific badges we don't setup a start period
        if (selfApprove) {
            return;
        }

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
        Document badge = getBadgeDoc(proposal.getContentGroups());

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
            eosio::permission_level(m_dao.get_self(), eosio::name("active")),
            m_dao.get_self(),
            eosio::name("activatebdg"),
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