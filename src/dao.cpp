#include <algorithm>
#include <limits>
#include <cmath>
#include <set>

#include <document_graph/content_wrapper.hpp>
#include <document_graph/util.hpp>
#include <proposals/proposal.hpp>
#include <proposals/proposal_factory.hpp>
#include <payers/payer_factory.hpp>
#include <payers/payer.hpp>
#include <logger/logger.hpp>
#include <memory>

#include <dao.hpp>
#include <common.hpp>
#include <util.hpp>
#include <member.hpp>
#include <period.hpp>
#include <assignment.hpp>
#include <recurring_activity.hpp>
#include <time_share.hpp>
#include <settings.hpp>
#include <typed_document.hpp>
#include <comments/section.hpp>
#include <comments/comment.hpp>

#include <typed_document_factory.hpp>

namespace hypha
{
  /**Testenv only

  // ACTION dao::adddocs(std::vector<Document>& docs)
  // {
  //   require_auth(get_self());
  //   Document::document_table d_t(get_self(), get_self().value);

  //   for (auto& doc : docs) {
  //     d_t.emplace(get_self(), [&doc](Document& newDoc){
  //       newDoc = std::move(doc);
  //     });
  //   }
  // }

  // ACTION dao::editdoc(uint64_t doc_id, const std::string& group, const std::string& key, const Content::FlexValue &value)
  // {
  //   require_auth(get_self());

  //   Document doc(get_self(), doc_id);

  //   auto cw = doc.getContentWrapper();

  //   cw.insertOrReplace(*cw.getGroupOrFail(group), Content{key, value});

  //   doc.update();
  // }

  // ACTION dao::clean(int64_t docs, int64_t edges)
  // {
  //   require_auth(get_self());

  //   Document::document_table d_t(get_self(), get_self().value);

  //   for (auto it = d_t.begin(); it != d_t.end() && docs-- > 0; ) {
  //     it = d_t.erase(it);
  //   }

  //   Edge::edge_table e_t(get_self(), get_self().value);

  //   for (auto it = e_t.begin(); it != e_t.end() && edges-- > 0; ) {
  //     it = e_t.erase(it);
  //   }
  // }

  // ACTION dao::addedge(std::vector<InputEdge>& edges)
  // {
  //   require_auth(get_self());

  //   Edge::edge_table e_t(get_self(), get_self().value);

  //   for (auto& edge : edges) {
  //     const int64_t edgeID = util::hashCombine(edge.from_node, edge.to_node, edge.edge_name);

  //     EOS_CHECK(
  //       e_t.find(edgeID) == e_t.end(),
  //       util::to_str("Edge from: ", edge.from_node,
  //                   " to: ", edge.to_node,
  //                   " with name: ", edge.edge_name, " already exists")
  //     );

  //     e_t.emplace(get_self(), [&](Edge &e) {

  //       auto& [creator, created_date, from_node, to_node, edge_name] = edge;

  //       e.id = edgeID;
  //       e.from_node_edge_name_index = util::hashCombine(from_node, edge_name);
  //       e.from_node_to_node_index = util::hashCombine(from_node, to_node);
  //       e.to_node_edge_name_index = util::hashCombine(to_node, edge_name);
  //       e.creator = creator;
  //       e.contract = get_self();
  //       e.from_node = from_node;
  //       e.to_node = to_node;
  //       e.edge_name = edge_name;
  //       e.created_date = created_date;
  //     });
  //   }
  // }

  ACTION
  dao::autoenroll(uint64_t dao_id, const name& enroller, const name& member)
  {
    require_auth(get_self());

    //Auto enroll
    auto mem = getOrCreateMember(member);

    mem.apply(dao_id, "Auto enrolled member");
    mem.enroll(enroller, dao_id, "Auto enrolled member");
  }



   ACTION dao::addedge(uint64_t from, uint64_t to, const name& edge_name)
   {
     require_auth(get_self());

     Document fromDoc(get_self(), from);
     Document toDoc(get_self(), to);

     Edge(get_self(), get_self(), fromDoc.getID(), toDoc.getID(), edge_name);
   }
   */

  ACTION dao::remmember(uint64_t dao_id, const std::vector<name>& member_names)
  {
    if (!eosio::has_auth(get_self())) {
      checkAdminsAuth(dao_id);
    }

    for (auto& member : member_names) {
      auto memberID = getMemberID(member);
      Edge::get(get_self(), dao_id, memberID, common::MEMBER).erase();
      Edge::get(get_self(), memberID, dao_id, common::MEMBER_OF).erase();
    }
  }

  ACTION dao::remapplicant(uint64_t dao_id, const std::vector<name>& applicant_names)
  {
    if (!eosio::has_auth(get_self())) {
      checkAdminsAuth(dao_id);
    }

    for (auto& applicant : applicant_names) {
      auto applicantID = getMemberID(applicant);
      Edge::get(get_self(), dao_id, applicantID, common::APPLICANT).erase();
      Edge::get(get_self(), applicantID, dao_id, common::APPLICANT_OF).erase();
    }
  }

   void dao::propose(uint64_t dao_id,
                     const name &proposer,
                     const name &proposal_type,
                     ContentGroups &content_groups,
                     bool publish)
   {
      TRACE_FUNCTION()
      EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

      std::unique_ptr<Proposal> proposal = std::unique_ptr<Proposal>(ProposalFactory::Factory(*this, dao_id, proposal_type));
      proposal->propose(proposer, content_groups, publish);
   }

   void dao::vote(const name& voter, uint64_t proposal_id, string &vote, const std::optional<string> &notes)
   {
      TRACE_FUNCTION()
      EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");
      Document docprop(get_self(), proposal_id);
      name proposal_type = docprop.getContentWrapper().getOrFail(SYSTEM, TYPE)->getAs<eosio::name>();

      auto daoID = Edge::get(get_self(), docprop.getID(), common::DAO).getToNode();

      Proposal *proposal = ProposalFactory::Factory(*this, daoID, proposal_type);
      proposal->vote(voter, vote, docprop, notes);
   }

   void dao::closedocprop(uint64_t proposal_id)
   {
      TRACE_FUNCTION()
      EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

      Document docprop(get_self(), proposal_id);

      auto daoID = Edge::get(get_self(), docprop.getID(), common::DAO).getToNode();

      name proposal_type = docprop.getContentWrapper().getOrFail(SYSTEM, TYPE)->getAs<eosio::name>();

      Proposal *proposal = ProposalFactory::Factory(*this, daoID, proposal_type);
      proposal->close(docprop);
   }

   void dao::proposepub(const name &proposer, uint64_t proposal_id)
   {
      TRACE_FUNCTION()
      EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

      Document docprop(get_self(), proposal_id);
      name proposal_type = docprop.getContentWrapper().getOrFail(SYSTEM, TYPE)->getAs<eosio::name>();

      auto daoID = Edge::get(get_self(), docprop.getID(), common::DAO).getToNode();

      Proposal *proposal = ProposalFactory::Factory(*this, daoID, proposal_type);
      proposal->publish(proposer, docprop);
   }

   void dao::proposerem(const name &proposer, uint64_t proposal_id)
   {
      TRACE_FUNCTION()
      EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

      Document docprop(get_self(), proposal_id);
      name proposal_type = docprop.getContentWrapper().getOrFail(SYSTEM, TYPE)->getAs<eosio::name>();

      auto daoID = Edge::get(get_self(), docprop.getID(), common::DAO).getToNode();

      Proposal *proposal = ProposalFactory::Factory(*this, daoID, proposal_type);
      proposal->remove(proposer, docprop);
   }

   void dao::proposeupd(const name &proposer, uint64_t proposal_id, ContentGroups &content_groups)
   {
      TRACE_FUNCTION()
      EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

      Document docprop(get_self(), proposal_id);
      name proposal_type = docprop.getContentWrapper().getOrFail(SYSTEM, TYPE)->getAs<eosio::name>();

      auto daoID = Edge::get(get_self(), docprop.getID(), common::DAO).getToNode();

      Proposal *proposal = ProposalFactory::Factory(*this, daoID, proposal_type);
      proposal->update(proposer, docprop, content_groups);
   }

