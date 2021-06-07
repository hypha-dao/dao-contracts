#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>

#include <document_graph/document.hpp>
#include <document_graph/content_wrapper.hpp>

#include <logger/logger.hpp>

#include <common.hpp>
#include <util.hpp>
#include <proposals/edit_proposal.hpp>
#include <proposals/ass_extend_proposal.hpp>
#include <assignment.hpp>

namespace hypha
{

    void AssignmentExtensionProposal::proposeImpl(const name &proposer, ContentWrapper &contentWrapper)
    {   
        TRACE_FUNCTION()
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
        TRACE_FUNCTION()
        ContentWrapper proposalContent = proposal.getContentWrapper();

        // confirm that the original document exists
        Document original (m_dao.get_self(), proposalContent.getOrFail(DETAILS, ORIGINAL_DOCUMENT)->getAs<eosio::checksum256>());

        //TODO: This edge has to cleaned up when proposal fails
        // connect the edit proposal to the original
        Edge::write (m_dao.get_self(), m_dao.get_self(), proposal.getHash(), original.getHash(), common::ORIGINAL);
        eosio::print("writing edge from proposal to original. proposal: " + readableHash(proposal.getHash()) + "\n");
        eosio::print("original: " + readableHash(original.getHash()) + "\n");

    }

    void AssignmentExtensionProposal::passImpl(Document &proposal)
    {
        TRACE_FUNCTION()
        // merge the original with the edits and save
        ContentWrapper proposalContent = proposal.getContentWrapper();

        auto [detailsIdx, details] = proposalContent.getGroup(DETAILS);

        //Save a copy of original details
        auto originalContents = proposal.getContentGroups();

        auto skippedGroups = {SYSTEM, BALLOT, BALLOT_OPTIONS};

        for (auto groupLabel : skippedGroups) 
        {
          if (auto [groupIdx, group] = proposalContent.getGroup(groupLabel);
              group != nullptr)
          {
            proposalContent.insertOrReplace(groupIdx, 
                                            Content{"skip_from_merge", 0});
          }
        }

        auto ignoredDetailsItems = {TITLE,
                                    DESCRIPTION,
                                    "ballot_title",
                                    "ballot_description",
                                    ORIGINAL_DOCUMENT};

        for (auto item : ignoredDetailsItems) 
        {
          if (auto [_, content] = proposalContent.get(detailsIdx, item);
              content != nullptr)
          {
            proposalContent.removeContent(detailsIdx, item);
          }
        }
        
        // confirm that the original document exists
        // Use the ORIGINAL edge since the original document could have changed since this was 
        // proposed
        //Document original (m_dao.get_self(), proposalContent.getOrFail(DETAILS, ORIGINAL_DOCUMENT)->getAs<eosio::checksum256>());
        auto edges = m_dao.getGraph().getEdgesFrom(proposal.getHash(), common::ORIGINAL);
        
        EOS_CHECK(
          edges.size() == 1, 
          "Missing edge from extension proposal: " + readableHash(proposal.getHash()) + " to original document"
        );

        Document original (m_dao.get_self(), edges[0].getToNode());

        // update all edges to point to the new document
        Document merged = Document::merge(original, proposal);
        merged.emplace ();

        // replace the original node with the new one in the edges table
        m_dao.getGraph().replaceNode(original.getHash(), merged.getHash());

        // erase the original document
        m_dao.getGraph().eraseDocument(original.getHash(), true);

        //Restore groups
        proposalContent.getContentGroups() = std::move(originalContents);
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