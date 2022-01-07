#include <proposals/quest_start_proposal.hpp>
#include <common.hpp>
#include <util.hpp>
#include <member.hpp>

namespace hypha
{
    void QuestStartProposal::passImpl(Document &proposal)
    {
        TRACE_FUNCTION()
        Edge::write(m_dao.get_self(), m_dao.get_self(), getRoot(m_dao.get_self()), proposal.getHash(), common::QUEST_START);
        
        ContentWrapper contentWrapper = proposal.getContentWrapper();
        eosio::checksum256 assignee = Member::calcHash(contentWrapper.getOrFail(DETAILS, ASSIGNEE)->getAs<eosio::name>());
        
        Edge::write(m_dao.get_self(), m_dao.get_self(), assignee, proposal.getHash(), common::ASSIGNED);
        Edge::write(m_dao.get_self(), m_dao.get_self(), proposal.getHash(), assignee, common::ASSIGNEE_NAME);
    }

    name QuestStartProposal::getProposalType()
    {
        return common::QUEST_START;
    }
}