   ACTION dao::cmntmigrate(const uint64_t &dao_id)
   {
       TRACE_FUNCTION()
       EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");
       std::vector<Edge> proposalEdges = this->getGraph().getEdgesFrom(dao_id, common::PROPOSAL);
       for (Edge proposalEdge : proposalEdges)
       {
           std::pair<bool, Edge> commentSectionEdge = Edge::getIfExists(this->get_self(), proposalEdge.getToNode(), common::COMMENT_SECTION);
           if (!commentSectionEdge.first)
           {
               Document proposal(this->get_self(), proposalEdge.getToNode());
               Section(*this, proposal);
           }
       }

   }

   ACTION dao::cmntlike(const name &user, const uint64_t comment_section_id)
   {
       TRACE_FUNCTION()
       EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");
       require_auth(user);
       TypedDocumentFactory::getLikeableDocument(*this, comment_section_id)->like(user);
   }

   ACTION dao::cmntunlike(const name &user, const uint64_t comment_section_id)
   {
       TRACE_FUNCTION()
       EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");
       require_auth(user);
       TypedDocumentFactory::getLikeableDocument(*this, comment_section_id)->unlike(user);
   }

   ACTION dao::cmntadd(const name &author, const string content, const uint64_t comment_or_section_id)
   {
       TRACE_FUNCTION()
       EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");
       require_auth(author);

       Document commentOrSection(get_self(), comment_or_section_id);
       eosio::name type = commentOrSection.getContentWrapper().getOrFail(SYSTEM, TYPE)->template getAs<eosio::name>();
       if (type == eosio::name(document_types::COMMENT)) {
           Comment parent(*this, comment_or_section_id);
           Comment(
               *this,
               parent,
               author,
               content
           );
       } else if (type == eosio::name(document_types::COMMENT_SECTION)) {
           Section parent(*this, comment_or_section_id);
           Comment(
               *this,
               parent,
               author,
               content
           );
       } else {
           eosio::check(false, "comment_or_section_id is no the id of a comment or a comment_section");
       }
   }

   ACTION dao::cmntupd(const string new_content, const uint64_t comment_id)
   {
       TRACE_FUNCTION()
       EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

       Comment comment(*this, comment_id);
       require_auth(comment.getAuthor());

       comment.edit(new_content);
   }

   ACTION dao::cmntrem(const uint64_t comment_id)
   {
       TRACE_FUNCTION()
       EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

       Comment comment(*this, comment_id);
       require_auth(comment.getAuthor());

       comment.markAsDeleted();
   }

   void dao::proposeextend(uint64_t assignment_id, const int64_t additional_periods)
   {
      TRACE_FUNCTION()
      EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");
      EOS_CHECK(false, "Use propose action with 'edit' type")

      Assignment assignment(this, assignment_id);

      // only the assignee can submit an extension proposal
      eosio::name assignee = assignment.getAssignee().getAccount();
      eosio::require_auth(assignee);

      auto daoID = Edge::get(get_self(), assignment.getID(), common::DAO).getToNode();

      //Get the DAO edge from the hash
      EOS_CHECK(
        Member::isMember(*this, daoID, assignee),
        "assignee must be a current member to request an extension: " + assignee.to_string());

      eosio::print("\nproposer is: " + assignee.to_string() + "\n");
      // construct ContentGroups to call propose
      auto contentGroups = ContentGroups{
        ContentGroup{
              Content(CONTENT_GROUP_LABEL, DETAILS),
              Content(PERIOD_COUNT, assignment.getPeriodCount() + additional_periods),
              Content(TITLE, std::string("Assignment Extension Proposal")),
              Content(ORIGINAL_DOCUMENT, static_cast<int64_t>(assignment.getID()))
        }
      };

      // propose the extension
      std::unique_ptr<Proposal> proposal = std::unique_ptr<Proposal>(ProposalFactory::Factory(*this, daoID, common::EXTENSION));
      proposal->propose(assignee, contentGroups, false);
   }

   void dao::withdraw(name owner, uint64_t document_id)
   {
     TRACE_FUNCTION()
     EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

     Assignment assignment(this, document_id);

     eosio::name assignee = assignment.getAssignee().getAccount();

     EOS_CHECK(
       assignee == owner,
       util::to_str("Only the member [", assignee.to_string() ,"] can withdraw the assignment [", document_id, "]")
     );

     eosio::require_auth(owner);

     auto cw = assignment.getContentWrapper();

     auto state = cw.getOrFail(DETAILS, common::STATE)->getAs<string>();

     EOS_CHECK(
       state == common::STATE_APPROVED,
       util::to_str("Cannot withdraw ", state, " assignments")
     )

     auto originalPeriods = assignment.getPeriodCount();

     auto startPeriod = assignment.getStartPeriod();

     auto now = eosio::current_time_point();

     auto periodsToCurrent = int64_t(0);

     //If the start period is in the future then we just set the period count to 0
     //since it means that we are withdrawing before the start period
     if (now > startPeriod.getStartTime()) {
        //Calculate the number of periods since start period to the current period
        auto currentPeriod = startPeriod.getPeriodUntil(now);

        periodsToCurrent = startPeriod.getPeriodCountTo(currentPeriod);

        periodsToCurrent = std::max(periodsToCurrent, int64_t(0)) + 1;

        EOS_CHECK(
          originalPeriods >= periodsToCurrent,
          util::to_str("Withdrawal of expired assignment: ", document_id, " is not allowed")
        );

        modifyCommitment(assignment, 0, std::nullopt, common::MOD_WITHDRAW);
     }

     auto detailsGroup = cw.getGroupOrFail(DETAILS);

     ContentWrapper::insertOrReplace(*detailsGroup, Content { PERIOD_COUNT, periodsToCurrent });

     ContentWrapper::insertOrReplace(*detailsGroup, Content { common::STATE, common::STATE_WITHDRAWED });

     assignment.update();
   }

   void dao::suspend(name proposer, uint64_t document_id, string reason)
   {
     TRACE_FUNCTION()
     EOS_CHECK(
       !isPaused(),
       "Contract is paused for maintenance. Please try again later."
     );

     //Get the DAO edge from the hash
     auto daoID = Edge::get(get_self(), document_id, common::DAO).getToNode();

     EOS_CHECK(
       Member::isMember(*this, daoID, proposer),
       util::to_str("Only members are allowed to propose suspensions")
     );

     eosio::require_auth(proposer);

     ContentGroups cgs = {
       ContentGroup {
          Content { CONTENT_GROUP_LABEL, DETAILS },
          Content { DESCRIPTION, std::move(reason) },
          Content { ORIGINAL_DOCUMENT, static_cast<int64_t>(document_id) },
       },
     };

     propose(daoID, proposer, common::SUSPEND, cgs, true);
   }

