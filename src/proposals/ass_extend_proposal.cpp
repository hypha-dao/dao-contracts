#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>

#include <document_graph/document.hpp>
#include <document_graph/content_wrapper.hpp>

#include <common.hpp>
#include <util.hpp>
#include <proposals/edit_proposal.hpp>
#include <proposals/ass_extend_proposal.hpp>
#include <assignment.hpp>

namespace hypha
{

    void AssignmentExtensionProposal::proposeImpl(const name &proposer, ContentWrapper &contentWrapper)
    { 
        // the original document must be an assignment
        Assignment assignment (&m_dao, contentWrapper.getOrFail(DETAILS, ORIGINAL_DOCUMENT)->getAs<eosio::checksum256>());

        int64_t currentPeriodCount = assignment.getPeriodCount();
        int64_t newPeriodCount = contentWrapper.getOrFail(DETAILS, PERIOD_COUNT)->getAs<int64_t>();
        eosio::print("current period count is: " + std::to_string(currentPeriodCount) + "\n");
        eosio::print("new period count is: " + std::to_string(newPeriodCount) + "\n");

        eosio::check (newPeriodCount > currentPeriodCount, PERIOD_COUNT + 
            string(" on the proposal must be greater than the period count on the existing assignment; original: ") + 
            std::to_string(currentPeriodCount) + "; proposed: " + std::to_string(newPeriodCount));
    }

    void AssignmentExtensionProposal::postProposeImpl (Document &proposal) 
    {
        ContentWrapper proposalContent = proposal.getContentWrapper();

        // confirm that the original document exists
        Document original (m_dao.get_self(), proposalContent.getOrFail(DETAILS, ORIGINAL_DOCUMENT)->getAs<eosio::checksum256>());

        // connect the edit proposal to the original
        Edge::write (m_dao.get_self(), m_dao.get_self(), proposal.getHash(), original.getHash(), common::ORIGINAL);
        eosio::print("writing edge from proposal to original. proposal: " + readableHash(proposal.getHash()) + "\n");
        eosio::print("original: " + readableHash(original.getHash()) + "\n");

    }

    void AssignmentExtensionProposal::passImpl(Document &proposal)
    {
        // merge the original with the edits and save
        ContentWrapper proposalContent = proposal.getContentWrapper();

        // confirm that the original document exists
        Document original (m_dao.get_self(), proposalContent.getOrFail(DETAILS, ORIGINAL_DOCUMENT)->getAs<eosio::checksum256>());

        // update all edges to point to the new document
        Document merged = Document::merge(original, proposal);
        merged.emplace ();

        // replace the original node with the new one in the edges table
        m_dao.getGraph().replaceNode(original.getHash(), merged.getHash());

        // erase the original document
        m_dao.getGraph().eraseDocument(original.getHash(), true);
    }

    std::string AssignmentExtensionProposal::getBallotContent (ContentWrapper &contentWrapper)
    {
        return std::string{"Assignment Extension Proposal"};
    }
    
    name AssignmentExtensionProposal::getProposalType () 
    {
        return common::EXTENSION;
    }

} // namespace hypha