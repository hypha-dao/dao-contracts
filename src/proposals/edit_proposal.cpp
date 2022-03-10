#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>

#include <document_graph/document.hpp>
#include <document_graph/content_wrapper.hpp>

#include <common.hpp>
#include <util.hpp>
#include <proposals/edit_proposal.hpp>
#include <assignment.hpp>
#include <period.hpp>
#include <logger/logger.hpp>

namespace hypha
{

    void EditProposal::proposeImpl(const name &proposer, ContentWrapper &contentWrapper)
    { 
      EOS_CHECK(
        Member::isMember(m_dao, m_daoID, proposer),
        util::to_str("Only members of: ", m_daoID, " can edit this proposal")
      )

      auto originalDocHash = contentWrapper.getOrFail(DETAILS, ORIGINAL_DOCUMENT)->getAs<int64_t>();

      Document originalDoc(m_dao.get_self(), originalDocHash);

      if (auto [hasOpenEditProp, proposalHash] = hasOpenProposal(common::SUSPEND, originalDoc.getID());
          hasOpenEditProp) {
        EOS_CHECK(
          false,
          util::to_str("There is an open edit proposal already:", proposalHash)  
        )
      }
    }

    void EditProposal::postProposeImpl (Document &proposal) 
    {
        TRACE_FUNCTION()
        ContentWrapper proposalContent = proposal.getContentWrapper();

        // original_document is a required hash
        auto originalDocID = static_cast<uint64_t>(
          proposalContent.getOrFail(DETAILS, ORIGINAL_DOCUMENT)->getAs<int64_t>()
        );

        Document original;

        if (auto edges = m_dao.getGraph().getEdgesTo(originalDocID, common::ASSIGNMENT);
            !edges.empty()) 
        {
            // the original document must be an assignment
            Assignment assignment (&m_dao, originalDocID);

            int64_t currentPeriodCount = assignment.getPeriodCount();
            
            //Check if the proposal is to extend the period count
            if (auto [_, count] = proposalContent.get(DETAILS, PERIOD_COUNT);
                count != nullptr)
            {
                auto newPeriodCount = count->getAs<int64_t>();
                eosio::print("current period count is: " + std::to_string(currentPeriodCount) + "\n");
                eosio::print("new period count is: " + std::to_string(newPeriodCount) + "\n");

                eosio::check (
                  newPeriodCount > currentPeriodCount, 
                  PERIOD_COUNT + 
                  string(" on the proposal must be greater than the period count on the existing assignment; original: ") + 
                  std::to_string(currentPeriodCount) + "; proposed: " + std::to_string(newPeriodCount)
                );    
            }
                           
            auto currentTimeSecs = eosio::current_time_point().sec_since_epoch();

            auto lastGracePeriodEndSecs = assignment.getLastPeriod()
                                             .getNthPeriodAfter(2)
                                             .getEndTime()
                                             .sec_since_epoch();

            EOS_CHECK(
              lastGracePeriodEndSecs > currentTimeSecs, 
              "Assignment extension grace period (2 periods after expiration) is over, create a new one instead"
            );
            
            original = std::move(*static_cast<Document*>(&assignment));
        }
        else {
          // confirm that the original document exists
            original = Document(m_dao.get_self(), originalDocID);
        }

        ContentWrapper ocw = original.getContentWrapper();

        auto state = ocw.getOrFail(DETAILS, common::STATE)->getAs<string>();

        EOS_CHECK(
          state == common::STATE_APPROVED ||
          state == common::STATE_ARCHIVED, //We could still extend after the assignment expires
          util::to_str("Cannot open edit proposals on ", state, " documents")
        )

        // connect the edit proposal to the original
        Edge::write (m_dao.get_self(), m_dao.get_self(), proposal.getID(), original.getID(), common::ORIGINAL);
    }

    void EditProposal::passImpl(Document &proposal)
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

        //Warning: This will automatically change the state of the original document to approved
        auto ignoredDetailsItems = {ORIGINAL_DOCUMENT,
                                    common::BALLOT_TITLE,
                                    common::BALLOT_DESCRIPTION};

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

        auto edges = m_dao.getGraph().getEdgesFrom(proposal.getID(), common::ORIGINAL);
        
        EOS_CHECK(
          edges.size() == 1, 
          "Missing edge from extension proposal: " + util::to_str(proposal.getID()) + " to original document"
        );

        Document original (m_dao.get_self(), edges[0].getToNode());
        
        // update all edges to point to the new document
        Document merged = Document::merge(original, proposal);
        merged.emplace ();

        // replace the original node with the new one in the edges table
        m_dao.getGraph().replaceNode(original.getID(), merged.getID());

        // erase the original document
        m_dao.getGraph().eraseDocument(original.getID(), true);

        //Restore groups
        proposalContent.getContentGroups() = std::move(originalContents);
    }

    std::string EditProposal::getBallotContent (ContentWrapper &contentWrapper)
    {
        TRACE_FUNCTION()
        return getTitle(contentWrapper);
    }
    
    name EditProposal::getProposalType () 
    {
        return common::EDIT;
    }

} // namespace hypha