   void dao::claimnextper(uint64_t assignment_id)
   {
      TRACE_FUNCTION()

      EOS_CHECK(
        !isPaused(),
        "Contract is paused for maintenance. Please try again later."
      );

      Assignment assignment(this, assignment_id);

      eosio::name assignee = assignment.getAssignee().getAccount();

      // assignee must still be a DHO member
      auto daoID = Edge::get(get_self(), assignment.getID(), common::DAO).getToNode();

      EOS_CHECK(
        Member::isMember(*this, daoID, assignee),
        "assignee must be a current member to claim pay: " + assignee.to_string()
      );

      std::optional<Period> periodToClaim = assignment.getNextClaimablePeriod();
      EOS_CHECK(periodToClaim != std::nullopt, util::to_str("All available periods for this assignment have been claimed: ", assignment_id));

      // require_auth(assignee);
      EOS_CHECK(has_auth(assignee) || has_auth(get_self()), "only assignee or " + get_self().to_string() + " can claim pay");

      // Valid claim identified - start process
      // process this claim
      Edge::write(get_self(), get_self(), assignment.getID(), periodToClaim.value().getID(), common::CLAIMED);
      int64_t periodStartSec = periodToClaim.value().getStartTime().sec_since_epoch();
      int64_t periodEndSec = periodToClaim.value().getEndTime().sec_since_epoch();

      // Pro-rate the payment if the assignment was created during the period being claimed
      //float first_phase_ratio_calc = 1.f; // pro-rate based on elapsed % of the first phase

      //EOS_CHECK(first_phase_ratio_calc <= 1, "fatal error: first_phase_ratio_calc is greater than 1: " + std::to_string(first_phase_ratio_calc));

      auto daoSettings = getSettingsDocument(daoID);

      auto daoTokens = AssetBatch{
        .reward = daoSettings->getOrFail<asset>(common::REWARD_TOKEN),
        .peg = daoSettings->getOrFail<asset>(common::PEG_TOKEN),
        .voice = daoSettings->getOrFail<asset>(common::VOICE_TOKEN)
      };

      const asset pegSalary = assignment.getPegSalary();
      const asset voiceSalary = assignment.getVoiceSalary();
      const asset rewardSalary = assignment.getRewardSalary();

      asset deferredSeeds;
      asset peg;
      asset voice;
      asset reward;

      {
         const int64_t initTimeShare = assignment.getInitialTimeShare()
                                           .getContentWrapper()
                                           .getOrFail(DETAILS, TIME_SHARE)
                                           ->getAs<int64_t>();

         TimeShare current = assignment.getCurrentTimeShare();

         //Initialize nextOpt with current in order to have a valid initial timeShare
         auto nextOpt = std::optional<TimeShare>{current};

         int64_t currentTimeSec = periodStartSec;

         std::optional<TimeShare> lastUsedTimeShare;

         while (nextOpt)
         {

            ContentWrapper nextWrapper = nextOpt->getContentWrapper();

            const int64_t timeShare = nextWrapper.getOrFail(DETAILS, TIME_SHARE)->getAs<int64_t>();
            const int64_t startDateSec = nextWrapper.getOrFail(DETAILS, TIME_SHARE_START_DATE)->getAs<time_point>().sec_since_epoch();

            //If the time share doesn't belong to the claim period we
            //finish pro-rating
            if (periodEndSec - startDateSec <= 0)
            {
               break;
            }

            int64_t remainingTimeSec;

            lastUsedTimeShare = std::move(nextOpt.value());

            //It's possible that time share was set on previous periods,
            //if so we should use period start date as the base date
            const int64_t baseDateSec = std::max(periodStartSec, startDateSec);

            //Check if there is another timeshare
            if ((nextOpt = lastUsedTimeShare->getNext(get_self())))
            {
               const time_point commingStartDate = nextOpt->getContentWrapper().getOrFail(DETAILS, TIME_SHARE_START_DATE)->getAs<time_point>();
               const int64_t commingStartDateSec = commingStartDate.sec_since_epoch();
               //Check if the time share belongs to the same peroid
               if (commingStartDateSec >= periodEndSec)
               {
                  remainingTimeSec = periodEndSec - baseDateSec;
               }
               else
               {
                  remainingTimeSec = commingStartDateSec - baseDateSec;
               }
            }
            else
            {
               remainingTimeSec = periodEndSec - baseDateSec;
            }

            const int64_t fullPeriodSec = periodEndSec - periodStartSec;

            //Time share could only represent a portion of the whole period
            float relativeDuration = static_cast<float>(remainingTimeSec) / static_cast<float>(fullPeriodSec);
            float relativeCommitment = static_cast<float>(timeShare) / static_cast<float>(initTimeShare);
            float commitmentMultiplier = relativeDuration * relativeCommitment;

            //Accumlate each of the currencies with the time share multiplier
            //  deferredSeeds = (deferredSeeds.is_valid() ? deferredSeeds : eosio::asset{0, common::S_SEEDS}) +
            //  adjustAsset(assignment.getSalaryAmount(&common::S_SEEDS, &periodToClaim.value()), first_phase_ratio_calc * commitmentMultiplier);

            peg = (peg.is_valid() ? peg : eosio::asset{0, daoTokens.peg.symbol}) +
                   adjustAsset(pegSalary, commitmentMultiplier);

            voice = (voice.is_valid() ? voice : eosio::asset{0, daoTokens.voice.symbol}) +
                     adjustAsset(voiceSalary, commitmentMultiplier);

            reward = (reward.is_valid() ? reward : eosio::asset{0, daoTokens.reward.symbol}) +
                    adjustAsset(rewardSalary, commitmentMultiplier);
         }

         //If the last used time share is different from current time share
         //let's update the edge
         if (lastUsedTimeShare->getID() != current.getID())
         {
            Edge::get(get_self(), assignment.getID(), common::CURRENT_TIME_SHARE).erase();
            Edge::write(get_self(), get_self(), assignment.getID(), lastUsedTimeShare->getID(), common::CURRENT_TIME_SHARE);
         }
      }

      // EOS_CHECK(deferredSeeds.is_valid(), "fatal error: SEEDS has to be a valid asset");
      EOS_CHECK(peg.is_valid(), "fatal error: PEG has to be a valid asset");
      EOS_CHECK(voice.is_valid(), "fatal error: VOICE has to be a valid asset");
      EOS_CHECK(reward.is_valid(), "fatal error: REWARD has to be a valid asset");

      string assignmentNodeLabel = "";
      if (auto [idx, assignmentLabel] = assignment.getContentWrapper().get(SYSTEM, NODE_LABEL); assignmentLabel)
      {
         EOS_CHECK(std::holds_alternative<std::string>(assignmentLabel->value), "fatal error: assignment content item type is expected to be a string: " + assignmentLabel->label);
         assignmentNodeLabel = std::get<std::string>(assignmentLabel->value);
      }

      //If node_label is not present for any reason fallback to the assignment hash
      if (assignmentNodeLabel.empty()) {
        assignmentNodeLabel = util::to_str(assignment.getID());
      }

      string memo = assignmentNodeLabel + ", period: " + periodToClaim.value().getNodeLabel();

      // creating a single struct improves performance for table queries here
      AssetBatch ab{};
      ab.reward = reward;
      ab.voice = voice;
      ab.peg = peg;

      ab = applyBadgeCoefficients(periodToClaim.value(), assignee, daoID, ab);

      makePayment(daoSettings, periodToClaim.value().getID(), assignee, ab.reward, memo, eosio::name{0}, daoTokens);
      makePayment(daoSettings, periodToClaim.value().getID(), assignee, ab.voice, memo, eosio::name{0}, daoTokens);
      makePayment(daoSettings, periodToClaim.value().getID(), assignee, ab.peg, memo, eosio::name{0}, daoTokens);
   }

   asset dao::getProRatedAsset(ContentWrapper *assignment, const symbol &symbol, const string &key, const float &proration)
   {
      TRACE_FUNCTION()
      asset assetToPay = asset{0, symbol};
      if (auto [idx, assetContent] = assignment->get(DETAILS, key); assetContent)
      {
         EOS_CHECK(std::holds_alternative<eosio::asset>(assetContent->value), "fatal error: expected token type must be an asset value type: " + assetContent->label);
         assetToPay = std::get<eosio::asset>(assetContent->value);
      }
      return adjustAsset(assetToPay, proration);
   }

