#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>

#include <document_graph/document.hpp>
#include <document_graph/content_wrapper.hpp>

#include <common.hpp>
#include <util.hpp>
#include <proposals/edit_proposal.hpp>
#include <proposals/payout_proposal.hpp>
#include <assignment.hpp>
#include <period.hpp>
#include <logger/logger.hpp>

#include <badges/badges.hpp>

namespace hypha
{

  /**
   * TODO: Refactor this class into only allowing fetching specific items
   * for each proposal type in order to avoid editing restricted data.
   * Also implement locked by idiom to avoid reading all existing edit proposals
   */

  void EditProposal::proposeImpl(const name& proposer, ContentWrapper& contentWrapper)
  {
    auto originalDocHash = contentWrapper.getOrFail(DETAILS, ORIGINAL_DOCUMENT)->getAs<int64_t>();

    Document originalDoc(m_dao.get_self(), originalDocHash);

    if (auto [hasOpenEditProp, proposalHash] = hasOpenProposal(common::ORIGINAL, originalDoc.getID());
      hasOpenEditProp) {
      EOS_CHECK(
        false,
        to_str("There is an open edit proposal already:", proposalHash)
      )
    }
  }

  void EditProposal::postProposeImpl(Document& proposal)
  {
    TRACE_FUNCTION()
      ContentWrapper proposalContent = proposal.getContentWrapper();

    // original_document is a required hash
    auto originalDocID = static_cast<uint64_t>(
      proposalContent.getOrFail(DETAILS, ORIGINAL_DOCUMENT)->getAs<int64_t>()
      );

    Document original;

    //TODO: Make restricted items list that should never be edited in 
    //any proposal type, i.e. dao, state, original_approved_date

    //Valid for both Assignment and BadgeAssignment
    if (auto edges = m_dao.getGraph().getEdgesTo(originalDocID, common::ASSIGNMENT);
      !edges.empty())
    {
      RecurringActivity assignment(&m_dao, originalDocID);

      int64_t currentPeriodCount = assignment.getPeriodCount();

      //Check if the proposal is to extend the period count
      if (auto [_, count] = proposalContent.get(DETAILS, PERIOD_COUNT);
        count != nullptr)
      {
        auto newPeriodCount = count->getAs<int64_t>();
        eosio::print("current period count is: " + std::to_string(currentPeriodCount) + "\n");
        eosio::print("new period count is: " + std::to_string(newPeriodCount) + "\n");

        EOS_CHECK(
          newPeriodCount > currentPeriodCount,
          PERIOD_COUNT +
          string(" on the proposal must be greater than the period count on the existing assignment; original: ") +
          std::to_string(currentPeriodCount) + "; proposed: " + std::to_string(newPeriodCount)
        );

        EOS_CHECK(
          (newPeriodCount - currentPeriodCount) <= 52,
          "The assignment cannot be extended for more than 52 periods."
        );
      }

      auto currentTimeSecs = eosio::current_time_point().sec_since_epoch();

      auto lastGracePeriodEndSecs = assignment.getLastPeriod()
        .getNthPeriodAfter(2)
        .getEndTime()
        .sec_since_epoch();

      EOS_CHECK(
        lastGracePeriodEndSecs > currentTimeSecs,
        "Extension grace period (2 periods after expiration) is over, create a new one instead"
      );

      original = std::move(*static_cast<Document*>(&assignment));
    }
    else {
      // confirm that the original document exists
      original = Document(m_dao.get_self(), originalDocID);
    }

    ContentWrapper ocw = original.getContentWrapper();

    auto type = ocw.getOrFail(SYSTEM, TYPE)->getAs<name>();

    if (type == common::BUDGET) {
      //Check that we have required token items
      PayoutProposal::checkTokenItems(m_daoSettings, proposalContent);
    }

    auto state = ocw.getOrFail(DETAILS, common::STATE)->getAs<string>();

    EOS_CHECK(
      state == common::STATE_APPROVED ||
      state == common::STATE_ARCHIVED, //We could still extend after the assignment expires
      to_str("Cannot open edit proposals on ", state, " documents")
    )

      // connect the edit proposal to the original
      Edge::write(m_dao.get_self(), m_dao.get_self(), proposal.getID(), original.getID(), common::ORIGINAL);
  }

  void EditProposal::passImpl(Document& proposal)
  {
    TRACE_FUNCTION()
      // merge the original with the edits and save
      auto proposalCpy = proposal.getContentGroups();

    ContentWrapper proposalContent(proposalCpy);

    auto [detailsIdx, details] = proposalContent.getGroup(DETAILS);

    auto skippedGroups = { SYSTEM, BALLOT, BALLOT_OPTIONS };

    for (auto groupLabel : skippedGroups)
    {
      if (auto [groupIdx, group] = proposalContent.getGroup(groupLabel);
        group != nullptr)
      {
        proposalContent.insertOrReplace(groupIdx,
          Content{ "skip_from_merge", 0 });
      }
    }

    //Warning: This will automatically change the state of the original document to approved
    auto ignoredDetailsItems = { ORIGINAL_DOCUMENT,
                                common::BALLOT_TITLE,
                                common::BALLOT_DESCRIPTION };

    for (auto item : ignoredDetailsItems)
    {
      if (auto [_, content] = proposalContent.get(detailsIdx, item);
        content != nullptr)
      {
        proposalContent.removeContent(detailsIdx, item);
      }
    }

    // confirm that the original document exists
    auto edges = m_dao.getGraph().getEdgesFrom(proposal.getID(), common::ORIGINAL);

    EOS_CHECK(
      edges.size() == 1,
      "Missing edge from edit proposal: " + to_str(proposal.getID()) + " to original document"
    );

    Document original(m_dao.get_self(), edges[0].getToNode());

    Document toMerge;
    toMerge.content_groups = std::move(proposalCpy);
    // update all edges to point to the new document
    Document merged = Document::merge(std::move(original), toMerge);

    //Re-schedule archive if activity period count was changed
    if (RecurringActivity::isRecurringActivity(merged) &&
      proposalContent.exists(DETAILS, PERIOD_COUNT)) {
      RecurringActivity recAct(&m_dao, merged.getID());

      recAct.scheduleArchive();

      auto& state = recAct.getContentWrapper()
        .getOrFail(DETAILS, common::STATE)->getAs<string>();

      //We have to re-activate if state is archived and expirationDate > now
      if (state == common::STATE_ARCHIVED &&
        eosio::current_time_point() < recAct.getLastPeriod().getEndTime()) {
        state = common::STATE_APPROVED;

        //Also if the Original Document is of BadgeAssignment type, let's reactivate it
        if (recAct.getContentWrapper()
          .getOrFail(SYSTEM, TYPE)->getAs<name>() == common::ASSIGN_BADGE) {
          badges::onBadgeActivated(m_dao, recAct);
        }
      }
    }

    merged.update();
    // replace the original node with the new one in the edges table
    // m_dao.getGraph().replaceNode(original.getID(), merged.getID());

    // // erase the original document
    // m_dao.getGraph().eraseDocument(original.getID(), true);
  }

  std::string EditProposal::getBallotContent(ContentWrapper& contentWrapper)
  {
    TRACE_FUNCTION()
      return getTitle(contentWrapper);
  }

  name EditProposal::getProposalType()
  {
    return common::EDIT;
  }

} // namespace hypha