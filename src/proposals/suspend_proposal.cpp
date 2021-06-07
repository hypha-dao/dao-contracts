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
      //TODO: Setup ballot_title according to document type [Assignment, Badge, etc.]
      TRACE_FUNCTION()

      // original_document is a required hash
      auto originalDocHash = contentWrapper.getOrFail(DETAILS, ORIGINAL_DOCUMENT)->getAs<eosio::checksum256>();

      Document originalDoc(m_dao.get_self(), originalDocHash);

      ContentWrapper ocw = originalDoc.getContentWrapper();

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
                                      Content { TITLE, to_str("Suspention of ", type, ":", title ) });
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

      Document originalDoc(m_dao.get_self(), edges[0].getToNode());

      ContentWrapper ocw = originalDoc.getContentWrapper();

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

          auto newHash = m_dao.getGraph().updateDocument(assignment.getCreator(), 
                                                         assignment.getHash(), 
                                                         cw.getContentGroups()).getHash();

          assignment = Assignment(&m_dao, newHash);
        }

        m_dao.modifyCommitment(assignment, 0, std::nullopt, common::MOD_WITHDRAW);
      } break;
      default: {
        EOS_CHECK(
          false,
          to_str("Unexpected document type for suspension: ",
                 type, ". Valid types [", common::ASSIGNMENT ,"]")
        );
      } break;
      }
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