   std::vector<Document> dao::getCurrentBadges(Period &period, const name &member, uint64_t dao)
   {
      TRACE_FUNCTION()
      std::vector<Document> current_badges;

      Member memDoc(*this, getMemberID(member));

      std::vector<Edge> badge_assignment_edges = m_documentGraph.getEdgesFrom(memDoc.getID(), common::ASSIGN_BADGE);
      for (const Edge& e : badge_assignment_edges)
      {
        Document badgeAssignmentDoc(get_self(), e.to_node);
        Edge badge_edge = Edge::get(get_self(), badgeAssignmentDoc.getID(), common::BADGE_NAME);

        //Verify badge still exists
        EOS_CHECK(
          Document::exists(get_self(), badge_edge.getToNode()),
          util::to_str("Badge document doesn't exits for badge assignment:",
                 badgeAssignmentDoc.getID(),
                 " badge:", badge_edge.getToNode())
        )

        //Verify the badge is actually from the requested DAO
        if (Edge::get(get_self(), e.to_node, common::DAO).getToNode() != dao) {
          continue;
        }

        auto badgeAssignment = badgeAssignmentDoc.getContentWrapper();

        //Check if badge assignment is old (it contains end_period)
        if (badgeAssignment.exists(DETAILS, END_PERIOD)) {
          continue;
        }

        //Verify Badge document exists
        Document badge(get_self(), badge_edge.getToNode());

        Content* startPeriodContent = badgeAssignment.getOrFail(DETAILS, START_PERIOD);

        Period startPeriod(
          this,
          static_cast<uint64_t>(startPeriodContent->getAs<int64_t>())
        );
        int64_t periodCount = badgeAssignment.getOrFail(DETAILS, PERIOD_COUNT)->getAs<int64_t>();
        auto endPeriod = startPeriod.getNthPeriodAfter(periodCount);

        int64_t badgeAssignmentStart = startPeriod.getStartTime().sec_since_epoch();
        int64_t badgeAssignmentExpiration = endPeriod.getStartTime().sec_since_epoch();

        auto periodStartSecs = period.getStartTime().sec_since_epoch();
        //Badge expiration should be compared against the claimed period instead
        //of the current time, point since it could happen that the assignment is
        //beign claimed after the badge assignment expired.
        if (badgeAssignmentStart <= periodStartSecs &&
            periodStartSecs < badgeAssignmentExpiration) {
          current_badges.push_back(badge);
        }
      }

      return current_badges;
   }

   eosio::asset dao::applyCoefficient(ContentWrapper &badge, const eosio::asset &base, const std::string &key)
   {
      TRACE_FUNCTION()
      if (auto [idx, coefficient] = badge.get(DETAILS, key); coefficient)
      {
         if (std::holds_alternative<std::monostate>(coefficient->value))
         {
            return asset{0, base.symbol};
         }

         EOS_CHECK(std::holds_alternative<int64_t>(coefficient->value), "fatal error: coefficient must be an int64_t type: key: " + key);

         float coeff_float = (float)((float)std::get<int64_t>(coefficient->value) / (float)10000);
         float adjustment = (float)coeff_float - (float)1;
         return adjustAsset(base, adjustment);
      }
      return asset{0, base.symbol};
   }

   AssetBatch dao::applyBadgeCoefficients(Period &period, const eosio::name &member, uint64_t dao, AssetBatch &ab)
   {
      TRACE_FUNCTION()
      // get list of badges
      auto badges = getCurrentBadges(period, member, dao);
      AssetBatch applied_assets = ab;

      // for each badge, apply appropriate coefficients
      for (auto &badge : badges)
      {
         ContentWrapper badgeCW = badge.getContentWrapper();
         applied_assets.reward = applied_assets.reward + applyCoefficient(badgeCW, ab.reward, common::REWARD_COEFFICIENT);
         applied_assets.peg = applied_assets.peg + applyCoefficient(badgeCW, ab.peg, common::PEG_COEFFICIENT);
         applied_assets.voice = applied_assets.voice + applyCoefficient(badgeCW, ab.voice, common::VOICE_COEFFICIENT);
      }

      return applied_assets;
   }

   void dao::makePayment(Settings* daoSettings,
                         uint64_t fromNode,
                         const eosio::name &recipient,
                         const eosio::asset &quantity,
                         const string &memo,
                         const eosio::name &paymentType,
                         const AssetBatch& daoTokens)
   {
      TRACE_FUNCTION()
      // nothing to do if quantity is zero of symbol is USD, a known placeholder
      if (quantity.amount == 0 || quantity.symbol == common::S_USD)
      {
         return;
      }

      std::unique_ptr<Payer> payer = std::unique_ptr<Payer>(PayerFactory::Factory(*this, daoSettings, quantity.symbol, paymentType, daoTokens));
      Document paymentReceipt = payer->pay(recipient, quantity, memo);
      Document recipientDoc(get_self(), getMemberID(recipient));
      Edge::write(get_self(), get_self(), fromNode, paymentReceipt.getID(), common::PAYMENT);
      Edge::write(get_self(), get_self(), recipientDoc.getID(), paymentReceipt.getID(), common::PAID);
   }

   void dao::apply(const eosio::name &applicant, uint64_t dao_id, const std::string &content)
   {
      TRACE_FUNCTION()
      require_auth(applicant);

      auto member = getOrCreateMember(applicant);

      member.apply(dao_id, content);
   }

   void dao::enroll(const eosio::name &enroller, uint64_t dao_id, const eosio::name &applicant, const std::string &content)
   {
      TRACE_FUNCTION()

      require_auth(enroller);

      checkEnrollerAuth(dao_id, enroller);

      auto memberID = getMemberID(applicant);

      Member member = Member(*this, memberID);
      member.enroll(enroller, dao_id, content);
   }

   bool dao::isPaused() {
     return getSettingsDocument()->getSettingOrDefault<int64_t>("paused", 0) == 1;
   }

   Settings* dao::getSettingsDocument(uint64_t daoID)
   {
     TRACE_FUNCTION();

     //Check if it'S already loaded in cache
     for (auto& settingsDoc : m_settingsDocs) {
       if (settingsDoc->getRootID() == daoID) {
         return settingsDoc.get();
       }
     }

     //If not then we have to load it
     auto edges = m_documentGraph.getEdgesFromOrFail(daoID, common::SETTINGS_EDGE);
     EOS_CHECK(edges.size() == 1, "There should only exists only 1 settings edge from a dao node");

     m_settingsDocs.emplace_back(std::make_unique<Settings>(
       *this,
       edges[0].to_node,
       daoID
     ));

     return m_settingsDocs.back().get();
   }

   Settings* dao::getSettingsDocument()
   {
      TRACE_FUNCTION()

      return getSettingsDocument(getRootID());
   }

   void dao::setsetting(const string &key, const Content::FlexValue &value, std::optional<std::string> group)
   {
      TRACE_FUNCTION()
      require_auth(get_self());
      auto settings = getSettingsDocument();

      settings->setSetting(group.value_or(string{"settings"}), Content{key, value});
   }

   void dao::setdaosetting(const uint64_t& dao_id, std::map<std::string, Content::FlexValue> kvs, std::optional<std::string> group)
   {
     TRACE_FUNCTION()
     EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

     checkAdminsAuth(dao_id);
     auto settings = getSettingsDocument(dao_id);
     //Only hypha dao should be able to use this flag
     EOS_CHECK(
       kvs.count("is_hypha") == 0 ||
       settings->getOrFail<eosio::name>(DAO_NAME) ==  "hypha"_n,
       "Only hypha dao is allowed to add this setting"
     )

     std::string groupName = group.value_or(std::string{"settings"});

     //Fixed settings that cannot be changed
     std::map<std::string, const std::vector<std::string>> fixedSettings = {
       {
         "settings",
         {
          common::REWARD_TOKEN,
          common::VOICE_TOKEN,
          common::PEG_TOKEN,
          common::PERIOD_DURATION,
          common::VOICE_TOKEN_DECAY_PERIOD,
          common::VOICE_TOKEN_DECAY_PER_PERIOD,
          DAO_NAME
         }
       }
     };
     
     //If groupName doesn't exists in fixedSettings it will just create an empty array
     for (auto& fs : fixedSettings[groupName]) {
       EOS_CHECK(
         kvs.count(fs) == 0,
         util::to_str(fs, " setting cannot be modified in group: ", groupName)
       )
     }

     settings->setSettings(groupName, kvs);
   }

