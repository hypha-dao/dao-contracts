#include <proposals/quest_completion_proposal.hpp>
#include <common.hpp>
#include <util.hpp>

namespace hypha
{

    constexpr std::string QUEST_START = "quest_start";

    void QuestCompletionProposal::proposeImpl(const name &proposer, ContentWrapper &contentWrapper)
    {
        TRACE_FUNCTION()
        PayoutProposal::proposeImpl(proposer, contentWrapper);
        auto detailsGroup = contentWrapper.getGroupOrFail(DETAILS);

        // Validate the associated quest_start exists
        const eosio::checksum256 questStartHash = contentWrapper.getOrFail(
            DETAILS, 
            QUEST_START,
            "Associated quest_document not found in the contents"
        )->getAs<eosio::checksum256>();

        Document questStart(this->m_dao.get_self(), questStartHash);
        eosio::check(questStart.getCreator() == proposer, "Quest start proposal not owned by proposer");

        const eosio::name documentType = questStart.getContentWrapper().getOrFail(
            SYSTEM, 
            TYPE, 
            "No type found on quest start document"
        )->getAs<eosio::name>();
        eosio::check(documentType == QUEST_START, "Document type is not a quest start");
    }

    void QuestCompletionProposal::passImpl(Document &proposal)
    {
        TRACE_FUNCTION()
        Edge::write(m_dao.get_self(), m_dao.get_self(), getRoot(m_dao.get_self()), proposal.getHash(), common::QUEST_COMPLETION);
        pay(proposal, common::QUEST_COMPLETION);

        const eosio::checksum256 questStartHash = contentWrapper.getOrFail(
            DETAILS, 
            QUEST_START
        )->getAs<eosio::checksum256>();
        Edge::write(m_dao.get_self(), m_dao.get_self(), questStartHash, proposal.getHash(), common::COMPLETED_BY)
    }

    name QuestCompletionProposal::getProposalType()
    {
        return common::QUEST_COMPLETION;
    }
}
