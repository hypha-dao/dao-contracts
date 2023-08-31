#include <proposals/quest_start_proposal.hpp>
#include <common.hpp>
#include <util.hpp>
#include <member.hpp>
#include <typed_document.hpp>

namespace hypha
{
    void QuestStartProposal::proposeImpl(const name &proposer, ContentWrapper &contentWrapper)
    {
        // PERIOD_COUNT - number of periods the quest is valid for
        auto periodCount = contentWrapper.getOrFail(DETAILS, PERIOD_COUNT);

        EOS_CHECK(
            periodCount->getAs<int64_t>() < 26, 
            PERIOD_COUNT + string(" must be less than 26. You submitted: ") + std::to_string(std::get<int64_t>(periodCount->value))
        );

        //Check token rewards for the quest and recipient item
        PayoutProposal::proposeImpl(proposer, contentWrapper);
    }

    void QuestStartProposal::postProposeImpl(Document &proposal)
    {
        auto cw = proposal.getContentWrapper();

        //We might have stablished a previous quest start
        if (auto parentQuest = getItemDocOpt(common::PARENT_QUEST_ITEM, common::QUEST_START, cw)) {
            //Create parent quest edge
            Edge(m_dao.get_self(), m_dao.get_self(), proposal.getID(), parentQuest->getID(), common::PARENT_QUEST);

            //Note: We might want to also check if parent quest is already linked to another quest so we don't 
            //allow multiple children and limit it to just 1
            markRelatives(common::PARENT_QUEST, common::ASCENDANT, name(), proposal.getID());
        }

        if (auto circle = getItemDocOpt(common::CIRCLE_ID, common::CIRCLE, cw)) {
            //Create circle link
            Edge(m_dao.get_self(), m_dao.get_self(), proposal.getID(), circle->getID(), common::CIRCLE);
        }

        //Check start period
        Document period = getItemDoc(START_PERIOD, common::PERIOD, cw);

        auto calendarId = Edge::get(m_dao.get_self(), period.getID(), common::CALENDAR).getToNode();

        //TODO Period: Remove since period refactor will no longer point to DAO
        EOS_CHECK(
            Edge::exists(m_dao.get_self(), m_daoID, calendarId, common::CALENDAR),
            "Period must belong to the DAO"
        );

        //TODO: Verify period is in the future (?)
        Edge(m_dao.get_self(), m_dao.get_self(), proposal.getID(), period.getID(), common::START);
    }

    void QuestStartProposal::passImpl(Document &proposal)
    {        
        ContentWrapper contentWrapper = proposal.getContentWrapper();
        Member assignee = Member(m_dao, m_dao.getMemberID(contentWrapper.getOrFail(DETAILS, RECIPIENT)->getAs<eosio::name>()));
        
        Edge::write(m_dao.get_self(), m_dao.get_self(), assignee.getID(), proposal.getID(), common::ENTRUSTED_TO);

        markRelatives(common::PARENT_QUEST, name(), common::DESCENDANT, proposal.getID());
    }

    void QuestStartProposal::failImpl(Document &proposal)
    {
        //Remove relationships
        //markRelatives(common::PARENT_QUEST, common::ASCENDANT, common::DESCENDANT, proposal.getID(), true);
    }

    name QuestStartProposal::getProposalType()
    {
        return common::QUEST_START;
    }
}