  //  void dao::adddaosetting(const uint64_t& dao_id, const std::string &key, const Content::FlexValue &value, std::optional<std::string> group)
  //  {
  //    TRACE_FUNCTION()
  //    checkAdminsAuth(dao_id);
  //    auto settings = getSettingsDocument(dao_id);
  //    //Only hypha dao should be able to use this flag
  //    EOS_CHECK(
  //      key != "is_hypha" ||
  //      settings->getOrFail<eosio::name>(DAO_NAME) ==  "hypha"_n,
  //      "Only hypha dao is allowed to add this setting"
  //    )
  //    settings->addSetting(group.value_or(string{"settings"}), Content{key, value});
  //  }

   void dao::remdaosetting(const uint64_t& dao_id, const std::string &key, std::optional<std::string> group)
   {
     TRACE_FUNCTION()
     EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

     checkAdminsAuth(dao_id);
     auto settings = getSettingsDocument(dao_id);
     settings->remSetting(group.value_or(string{"settings"}), key);
   }

   void dao::remkvdaoset(const uint64_t& dao_id, const std::string &key, const Content::FlexValue &value, std::optional<std::string> group)
   {
     TRACE_FUNCTION()
     EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");
     checkAdminsAuth(dao_id);
     auto settings = getSettingsDocument(dao_id);
     settings->remKVSetting(group.value_or(string{"settings"}), Content{ key, value });
   }

   void dao::remsetting(const string &key)
   {
      TRACE_FUNCTION()
      EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");
      require_auth(get_self());
      auto settings = getSettingsDocument();
      settings->remSetting(key);
   }

   void  dao::addenroller(const uint64_t dao_id, name enroller_account)
   {
     TRACE_FUNCTION()
     EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

     checkAdminsAuth(dao_id);
     auto mem = getOrCreateMember(enroller_account);
     if (!Member::isMember(*this, dao_id, enroller_account)) {
        if (!Edge::exists(get_self(), dao_id, mem.getID(), common::APPLICANT)) {
          mem.apply(dao_id, "Auto enrolled member");
        }
        mem.enroll(get_self(), dao_id, "Auto enrolled member");
     }
     Edge::getOrNew(get_self(), get_self(), dao_id, getMemberID(enroller_account), common::ENROLLER);
   }
   void dao::addadmin(const uint64_t dao_id, name admin_account)
   {
     TRACE_FUNCTION()
     EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

     checkAdminsAuth(dao_id);
     auto mem = getOrCreateMember(admin_account);
     if (!Member::isMember(*this, dao_id, admin_account)) {
        if (!Edge::exists(get_self(), dao_id, mem.getID(), common::APPLICANT)) {
          mem.apply(dao_id, "Auto enrolled member");
        }
        mem.enroll(get_self(), dao_id, "Auto enrolled member");
     }
     Edge::getOrNew(get_self(), get_self(), dao_id, getMemberID(admin_account), common::ADMIN);
   }
   void dao::remenroller(const uint64_t dao_id, name enroller_account)
   {
     TRACE_FUNCTION()
     EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

     checkAdminsAuth(dao_id);

     EOS_CHECK(
       m_documentGraph.getEdgesFrom(dao_id, common::ENROLLER).size() > 1,
       "Cannot remove enroller, there has to be at least 1"
     )

     Edge::get(get_self(), dao_id, getMemberID(enroller_account), common::ENROLLER).erase();
   }
   void dao::remadmin(const uint64_t dao_id, name admin_account)
   {
     TRACE_FUNCTION()
     EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");
     checkAdminsAuth(dao_id);

     EOS_CHECK(
       m_documentGraph.getEdgesFrom(dao_id, common::ADMIN).size() > 1,
       "Cannot remove admin, there has to be at least 1"
     )

     Edge::get(get_self(), dao_id, getMemberID(admin_account), common::ADMIN).erase();
   }

   ACTION dao::genperiods(uint64_t dao_id, int64_t period_count/*, int64_t period_duration_sec*/)
   {
      TRACE_FUNCTION()
      EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

      if (!eosio::has_auth(get_self())) {
        checkAdminsAuth(dao_id);
      }

      genPeriods(dao_id, period_count);
   }

