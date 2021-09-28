#include <proposals/quest_start_proposal.hpp>
#include <common.hpp>
#include <util.hpp>

namespace hypha
{
    void QuestStartProposal::passImpl(Document &proposal)
    {
        TRACE_FUNCTION()
        Edge::write(m_dao.get_self(), m_dao.get_self(), getRoot(m_dao.get_self()), proposal.getHash(), common::QUEST_START);
    }

    name QuestStartProposal::getProposalType()
    {
        return common::QUEST_START;
    }
}