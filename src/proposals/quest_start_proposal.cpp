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

        //TODO: Add required parameters check on proposeImpl
        //Also we should specify the quest rewards here
        PayoutProposal::proposeImpl(proposer, contentWrapper);
    }

    void QuestStartProposal::postProposeImpl(Document &proposal)
    {
        auto cw = proposal.getContentWrapper();

        //We might have stablished a previous quest start
        if (auto parentQuest = getParent(common::PARENT_QUEST_ITEM, common::QUEST_START, cw)) {

            auto parentCW = parentQuest->getContentWrapper();
            //Only approved quest can be used as parent quest
            EOS_CHECK(
                parentCW.getOrFail(DETAILS, common::STATE)
                        ->getAs<string>() == common::STATE_APPROVED,
                "Only approved quest can be used as parent quest"
            );

            //Create parent quest edge
            Edge(m_dao.get_self(), m_dao.get_self(), proposal.getID(), parentQuest->getID(), common::PARENT_QUEST);

            markRelatives(common::PARENT_QUEST, common::ASCENDANT, common::DESCENDANT, proposal.getID());
        }

         //TODO: Check circle field
        if (auto [_, circleIdItem] = cw.get(DETAILS, "circle_id"); circleIdItem) {
            auto circleId = circleIdItem->getAs<int64_t>();

            //Validate circle 
            TypedDocument::withType(m_dao, circleId, common::CIRCLE);

            //Create circle link
            Edge(m_dao.get_self(), m_dao.get_self(), proposal.getID(), circleId, common::CIRCLE);
        }

        //Check start period
        auto startPeriod = cw.getOrFail(DETAILS, START_PERIOD);

        Document period = TypedDocument::withType(
            m_dao,
            static_cast<uint64_t>(startPeriod->getAs<int64_t>()),
            common::PERIOD
        );

        EOS_CHECK(
            Edge::exists(m_dao.get_self(), period.getID(), m_daoID, common::DAO),
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
    }

    void QuestStartProposal::failImpl(Document &proposal)
    {
        //Remove relationships
        markRelatives(common::PARENT_QUEST, common::ASCENDANT, common::DESCENDANT, proposal.getID(), true);
    }

    name QuestStartProposal::getProposalType()
    {
        return common::QUEST_START;
    }
}