   void dao::createdao(ContentGroups &config)
   {
      TRACE_FUNCTION()
      EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

      //Extract mandatory configurations
      auto configCW = ContentWrapper(config);

      auto [detailsIdx, _] = configCW.getGroup(DETAILS);

      EOS_CHECK(
        detailsIdx != -1,
        util::to_str("Missing Details Group")
      )

      auto daoName = configCW.getOrFail(detailsIdx, DAO_NAME).second;

      const name dao = daoName->getAs<name>();

      //This is a reserved name for the root document
      EOS_CHECK(
        dao != common::DHO_ROOT_NAME,
        util::to_str(dao, " is a reserved name and can't be used to create a DAO.")
      )

      Document daoDoc(get_self(), get_self(), getDAOContent(daoName->getAs<name>()));

      addNameID<dao_table>(dao, daoDoc.getID());

      //Verify Root exists
      Document root = Document(get_self(), getRootID());

      Edge(get_self(), get_self(), root.getID(), daoDoc.getID(), common::DAO);

      const std::map<std::string, std::vector<Content>> mandatoryFields = {
        { 
          "details", 
          {
            {"dao_name", Content::FlexValue{}},
            {"dao_title", Content::FlexValue{}}
          }
        }
      };

      auto votingDurationSeconds = configCW.getOrFail(detailsIdx, VOTING_DURATION_SEC).second;

      EOS_CHECK(
        votingDurationSeconds->getAs<int64_t>() > 0,
        util::to_str(VOTING_DURATION_SEC, " has to be a positive number")
      )

      auto daoDescription = configCW.getOrFail(detailsIdx, common::DAO_DESCRIPTION).second;

      EOS_CHECK(
        daoDescription->getAs<std::string>().size() <= 512,
        "Dao description has be less than 512 characters"
      )

      auto daoTitle = configCW.getOrFail(detailsIdx, common::DAO_TITLE).second;

      EOS_CHECK(
        daoTitle->getAs<std::string>().size() <= 48,
        "Dao title has be less than 48 characters"
      )

      auto pegToken = configCW.getOrFail(detailsIdx, common::PEG_TOKEN).second;

      auto voiceToken = configCW.getOrFail(detailsIdx, common::VOICE_TOKEN).second;
      auto voiceTokenDecayPeriod = configCW.getOrFail(detailsIdx, common::VOICE_TOKEN_DECAY_PERIOD).second;
      auto voiceTokenDecayPerPeriodX10M = configCW.getOrFail(detailsIdx, common::VOICE_TOKEN_DECAY_PER_PERIOD).second;

      auto rewardToken = configCW.getOrFail(detailsIdx, common::REWARD_TOKEN).second;

      auto rewardToPegTokenRatio = configCW.getOrFail(detailsIdx, common::REWARD_TO_PEG_RATIO).second;

      rewardToPegTokenRatio->getAs<asset>();

      //Generate periods
      //auto inititialPeriods = configCW.getOrFail(detailsIdx, PERIOD_COUNT);

      auto periodDurationSeconds = configCW.getOrFail(detailsIdx, common::PERIOD_DURATION).second;

      EOS_CHECK(
        periodDurationSeconds->getAs<int64_t>() > 0,
        util::to_str(common::PERIOD_DURATION, " has to be a positive number")
      )

      auto onboarderAcc = configCW.getOrFail(detailsIdx, common::ONBOARDER_ACCOUNT).second;

      const name onboarder = onboarderAcc->getAs<name>();

      auto votingQuorum = configCW.getOrFail(detailsIdx, VOTING_QUORUM_FACTOR_X100).second;

      votingQuorum->getAs<int64_t>();

      auto votingAllignment = configCW.getOrFail(detailsIdx, VOTING_ALIGNMENT_FACTOR_X100).second;

      votingAllignment->getAs<int64_t>();

      int64_t useSeeds = 0;

      if (auto [_, daoUsesSeeds] = configCW.get(detailsIdx, common::DAO_USES_SEEDS);
          daoUsesSeeds)
      {
        useSeeds = daoUsesSeeds->getAs<int64_t>();
      }

      require_auth(onboarder);

      // Create the settings document as well and add an edge to it
      ContentGroups settingCgs{
          ContentGroup{
              Content(CONTENT_GROUP_LABEL, SETTINGS),
              *daoName,
              *daoTitle,
              *daoDescription,
              *pegToken,
              *voiceToken,
              *rewardToken,
              *rewardToPegTokenRatio,
              *periodDurationSeconds,
              *votingDurationSeconds,
              *onboarderAcc,
              *votingQuorum,
              *votingAllignment,
              Content{common::DAO_USES_SEEDS, useSeeds}
          },
          ContentGroup{
              Content(CONTENT_GROUP_LABEL, SYSTEM),
              Content(TYPE, common::SETTINGS_EDGE),
              Content(NODE_LABEL, "Settings")
          }
      };

      Document settingsDoc(get_self(), get_self(), std::move(settingCgs));
      Edge::write(get_self(), get_self(), daoDoc.getID(), settingsDoc.getID(), common::SETTINGS_EDGE);

      createVoiceToken(
          dao,
          voiceToken->getAs<asset>(),
          voiceTokenDecayPeriod->getAs<int64_t>(),
          voiceTokenDecayPerPeriodX10M->getAs<int64_t>()
      );
      
      auto dhoSettings = getSettingsDocument();

      //Check if we should skip creating reward & peg tokens
      //this might be the case if the tokens already exists
      
      if (auto [idx, skipPegTokFlag] = configCW.get(DETAILS, common::SKIP_PEG_TOKEN_CREATION); 
          skipPegTokFlag == nullptr || 
          skipPegTokFlag->getAs<int64_t>() == 0) {
        createToken(
          PEG_TOKEN_CONTRACT, 
          dhoSettings->getOrFail<eosio::name>(TREASURY_CONTRACT),
          pegToken->getAs<eosio::asset>()
        );
      }
      else {
        //Only dao.hypha should be able skip creating reward or peg token
        eosio::require_auth(get_self());
      }

      if (auto [idx, skipRewardTokFlag] = configCW.get(DETAILS, common::SKIP_REWARD_TOKEN_CREATION); 
          skipRewardTokFlag == nullptr || 
          skipRewardTokFlag->getAs<int64_t>() == 0) {
        createToken(
          REWARD_TOKEN_CONTRACT, 
          get_self(),
          rewardToken->getAs<eosio::asset>()
        );
      }
      else {
        //Only dao.hypha should be able skip creating reward or peg token
        eosio::require_auth(get_self());
      }

      std::set<eosio::name> coreMemNames = { onboarder };

      if (auto coreMemsGroup = configCW.getGroupOrFail("core_members");
          coreMemsGroup) {
        EOS_CHECK(
          coreMemsGroup->size() > 1 &&
          coreMemsGroup->at(0).label == CONTENT_GROUP_LABEL,
          util::to_str("Wrong format for core groups\n"
                       "[min size: 2, got: ", coreMemsGroup->size(), "]\n",
                       "[first item label: ", CONTENT_GROUP_LABEL, " got: ", coreMemsGroup->at(0).label)
        );

        //Skip content_group label (index 0)
        std::transform(coreMemsGroup->begin() + 1, 
                       coreMemsGroup->end(), 
                       std::inserter(coreMemNames, coreMemNames.begin()),
                       [](const Content& c){ return c.getAs<name>(); });
      }

      for (auto& coreMem : coreMemNames) {
        std::unique_ptr<Member> member;

        if (Member::exists(*this, coreMem)) {
          member = std::make_unique<Member>(*this, getMemberID(coreMem));
        }
        else {
          member = std::make_unique<Member>(*this, onboarder, coreMem);
        }

        member->apply(daoDoc.getID(), "DAO Core member");
        member->enroll(onboarder, daoDoc.getID(), "DAO Core member");

        //Create owner, admin and enroller edges
        Edge(get_self(), get_self(), daoDoc.getID(), member->getID(), common::ENROLLER);
        Edge(get_self(), get_self(), daoDoc.getID(), member->getID(), common::ADMIN);
      }

      //Create start period
      Period newPeriod(this, eosio::current_time_point(), util::to_str(dao, " start period"));

      //Create start & end edges
      Edge(get_self(), get_self(), daoDoc.getID(), newPeriod.getID(), common::START);
      Edge(get_self(), get_self(), daoDoc.getID(), newPeriod.getID(), common::END);
      Edge(get_self(), get_self(), daoDoc.getID(), newPeriod.getID(), common::PERIOD);
      Edge(get_self(), get_self(), newPeriod.getID(), daoDoc.getID(), common::DAO);

      // EOS_CHECK(
      //   inititialPeriods->getAs<int64_t>() - 1 <= 30,
      //   util::to_str("The max number of initial periods is 30")
      // )
   }

   void dao::archiverecur(uint64_t document_id)
   {
      /**
      * Notes
      * Watch for:
      * - Suspended documents
      * - Withdrawed documents
      * - Documents about to be suspended
      */

      RecurringActivity recurAct(this, document_id);

      bool userCalled = eosio::has_auth(recurAct.getAssignee().getAccount());

      if (!userCalled) {
        require_auth(get_self());
      }

      auto expirationDate = recurAct.getLastPeriod().getEndTime();

      auto cw = recurAct.getContentWrapper();

      auto state = cw.getOrFail(DETAILS, common::STATE)
                      ->getAs<std::string>();

      if (state == common::STATE_APPROVED) {
        if (expirationDate < eosio::current_time_point()) {

          auto details = cw.getGroupOrFail(DETAILS);

          cw.insertOrReplace(
            *details,
            Content {
              common::STATE,
              common::STATE_ARCHIVED
            }
          );

          recurAct.update();
        }
        //It could happen if the original recurring activity was extended}
        //so reschedule with the new end period
        //Just re-schedule if the action was called by an scheduled transaction
        else if (!userCalled) {
          recurAct.scheduleArchive();
        }
      }
   }

   void dao::createroot(const std::string &notes)
   {
      TRACE_FUNCTION()
      require_auth(get_self());

      Document rootDoc(get_self(), get_self(), getRootContent(get_self()));

      // Create the settings document as well and add an edge to it
      ContentGroups settingCgs{
          ContentGroup{
              Content(CONTENT_GROUP_LABEL, SETTINGS),
              Content(ROOT_NODE, util::to_str(rootDoc.getID()))},
          ContentGroup{
              Content(CONTENT_GROUP_LABEL, SYSTEM),
              Content(TYPE, common::SETTINGS_EDGE),
              Content(NODE_LABEL, "Settings")}};

      Document settingsDoc(get_self(), get_self(), std::move(settingCgs));
      Edge::write(get_self(), get_self(), rootDoc.getID(), settingsDoc.getID(), common::SETTINGS_EDGE);

      addNameID<dao_table>(common::DHO_ROOT_NAME, rootDoc.getID());
   }

   void dao::setalert(const eosio::name &level, const std::string &content)
   {
      TRACE_FUNCTION()

      EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");


      Document rootDoc(get_self(), getRootID());

      auto [exists, edge] = Edge::getIfExists(get_self(), rootDoc.getID(), common::ALERT);
      if (exists)
      {
         remalert("removing alert to replace with new alert");
      }

      Document alert(get_self(), get_self(),
                     ContentGroups{
                         ContentGroup{
                             Content(CONTENT_GROUP_LABEL, DETAILS),
                             Content(LEVEL, level),
                             Content(CONTENT, content)},
                         ContentGroup{
                             Content(CONTENT_GROUP_LABEL, SYSTEM),
                             Content(TYPE, common::ALERT),
                             Content(NODE_LABEL, "Alert : " + level.to_string())}});

      Edge::write(get_self(), get_self(), rootDoc.getID(), alert.getID(), common::ALERT);
   }

