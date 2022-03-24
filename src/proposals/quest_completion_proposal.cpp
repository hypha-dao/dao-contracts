#include <proposals/quest_completion_proposal.hpp>
#include <common.hpp>
#include <util.hpp>

namespace hypha
{
    constexpr auto QUEST_START = "quest_start";

    void QuestCompletionProposal::proposeImpl(const name& proposer, ContentWrapper& contentWrapper)
    {
        TRACE_FUNCTION()
        PayoutProposal::proposeImpl(proposer, contentWrapper);
        auto detailsGroup = contentWrapper.getGroupOrFail(DETAILS);

        // Validate the associated quest_start exists
        const uint64_t questStartID = contentWrapper.getOrFail(
            DETAILS,
            QUEST_START,
            "Associated quest_start document not found in the contents"
            )->getAs<int64_t>();

        Document questStart(m_dao.get_self(), questStartID);

        eosio::check(questStart.getCreator() == proposer, "Quest start proposal not owned by proposer");

        const eosio::name documentType = questStart.getContentWrapper().getOrFail(
            SYSTEM,
            TYPE,
            "No type found on quest start document"
            )->getAs<eosio::name>();

        eosio::check(documentType == common::QUEST_START, "Document type is not a quest start");
    }


    void QuestCompletionProposal::postProposeImpl(Document& proposal)
    {
        TRACE_FUNCTION()

        auto contentWrapper = proposal.getContentWrapper();

        const uint64_t questStartHash = contentWrapper.getOrFail(
            DETAILS,
            QUEST_START,
            "Associated quest_start document not found in the contents"
            )->getAs<int64_t>();

        Document questStartDoc(m_dao.get_self(), questStartHash);

        Edge::write(
            m_dao.get_self(),
            m_dao.get_self(),
            proposal.getID(),
            questStartDoc.getID(),
            common::QUEST_START
            );
    }


    void QuestCompletionProposal::passImpl(Document& proposal)
    {
        TRACE_FUNCTION()
        Edge::write(m_dao.get_self(), m_dao.get_self(), m_daoID, proposal.getID(), common::QUEST_COMPLETION);
        pay(proposal, common::QUEST_COMPLETION);

        auto questStart = m_dao.getGraph().getEdgesFrom(proposal.getID(), common::QUEST_START);

        EOS_CHECK(
            !questStart.empty(),
            util::to_str("Missing 'quest_start' edge from quest_completion: ", proposal.getID())
            )

        Edge::write(m_dao.get_self(), m_dao.get_self(), questStart.at(0).getToNode(), proposal.getID(), common::COMPLETED_BY);
    }


    name QuestCompletionProposal::getProposalType()
    {
        return(common::QUEST_COMPLETION);
    }
}
