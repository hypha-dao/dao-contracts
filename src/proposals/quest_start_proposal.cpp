#include <proposals/quest_start_proposal.hpp>
#include <common.hpp>
#include <util.hpp>
#include <member.hpp>

namespace hypha
{
    //TODO: Add required parameters check on proposeImpl
    //Also we should specify the quest rewards here
    void QuestStartProposal::passImpl(Document&proposal)
    {
        TRACE_FUNCTION()
        Edge::write(m_dao.get_self(), m_dao.get_self(), m_daoID, proposal.getID(), common::QUEST_START);

        ContentWrapper contentWrapper = proposal.getContentWrapper();
        Member         assignee       = Member(m_dao, m_dao.getMemberID(contentWrapper.getOrFail(DETAILS, ASSIGNEE)->getAs <eosio::name>()));

        Edge::write(m_dao.get_self(), m_dao.get_self(), assignee.getID(), proposal.getID(), common::ASSIGNED);
        Edge::write(m_dao.get_self(), m_dao.get_self(), proposal.getID(), assignee.getID(), common::ASSIGNEE_NAME);
    }

    name QuestStartProposal::getProposalType()
    {
        return(common::QUEST_START);
    }
}