   void dao::remalert(const string &notes)
   {
      TRACE_FUNCTION()
      EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

      Document rootDoc(get_self(), getRootID());

      Edge alertEdge = Edge::get(get_self(), rootDoc.getID(), common::ALERT);
      Document alert(get_self(), alertEdge.getToNode());
      getGraph().eraseDocument(alert.getID());
   }

   DocumentGraph &dao::getGraph()
   {
      return m_documentGraph;
   }

   /**
  * Info Structure
  *
  * ContentGroups
  * [
  *   Group Assignment 0 Details: [
  *     assignment: [int64_t]
  *     new_time_share_x100: [int64_t] min_time_share_x100 <= new_time_share_x100 <= time_share_x100
  *     modifier: [withdraw]
  *   ],
  *   Group Assignment 1 Details: [
  *     assignment: [int64_t]
  *     new_time_share_x100: [int64_t] min_time_share_x100 <= new_time_share_x100 <= time_share_x100
  *   ],
  *   Group Assignment N Details: [
  *     assignment: [int64_t]
  *     new_time_share_x100: [int64_t] min_time_share_x100 <= new_time_share_x100 <= time_share_x100
  *   ]
  * ]
  */
  ACTION dao::adjustcmtmnt(name issuer, ContentGroups &adjust_info)
  {
    TRACE_FUNCTION()
    EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

    require_auth(issuer);

    ContentWrapper cw(adjust_info);

    for (size_t i = 0; i < adjust_info.size(); ++i)
    {

      Assignment assignment = Assignment(
        this,
        static_cast<uint64_t>(cw.getOrFail(i, "assignment").second->getAs<int64_t>())
      );

      EOS_CHECK(assignment.getAssignee().getAccount() == issuer,
                  "Only the owner of the assignment can adjust it");

      int64_t newTimeShare = cw.getOrFail(i, NEW_TIME_SHARE).second->getAs<int64_t>();

      auto [modifierIdx, modifier] = cw.get(i, common::ADJUST_MODIFIER);

      string_view modstr = modifier ? string_view{modifier->getAs<string>()} : string_view{};

      auto [idx, startDate] = cw.get(i, TIME_SHARE_START_DATE);

      auto fixedStartDate = startDate ? std::optional<time_point>{startDate->getAs<time_point>()} : std::nullopt;

      auto assignmentCW = assignment.getContentWrapper();

      if (modstr.empty()) {

        auto lastPeriod = assignment.getLastPeriod();
        auto assignmentExpirationTime = lastPeriod.getEndTime();

        EOS_CHECK(
          assignmentExpirationTime.sec_since_epoch() > eosio::current_time_point().sec_since_epoch(),
          util::to_str("Cannot adjust expired assignment: ", assignment.getID(), " last period: ", lastPeriod.getID())
        )

        auto state = assignmentCW.getOrFail(DETAILS, common::STATE)->getAs<string>();

        EOS_CHECK(
          state == common::STATE_APPROVED,
          util::to_str("Cannot adjust commitment for ", state, " assignments")
        )
      }

      modifyCommitment(assignment,
                      newTimeShare,
                      fixedStartDate,
                      modstr);
    }
  }

  ACTION dao::adjustdeferr(name issuer, uint64_t assignment_id, int64_t new_deferred_perc_x100)
  {
    TRACE_FUNCTION()

    EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");


    require_auth(issuer);

    Assignment assignment = Assignment(this, assignment_id);

    EOS_CHECK(
      assignment.getAssignee().getAccount() == issuer,
      "Only the owner of the assignment can modify it"
    );

    auto cw = assignment.getContentWrapper();

    auto [detailsIdx, detailsGroup] = cw.getGroup(DETAILS);

    EOS_CHECK(
      detailsGroup != nullptr,
      "Missing details group from assignment"
    );

    auto state = cw.getOrFail(detailsIdx, common::STATE).second->getAs<string>();

     EOS_CHECK(
       state == common::STATE_APPROVED,
       util::to_str("Cannot change deferred percentage on", state, " assignments")
     )

    auto approvedDeferredPerc = cw.getOrFail(detailsIdx, common::APPROVED_DEFERRED)
                                  .second->getAs<int64_t>();

    EOS_CHECK(
      new_deferred_perc_x100 >= approvedDeferredPerc,
      util::to_str("New percentage has to be greater or equal to approved percentage: ", approvedDeferredPerc)
    )

    const int64_t UPPER_LIMIT = 100;

    EOS_CHECK(
      approvedDeferredPerc <= new_deferred_perc_x100 && new_deferred_perc_x100 <= UPPER_LIMIT,
      util::to_str("New percentage is out of valid range [",
                   approvedDeferredPerc, " - ", UPPER_LIMIT, "]:", new_deferred_perc_x100)
    )

    asset usdPerPeriod = cw.getOrFail(detailsIdx, USD_SALARY_PER_PERIOD)
                           .second->getAs<eosio::asset>();

    int64_t initialTimeshare = assignment.getInitialTimeShare()
                                         .getContentWrapper()
                                         .getOrFail(DETAILS, TIME_SHARE)->getAs<int64_t>();

    auto daoSettings = getSettingsDocument(assignment.getDaoID());

    auto usdPerPeriodCommitmentAdjusted = normalizeToken(usdPerPeriod) * (initialTimeshare / 100.0);

    auto deferred = new_deferred_perc_x100 / 100.0;

    auto pegVal = usdPerPeriodCommitmentAdjusted * (1.0 - deferred);
    //husdVal.symbol = common::S_PEG;

    cw.insertOrReplace(*detailsGroup, Content{
      DEFERRED,
      new_deferred_perc_x100
    });

    auto rewardToPegVal = normalizeToken(daoSettings->getOrFail<eosio::asset>(common::REWARD_TO_PEG_RATIO));

    auto rewardToken = daoSettings->getOrFail<eosio::asset>(common::REWARD_TOKEN);

    auto pegToken = daoSettings->getOrFail<eosio::asset>(common::PEG_TOKEN);

    auto rewardVal = usdPerPeriodCommitmentAdjusted * deferred / rewardToPegVal;

      cw.insertOrReplace(*detailsGroup, Content{
      common::REWARD_SALARY_PER_PERIOD,
      denormalizeToken(rewardVal, rewardToken)
    });

    cw.insertOrReplace(*detailsGroup, Content{
      common::PEG_SALARY_PER_PERIOD,
      denormalizeToken(pegVal, pegToken)
    });

    assignment.update();
  }

   void dao::modifyCommitment(RecurringActivity& assignment, int64_t commitment, std::optional<eosio::time_point> fixedStartDate, std::string_view modifier)
   {
      TRACE_FUNCTION()

      ContentWrapper assignmentCW = assignment.getContentWrapper();

      //Check min_time_share_x100 <= new_time_share_x100 <= time_share_x100
      int64_t originalTimeShare = assignmentCW.getOrFail(DETAILS, TIME_SHARE)->getAs<int64_t>();
      int64_t minTimeShare = 0;

      EOS_CHECK(
        commitment >= minTimeShare,
        util::to_str(NEW_TIME_SHARE, " must be greater than or equal to: ", minTimeShare, " You submitted: ", commitment)
      );

      EOS_CHECK(
        commitment <= originalTimeShare,
        util::to_str(NEW_TIME_SHARE, " must be less than or equal to original (approved) time_share_x100: ", originalTimeShare, " You submitted: ", commitment)
      );

      //Update lasttimeshare
      Edge lastTimeShareEdge = Edge::get(get_self(), assignment.getID(), common::LAST_TIME_SHARE);

      time_point startDate = eosio::current_time_point();

      TimeShare lastTimeShare(get_self(), lastTimeShareEdge.getToNode());

      time_point lastStartDate = lastTimeShare.getStartDate();

      if (fixedStartDate)
      {
        startDate = *fixedStartDate;
      }

      EOS_CHECK(lastStartDate.sec_since_epoch() < startDate.sec_since_epoch(),
                "New time share start date must be greater than he previous time share start date");

      int64_t lastTimeSharex100 = lastTimeShare.getContentWrapper()
                                               .getOrFail(DETAILS, TIME_SHARE)
                                               ->getAs<int64_t>();

      //This allows withdrawing/suspending an assignment which has a current commitment of 0
      if (modifier != common::MOD_WITHDRAW)
      {
        EOS_CHECK(
          lastTimeSharex100 != commitment,
          util::to_str("New commitment: [", commitment, "] must be different than current commitment: [", lastTimeSharex100, "]")
        );
      }

      TimeShare newTimeShareDoc(get_self(),
                                assignment.getAssignee().getAccount(),
                                commitment,
                                startDate,
                                assignment.getID());

      Edge::write(get_self(), get_self(), lastTimeShareEdge.getToNode(), newTimeShareDoc.getID(), common::NEXT_TIME_SHARE);

      lastTimeShareEdge.erase();

      Edge::write(get_self(), get_self(), assignment.getID(), newTimeShareDoc.getID(), common::LAST_TIME_SHARE);
      Edge::write(get_self(), get_self(), assignment.getID(), newTimeShareDoc.getID(), common::TIME_SHARE_LABEL);
   }

