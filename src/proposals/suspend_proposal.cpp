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

      EOS_CHECK(
        Member::isMember(m_dao, m_daoID , proposer),
        util::to_str("Only members of: ", m_daoID , " can suspend this proposal")
      )

      // original_document is a required hash
      auto originalDocHash = contentWrapper.getOrFail(DETAILS, ORIGINAL_DOCUMENT)->getAs<int64_t>();

      Document originalDoc(m_dao.get_self(), originalDocHash);

      //Verify if this is a passed proposal
      EOS_CHECK(
        Edge::exists(m_dao.get_self(), m_daoID , originalDoc.getID(), common::PASSED_PROPS),
        "Only passed proposals can be suspended"
      )
      
      if (auto [hasOpenSuspendProp, proposalID] = hasOpenProposal(common::SUSPEND, originalDoc.getID());
          hasOpenSuspendProp) {
        EOS_CHECK(
          false,
          to_str("There is an open suspension proposal already:", proposalID)  
        )
      }

      ContentWrapper ocw = originalDoc.getContentWrapper();

      //TODO-J: Add state item to all active Roles/Assignments
      //Check assignments from 2 months ago with dgraph and then send them to a fix action (should verify if they are still active or not)
      //Check all roles that are not suspened (?)
      auto state = ocw.getOrFail(DETAILS, common::STATE)->getAs<string>();

      EOS_CHECK(
        state != common::STATE_PROPOSED &&
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
        
        Assignment assignment(&m_dao, originalDoc.getID());

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
        //We don't have to do anything special for roles
        break;
      case common::BADGE_NAME.value:        
        break;
      case common::ASSIGN_BADGE.value:
        
        break;
      default:
        EOS_CHECK(
          false,
          to_str("Unexpected document type for suspension: ",
                 type, ". Valid types [", common::ASSIGNMENT, ", ", common::ROLE_NAME ,"]")
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
      auto originalDocID = proposal.getContentWrapper()
                                     .getOrFail(DETAILS, ORIGINAL_DOCUMENT)
                                     ->getAs<int64_t>();

      Edge::write (m_dao.get_self(), m_dao.get_self(), proposal.getID(), originalDocID, common::SUSPEND);
    }

    void SuspendProposal::passImpl(Document &proposal)
    {
      TRACE_FUNCTION()
      auto edges = m_dao.getGraph().getEdgesFrom(proposal.getID(), common::SUSPEND);

      EOS_CHECK(
        edges.size() == 1, 
        "Missing edge from suspension proposal: " + util::to_str(proposal.getID()) + " to document"
      );

      Document originalDoc(m_dao.get_self(), edges[0].getToNode());

      ContentWrapper ocw = originalDoc.getContentWrapper();

      auto detailsGroup = ocw.getGroupOrFail(DETAILS);

      ContentWrapper::insertOrReplace(*detailsGroup, Content { common::STATE, common::STATE_SUSPENDED });

      originalDoc = m_dao.getGraph().updateDocument(originalDoc.getCreator(), 
                                                    originalDoc.getID(), 
                                                    ocw.getContentGroups());

      auto type = ocw.getOrFail(SYSTEM, TYPE)->getAs<name>();
      
      switch (type.value)
      {
      case common::ASSIGNMENT.value: {

        Assignment assignment(&m_dao, originalDoc.getID());

        auto cw = assignment.getContentWrapper();

        auto originalPeriods = assignment.getPeriodCount();

        auto startPeriod = assignment.getStartPeriod();

        //Calculate the number of periods since start period to the current period
        auto currentPeriod = startPeriod.getPeriodUntil(eosio::current_time_point());
        
        auto periodsToCurrent = startPeriod.getPeriodCountTo(currentPeriod);

        periodsToCurrent = std::min(periodsToCurrent + 1, originalPeriods);

        if (originalPeriods != periodsToCurrent) {

          auto detailsGroup = cw.getGroupOrFail(DETAILS);

          ContentWrapper::insertOrReplace(*detailsGroup, Content { PERIOD_COUNT, periodsToCurrent });

          originalDoc = m_dao.getGraph().updateDocument(assignment.getCreator(), 
                                                         assignment.getID(), 
                                                         cw.getContentGroups());

          assignment = Assignment(&m_dao, originalDoc.getID());
        }

        m_dao.modifyCommitment(assignment, 0, std::nullopt, common::MOD_WITHDRAW);
      } break;
      case common::ROLE_NAME.value: {
        //We don't have to do anything special for roles
      }  break;
      default: {
        EOS_CHECK(
          false,
          to_str("Unexpected document type for suspension: ",
                 type, ". Valid types [", common::ASSIGNMENT ,"]")
        );
      } break;
      }

      //dao --> suspended --> proposal
      Edge(m_dao.get_self(), m_dao.get_self(), m_daoID , originalDoc.getID(), common::SUSPENDED);
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