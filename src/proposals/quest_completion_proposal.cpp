#include <proposals/quest_completion_proposal.hpp>
#include <common.hpp>
#include <util.hpp>

namespace hypha
{

void QuestCompletionProposal::proposeImpl(const name &proposer, ContentWrapper &contentWrapper)
{
    //Verify recipient in quest start is the same as the proposer of the quest completion
    auto questStartDoc = *getParent(common::QUEST_START_ITEM, common::QUEST_START, contentWrapper, true);

    auto questStartCW = questStartDoc.getContentWrapper();

    name recipient = questStartCW.getOrFail(DETAILS, RECIPIENT)->getAs<eosio::name>();

    EOS_CHECK(
        proposer == recipient,
        "Proposer must be the recipient of the Quest Start"
    );

    //Quest start must have been approved

    auto state = questStartCW.getOrFail(DETAILS, common::STATE)->getAs<string>();

    EOS_CHECK(
        state == common::STATE_APPROVED,
        to_str("Quest Completion can be created only for approved Quest Start proposals")
    );
}

void QuestCompletionProposal::postProposeImpl(Document &proposal)
{
    auto cw = proposal.getContentWrapper();
    
    //We must have stablished a quest start 
    auto questStartDoc = *getParent(common::QUEST_START_ITEM, common::QUEST_START, cw, true);

    //TODO: Check if the pacted time duration already happened (current time > start_date + period_count)

    //Check start quest hasn't been completed yet or there isn't an open completion proposal
    auto [isCompleted, completeEdge] = Edge::getIfExists(m_dao.get_self(), questStartDoc.getID(), common::COMPLETED_BY);
    
    EOS_CHECK(
        !isCompleted,
        to_str("Quest was already completed by: ", completeEdge.getToNode())
    );

    auto [isLocked, lockedEdge] = Edge::getIfExists(m_dao.get_self(), questStartDoc.getID(), common::LOCKED_BY);

    EOS_CHECK(
        !isLocked,
        to_str("Quest is already being completed by: ", lockedEdge.getToNode())
    );

    //Create edge to start quest
    Edge(m_dao.get_self(), m_dao.get_self(), proposal.getID(), questStartDoc.getID(), common::QUEST_START);
    Edge(m_dao.get_self(), m_dao.get_self(), questStartDoc.getID(), proposal.getID(), common::LOCKED_BY);

}

void QuestCompletionProposal::passImpl(Document &proposal)
{
    TRACE_FUNCTION()

    auto questStart = getParent(proposal.getID(), common::QUEST_START, common::QUEST_START);
    pay(questStart, common::QUEST_COMPLETION);
    Edge(m_dao.get_self(), m_dao.get_self(), questStart.getID(), proposal.getID(), common::COMPLETED_BY);
    Edge(m_dao.get_self(), m_dao.get_self(), proposal.getID(), questStart.getID(), common::COMPLETED);
}

void QuestCompletionProposal::failImpl(Document &proposal)
{
    TRACE_FUNCTION()
    Edge::getTo(m_dao.get_self(), proposal.getID(), common::LOCKED_BY).erase();

    auto questStart = getParent(proposal.getID(), common::QUEST_START, common::QUEST_START);

    //Make a history of failed proposals
    Edge(m_dao.get_self(), m_dao.get_self(), questStart.getID(), proposal.getID(), common::FAILED_PROPS);
}

name QuestCompletionProposal::getProposalType()
{
    return common::QUEST_COMPLETION;
}
}