  uint64_t dao::getRootID()
  {
    auto rootID = getDAOID(common::DHO_ROOT_NAME);

    //Verify root entry exists
    EOS_CHECK(
      rootID.has_value(),
      util::to_str("Missing root document entry")
    )

    return *rootID;
  }

  Member dao::getOrCreateMember(const name& member)
  {
    if (Member::exists(*this, member)) {
      return Member(*this, getMemberID(member));
    }
    else {
      return Member(*this, member, member);
    }
  }

  std::optional<uint64_t> dao::getDAOID(const name& daoName)
  {
    return getNameID<dao_table>(daoName);
  }

  uint64_t dao::getMemberID(const name& memberName)
  {
    auto id = getNameID<member_table>(memberName);

    EOS_CHECK(
      id.has_value(),
      util::to_str("There is no member with name:", memberName)
    )

    return *id;
  }

  void dao::checkEnrollerAuth(uint64_t dao_id, const name& account)
  {
    TRACE_FUNCTION()

    auto memberID = getMemberID(account);

    EOS_CHECK(
      Edge::exists(get_self(), dao_id, memberID, common::ENROLLER),
      util::to_str("Only enrollers of the dao are allowed to perform this action")
    )
  }

  void dao::checkAdminsAuth(uint64_t dao_id)
  {
    TRACE_FUNCTION()

    //Contract account has admin perms over all DAO's
    if (eosio::has_auth(get_self())){
      return;
    }

    auto adminEdges = m_documentGraph.getEdgesFrom(dao_id, common::ADMIN);

    EOS_CHECK(
      std::any_of(adminEdges.begin(), adminEdges.end(), [this](const Edge& adminEdge) {
        Member member(*this, adminEdge.to_node);
        return eosio::has_auth(member.getAccount());
      }),
      util::to_str("Only admins of the dao are allowed to perform this action")
    )
  }

  void dao::genPeriods(uint64_t dao_id, int64_t period_count/*, int64_t period_duration_sec*/)
  {
    //Max number of periods that should be created in one call
    const int64_t MAX_PERIODS_PER_CALL = 30;

    //Get last period
    //Document daoDoc(get_self(), dao_id);

    auto settings = getSettingsDocument(dao_id);

    int64_t periodDurationSecs = settings->getOrFail<int64_t>(common::PERIOD_DURATION);

    name daoName = settings->getOrFail<eosio::name>(DAO_NAME);

    auto lastEdge = Edge::get(get_self(), dao_id, common::END);
    lastEdge.erase();

    uint64_t lastPeriodID = lastEdge.getToNode();

    int64_t lastPeriodStartSecs = Period(this, lastPeriodID)
                                    .getStartTime()
                                    .sec_since_epoch();

    for (int64_t i = 0; i < std::min(MAX_PERIODS_PER_CALL, period_count); ++i) {
      time_point nextPeriodStart(eosio::seconds(lastPeriodStartSecs + periodDurationSecs));

      Period nextPeriod(
        this,
        nextPeriodStart,
        util::to_str(daoName, ":", nextPeriodStart.time_since_epoch().count())
      );

      Edge(get_self(), get_self(), lastPeriodID, nextPeriod.getID(), common::NEXT);
      Edge(get_self(), get_self(), dao_id, nextPeriod.getID(), common::PERIOD);
      Edge(get_self(), get_self(), nextPeriod.getID(), dao_id, common::DAO);

      lastPeriodStartSecs = nextPeriodStart.sec_since_epoch();
      lastPeriodID = nextPeriod.getID();
    }

    Edge(get_self(), get_self(), dao_id, lastPeriodID, common::END);

    //Check if there are more periods to created

    if (period_count > MAX_PERIODS_PER_CALL) {
      eosio::action(
        eosio::permission_level(get_self(), "active"_n),
        get_self(),
        "genperiods"_n,
        std::make_tuple(dao_id,
                        period_count - MAX_PERIODS_PER_CALL)
      ).send();
    }
  }

  void dao::createVoiceToken(const eosio::name& daoName,
                             const eosio::asset& voiceToken,
                             const uint64_t& decayPeriod,
                             const uint64_t& decayPerPeriodx10M)
  {
    auto dhoSettings = getSettingsDocument();
    name governanceContract = dhoSettings->getOrFail<eosio::name>(GOVERNANCE_TOKEN_CONTRACT);

    eosio::action(
          eosio::permission_level{governanceContract, name("active")},
          governanceContract,
          name("create"),
          std::make_tuple(
            daoName,
            get_self(),
            asset{-getTokenUnit(voiceToken), voiceToken.symbol},
            decayPeriod,
            decayPerPeriodx10M
          )
        ).send();
  }

  void dao::createToken(const std::string& contractType, name issuer, const asset& token)
  {
    auto dhoSettings = getSettingsDocument();

    name contract = dhoSettings->getOrFail<eosio::name>(contractType);

    eosio::action(
      eosio::permission_level{contract, name("active")},
      contract,
      name("create"),
      std::make_tuple(
        issuer,
        asset{-getTokenUnit(token), token.symbol}
      )
    ).send();
  }

  void dao::on_husd(const name& from, const name& to, const asset& quantity, const string& memo) {
      
    EOS_CHECK(quantity.amount > 0, "quantity must be > 0");
    EOS_CHECK(quantity.is_valid(), "quantity invalid");
    
    asset hyphaUsdVal = getSettingOrFail<eosio::asset>(common::HYPHA_USD_VALUE);
    EOS_CHECK(
        hyphaUsdVal.symbol.precision() == 4,
        util::to_str("Expected hypha_usd_value precision to be 4, but got:", hyphaUsdVal.symbol.precision())
    );
    double factor = (hyphaUsdVal.amount / 10000.0);
    
    EOS_CHECK(common::S_HUSD.precision() == common::S_HYPHA.precision(), "unexpected precision mismatch");

    asset hyphaAmount = asset( quantity.amount / factor, common::S_HYPHA);

    auto hyphaID = getDAOID("hypha"_n);

    EOS_CHECK(
      hyphaID.has_value(),
      "Missing hypha DAO entry"
    )

    auto daoSettings = getSettingsDocument(*hyphaID);

    auto daoTokens = AssetBatch{
      .reward = daoSettings->getOrFail<asset>(common::REWARD_TOKEN),
      .peg = daoSettings->getOrFail<asset>(common::PEG_TOKEN),
      .voice = daoSettings->getOrFail<asset>(common::VOICE_TOKEN)
    };
    
    std::unique_ptr<Payer> payer = std::unique_ptr<Payer>(PayerFactory::Factory(*this, daoSettings, hyphaAmount.symbol, eosio::name{0}, daoTokens));
    payer->pay(from, hyphaAmount, string("Buy HYPHA for " + quantity.to_string()));
}

} // namespace hypha
