#include <algorithm>

#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>

#include <document_graph/document.hpp>
#include <document_graph/content_wrapper.hpp>

#include <common.hpp>
#include <util.hpp>
#include <proposals/suspend_proposal.hpp>
#include <assignment.hpp>
#include <period.hpp>
#include <logger/logger.hpp>

namespace hypha
{

    void SuspendProposal::proposeImpl(const name &proposer, ContentWrapper &contentWrapper)
    { 
      TRACE_FUNCTION()

      // original_document is a required hash
      auto originalDocHash = contentWrapper.getOrFail(DETAILS, ORIGINAL_DOCUMENT)->getAs<eosio::checksum256>();

      auto root = getRoot(m_dao.get_self());

      //Verify if this is a passed proposal
      EOS_CHECK(
        Edge::exists(m_dao.get_self(), root, originalDocHash, common::PASSED_PROPS),
        "Only passed proposals can be suspended"
      )
      
      Document originalDoc(m_dao.get_self(), originalDocHash);

      if (auto [hasOpenSuspendProp, proposalHash] = hasOpenProposal(common::SUSPEND, originalDocHash);
          hasOpenSuspendProp) {
        EOS_CHECK(
          false,
          to_str("There is an open suspension proposal already:", proposalHash)  
        )
      }

      ContentWrapper ocw = originalDoc.getContentWrapper();

      //TODO-J: Add state item to all active Roles/Assignments
      //Check assignments from 2 months ago with dgraph and then send them to a fix action (should verify if they are still active or not)
      //Check all roles that are not suspened (?)
      auto state = ocw.getOrFail(DETAILS, common::STATE)->getAs<string>();

      EOS_CHECK(
        state != common::STATE_WITHDRAWED && 
        state != common::STATE_SUSPENDED &&
        state != common::STATE_EXPIRED &&
        state != common::STATE_REJECTED,
        to_str("Cannot open suspend proposals on ", state, " documents")
      )

      auto type = ocw.getOrFail(SYSTEM, TYPE)->getAs<name>();
      
      switch (type.value)
      {
      case common::ASSIGNMENT.value: {
        
        Assignment assignment(&m_dao, originalDoc.getHash());

        auto currentTimeSecs = eosio::current_time_point().sec_since_epoch();

        auto lastPeriodEndSecs = assignment.getLastPeriod()
                                           .getEndTime()
                                           .sec_since_epoch();

        EOS_CHECK(
          currentTimeSecs < lastPeriodEndSecs,
          "Assignment is already expired"
        );
       } break;
      case common::ROLE_NAME.value:

        

        break;
      default:
        EOS_CHECK(
          false,
          to_str("Unexpected document type for suspension: ",
                 type, ". Valid types [", common::ASSIGNMENT ,"]")
        );
        break;
      }

      auto title = ocw.getOrFail(DETAILS, TITLE)->getAs<string>();

      ContentWrapper::insertOrReplace(*contentWrapper.getGroupOrFail(DETAILS), 
                                      Content { TITLE, to_str("Suspension of ", type, ": ", title ) });
    }

    void SuspendProposal::postProposeImpl (Document &proposal) 
    {
      TRACE_FUNCTION()
      auto originalDocHash = proposal.getContentWrapper()
                                     .getOrFail(DETAILS, ORIGINAL_DOCUMENT)
                                     ->getAs<eosio::checksum256>();

      Edge::write (m_dao.get_self(), m_dao.get_self(), proposal.getHash(), originalDocHash, common::SUSPEND);
    }

    void SuspendProposal::passImpl(Document &proposal)
    {
      TRACE_FUNCTION()
      auto edges = m_dao.getGraph().getEdgesFrom(proposal.getHash(), common::SUSPEND);

      EOS_CHECK(
        edges.size() == 1, 
        "Missing edge from suspension proposal: " + readableHash(proposal.getHash()) + " to document"
      );

      auto root = getRoot(m_dao.get_self());

      Document originalDoc(m_dao.get_self(), edges[0].getToNode());

      ContentWrapper ocw = originalDoc.getContentWrapper();

      auto detailsGroup = ocw.getGroupOrFail(DETAILS);

      ContentWrapper::insertOrReplace(*detailsGroup, Content { common::STATE, common::STATE_SUSPENDED });

      originalDoc = m_dao.getGraph().updateDocument(originalDoc.getCreator(), 
                                                    originalDoc.getHash(), 
                                                    ocw.getContentGroups());

      auto type = ocw.getOrFail(SYSTEM, TYPE)->getAs<name>();
      
      switch (type.value)
      {
      case common::ASSIGNMENT.value: {

        Assignment assignment(&m_dao, originalDoc.getHash());

        auto cw = assignment.getContentWrapper();

        auto originalPeriods = assignment.getPeriodCount();

        //Calculate the number of periods since start period to the current period
        auto currentPeriod = Period::current(&m_dao);
        
        auto periodsToCurrent = assignment.getStartPeriod().getPeriodCountTo(currentPeriod);

        periodsToCurrent = std::min(periodsToCurrent + 1, originalPeriods);

        if (originalPeriods != periodsToCurrent) {

          auto detailsGroup = cw.getGroupOrFail(DETAILS);

          ContentWrapper::insertOrReplace(*detailsGroup, Content { PERIOD_COUNT, periodsToCurrent });

          originalDoc = m_dao.getGraph().updateDocument(assignment.getCreator(), 
                                                         assignment.getHash(), 
                                                         cw.getContentGroups());

          assignment = Assignment(&m_dao, originalDoc.getHash());
        }

        m_dao.modifyCommitment(assignment, 0, std::nullopt, common::MOD_WITHDRAW);
      } break;
      case common::ROLE_NAME.value: {
        
      }  break;
      default: {
        EOS_CHECK(
          false,
          to_str("Unexpected document type for suspension: ",
                 type, ". Valid types [", common::ASSIGNMENT ,"]")
        );
      } break;
      }

      //root --> suspended --> proposal
      Edge(m_dao.get_self(), m_dao.get_self(), root, originalDoc.getHash(), common::SUSPENDED);
    }

    std::string SuspendProposal::getBallotContent (ContentWrapper &contentWrapper)
    {
        TRACE_FUNCTION()
        return getTitle(contentWrapper);
    }
    
    name SuspendProposal::getProposalType () 
    {
        return common::SUSPEND;
    }
} // namespace hypha