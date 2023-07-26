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

#include <badges/badges.hpp>

#include <typed_document_factory.hpp>

#include <pricing/plan_manager.hpp>
#include <pricing/common.hpp>

#include <reward/account.hpp>

namespace hypha {

/**Testenv only */

void dao::addedge(uint64_t from, uint64_t to, const name& edge_name)
{
  require_auth(get_self());

  Document fromDoc(get_self(), from);
  Document toDoc(get_self(), to);

  Edge(get_self(), get_self(), fromDoc.getID(), toDoc.getID(), edge_name);
}

void dao::editdoc(uint64_t doc_id, const std::string& group, const std::string& key, const Content::FlexValue &value)
{
  require_auth(get_self());

  Document doc(get_self(), doc_id);

  auto cw = doc.getContentWrapper();

  cw.insertOrReplace(*cw.getGroupOrFail(group), Content{key, value});

  doc.update();
}

void dao::remedge(uint64_t from_node, uint64_t to_node, name edge_name)
{
    eosio::require_auth(get_self());
    Edge::get(get_self(), from_node, to_node, edge_name).erase();
}

void dao::remdoc(uint64_t doc_id)
{
    eosio::require_auth(get_self());
    Document doc(get_self(), doc_id);
    m_documentGraph.eraseDocument(doc_id, true);
}

#ifdef DEVELOP_BUILD_HELPERS

void dao::addedge(uint64_t from, uint64_t to, const name& edge_name)
{
  require_auth(get_self());

  Document fromDoc(get_self(), from);
  Document toDoc(get_self(), to);

  Edge(get_self(), get_self(), fromDoc.getID(), toDoc.getID(), edge_name);
}

void dao::editdoc(uint64_t doc_id, const std::string& group, const std::string& key, const Content::FlexValue &value)
{
  require_auth(get_self());

  Document doc(get_self(), doc_id);

  auto cw = doc.getContentWrapper();

  cw.insertOrReplace(*cw.getGroupOrFail(group), Content{key, value});

  doc.update();
}

void dao::remedge(uint64_t from_node, uint64_t to_node, name edge_name)
{
    eosio::require_auth(get_self());
    Edge::get(get_self(), from_node, to_node, edge_name).erase();
}

void dao::adddocs(std::vector<Document>& docs)
{
  require_auth(get_self());
  Document::document_table d_t(get_self(), get_self().value);

  for (auto& doc : docs) {
    d_t.emplace(get_self(), [&doc](Document& newDoc){
      newDoc = std::move(doc);
    });
  }
}

void dao::remdoc(uint64_t doc_id)
{
    eosio::require_auth(get_self());
    Document doc(get_self(), doc_id);
    m_documentGraph.eraseDocument(doc_id, true);
}

void dao::addedges(std::vector<InputEdge>& edges)
{
  require_auth(get_self());

  Edge::edge_table e_t(get_self(), get_self().value);

  for (auto& edge : edges) {
    const int64_t edgeID = util::hashCombine(edge.from_node, edge.to_node, edge.edge_name);

    EOS_CHECK(
      e_t.find(edgeID) == e_t.end(),
      to_str("Edge from: ", edge.from_node,
                  " to: ", edge.to_node,
                  " with name: ", edge.edge_name, " already exists")
    );

    e_t.emplace(get_self(), [&](Edge &e) {

      auto& [creator, created_date, from_node, to_node, edge_name] = edge;

      e.id = edgeID;
      e.from_node_edge_name_index = util::hashCombine(from_node, edge_name);
      e.from_node_to_node_index = util::hashCombine(from_node, to_node);
      e.to_node_edge_name_index = util::hashCombine(to_node, edge_name);
      e.creator = creator;
      e.contract = get_self();
      e.from_node = from_node;
      e.to_node = to_node;
      e.edge_name = edge_name;
      e.created_date = created_date;
    });
  }
}

void dao::copybadge(uint64_t source_badge_id, uint64_t destination_dao_id, name proposer)
{
  //TODO: For system badges we should instead just share them among all existing DAO's

  if (!eosio::has_auth(proposer)) {
    eosio::require_auth(get_self());
  }
  else {
    checkAdminsAuth(destination_dao_id);
  }

  //Verify Badge type
  auto badgeDoc = TypedDocument::withType(*this, source_badge_id, common::BADGE_NAME);

  verifyDaoType(destination_dao_id);

  Document copy(get_self(), get_self(), badgeDoc.getContentGroups());

  auto badgeInfo = badges::getBadgeInfo(badgeDoc);

  EOS_CHECK(
    Member::isMember(*this, destination_dao_id, proposer),
    "Proposer must be a member of the destination DAO"
  );

  auto copyCW = copy.getContentWrapper();

  auto& cpyDaoID = copyCW.getOrFail(DETAILS, common::DAO.to_string())->getAs<int64_t>();

  EOS_CHECK(
    cpyDaoID != destination_dao_id,
    "You can only copy Badges from a different DAO"
  );

  EOS_CHECK(
    copyCW.getOrFail(DETAILS, common::STATE)
          ->getAs<string>() == common::STATE_APPROVED &&
    Edge::exists(get_self(), cpyDaoID, badgeDoc.getID(), common::PASSED_PROPS),
    "Only approved badges can be copied"
  );

  cpyDaoID = destination_dao_id;

  copy.update();

  auto memberID = getMemberID(proposer);

  Edge::write(get_self(), get_self(), copy.getID(), badgeDoc.getID(), name("copyof"));

  Edge::write(get_self(), get_self(), memberID, copy.getID(), common::OWNS);
  Edge::write(get_self(), get_self(), copy.getID(), memberID, common::OWNED_BY);
  Edge::write(get_self(), get_self(), cpyDaoID, copy.getID(), common::BADGE_NAME);
  Edge::write(get_self(), get_self(), cpyDaoID, copy.getID (), common::PASSED_PROPS);
  Edge::write(get_self(), get_self(), cpyDaoID, copy.getID(), common::VOTABLE);
  Edge::write(get_self(), get_self(), copy.getID(), cpyDaoID, common::DAO);
}

#endif

static asset getAccountBalance(const name& rewardContract, const name& account, const asset& token)
{
  accounts a_t(rewardContract, account.value);

  const auto& from = a_t.get( token.symbol.code().raw(), "no balance object found" );

  return from.balance;
}

static void updateCommunityMembership(dao& dao, const name& account, const asset& token)
{
  dao::token_to_dao_table tok_t(dao.get_self(), dao.get_self().value);

  auto entry = tok_t.get(token.symbol.raw(), "Missing token to dao entry");

  auto daoId = entry.id;

  dao.verifyDaoType(daoId);

  auto settings = dao.getSettingsDocument();

  auto rewardContract = settings->getOrFail<name>(REWARD_TOKEN_CONTRACT);

  auto balance = getAccountBalance(rewardContract, account, token);

  const auto minAmount = denormalizeToken(1.f, token);

  auto member = dao.getOrCreateMember(account);

  //TODO: Call this action on points where core membership could be removed

  if (balance >= minAmount) {
    //Let's check if we need to upgrade to community member
    if (!Member::isMember(dao, daoId, account) &&
        !Member::isCommunityMember(dao, daoId, account)) {
      //Create Community Membership
      Edge(dao.get_self(), dao.get_self(), daoId, member.getID(), common::COMMEMBER);
    }
  }
  else if (Member::isCommunityMember(dao, daoId, account)) {
    //Remove membership if we no longer meet the requirements
    Edge::get(dao.get_self(), daoId, member.getID(), common::COMMEMBER).erase();
  }
}

void dao::autoenroll(uint64_t dao_id, const name& enroller, const name& member)
{
  //require_auth(get_self());
  verifyDaoType(dao_id);

  checkAdminsAuth(dao_id);

  //Auto enroll
  auto mem = getOrCreateMember(member);

  mem.checkMembershipOrEnroll(dao_id);
}

void dao::setupdefs(uint64_t dao_id)
{
  verifyDaoType(dao_id);
  
  checkAdminsAuth(dao_id);

  auto defBadges = m_documentGraph.getEdgesFrom(getRootID(), name("defbadge"));

  for (auto& badge : defBadges) {
    //Verify it's a badge
    auto doc = TypedDocument::withType(*this, badge.getToNode(), common::BADGE_NAME);

    Edge::getOrNew(get_self(), get_self(), dao_id, doc.getID(), common::BADGE_NAME);
  }

  auto defRoles = m_documentGraph.getEdgesFrom(getRootID(), name("defrole"));

  for (auto& role : defRoles) {
    //Verify it's a badge
    auto doc = TypedDocument::withType(*this, role.getToNode(), common::ROLE_NAME);

    Edge::getOrNew(get_self(), get_self(), dao_id, doc.getID(), common::ROLE_NAME);
  }

  auto defBands = m_documentGraph.getEdgesFrom(getRootID(), name("defband"));

  for (auto& band : defBands) {
    //Verify it's a badge
    auto doc = TypedDocument::withType(*this, band.getToNode(), common::SALARY_BAND);

    Edge::getOrNew(get_self(), get_self(), dao_id, doc.getID(), common::SALARY_BAND);
  }
}

void dao::setclaimenbld(uint64_t dao_id, bool enabled)
{
  verifyDaoType(dao_id);

  checkAdminsAuth(dao_id);

  auto settings = getSettingsDocument(dao_id);

  auto currentState = settings->getSettingOrDefault<int64_t>(common::CLAIM_ENABLED, 1);

  EOS_CHECK(
    currentState != enabled,
    to_str("Claiming is already ", currentState ? "enabled" : "disabled") 
  )

  settings->setSetting({ common::CLAIM_ENABLED, static_cast<int64_t>(enabled) });
}

static void validateCircle(dao* dao, uint64_t circleID) {
  
  auto circle = TypedDocument::withType(*dao, circleID, common::CIRCLE);

  auto cw = circle.getContentWrapper();

  auto state = cw.getOrFail(DETAILS, common::STATE)->getAs<std::string>();

  EOS_CHECK(
    state == common::STATE_APPROVED,
    to_str("Cannot perform operation on ", state, " circle")
  );
}

void dao::applycircle(uint64_t circle_id, name applicant)
{
  validateCircle(this, circle_id);

  auto daoId = Edge::get(get_self(), circle_id, common::DAO).getToNode();

  //Only core members are allowed to apply as circle members
  EOS_CHECK(
    Member::isMember(*this, daoId, applicant),
    "Only core members can apply for circle membership"
  );

  eosio::require_auth(applicant);

  auto applicantId = getMemberID(applicant);

  EOS_CHECK(
    !Edge::exists(get_self(), circle_id, applicantId, common::MEMBER),
    "Applicant is member of circle already"
  );

  Edge(get_self(), get_self(), circle_id, applicantId, common::APPLICANT);
  Edge(get_self(), get_self(), applicantId, circle_id, common::APPLICANT_OF_CIRCLE);
}

void dao::rejectcircle(uint64_t circle_id, name enroller, name applicant)
{
  validateCircle(this, circle_id);

  auto daoId = Edge::get(get_self(), circle_id, common::DAO).getToNode();

  checkEnrollerAuth(daoId, enroller);

  auto memberId = getMemberID(applicant);

  Edge::get(get_self(), circle_id, memberId, common::APPLICANT).erase();
  Edge::get(get_self(), memberId, circle_id, common::APPLICANT_OF_CIRCLE).erase();
}

void dao::enrollcircle(uint64_t circle_id, name enroller, name applicant)
{
  validateCircle(this, circle_id);

  auto daoId = Edge::get(get_self(), circle_id, common::DAO).getToNode();

  checkEnrollerAuth(daoId, enroller);

  auto memberId = getMemberID(applicant);

  Edge::get(get_self(), circle_id, memberId, common::APPLICANT).erase();
  Edge::get(get_self(), memberId, circle_id, common::APPLICANT_OF_CIRCLE).erase();

  Edge(get_self(), get_self(), circle_id, memberId, common::MEMBER);
  Edge(get_self(), get_self(), memberId, circle_id, common::MEMBER_OF_CIRCLE);

  //Optimize: Move to an Edge static function and reuse in payout_proposal as well
  const auto getToID = [&](name contract, int64_t doc, name edgeName) -> std::optional<uint64_t> {
      if (auto [exists, edge] = Edge::getIfExists(contract, doc, edgeName); exists) {
          return edge.getToNode();
      }

      return std::nullopt;
  };

  auto currentID = circle_id;
  //We need to recursively add member to parent circles
  while (auto parentID = getToID(get_self(), currentID, common::PARENT_CIRCLE)) {
    //Use get or new in case it is already member of the circle
    Edge::getOrNew(get_self(), get_self(), *parentID, memberId, common::MEMBER);
    Edge::getOrNew(get_self(), get_self(), memberId, *parentID, common::MEMBER_OF_CIRCLE);
    currentID = *parentID;
  }
}

void dao::remmember(uint64_t dao_id, const std::vector<name>& member_names)
{
  verifyDaoType(dao_id);

  if (!eosio::has_auth(get_self())) {
    checkAdminsAuth(dao_id);
  }

  for (auto& member : member_names) {
    auto memberID = getMemberID(member);
    Edge::get(get_self(), dao_id, memberID, common::MEMBER).erase();
    Edge::get(get_self(), memberID, dao_id, common::MEMBER_OF).erase();
  }
}

void dao::remapplicant(uint64_t dao_id, const std::vector<name>& applicant_names)
{
  verifyDaoType(dao_id);

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
  const name& proposer,
  const name& proposal_type,
  ContentGroups& content_groups,
  bool publish)
{
  TRACE_FUNCTION();
  EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

  verifyDaoType(dao_id);

  eosio::require_auth(proposer);

  std::unique_ptr<Proposal> proposal = std::unique_ptr<Proposal>(ProposalFactory::Factory(*this, dao_id, proposal_type));
  proposal->propose(proposer, content_groups, publish);
}

void dao::vote(const name& voter, uint64_t proposal_id, string& vote, const std::optional<string>& notes)
{
  TRACE_FUNCTION();
  EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");
  Document docprop(get_self(), proposal_id);
  name proposal_type = docprop.getContentWrapper().getOrFail(SYSTEM, TYPE)->getAs<eosio::name>();

  auto daoID = Edge::get(get_self(), docprop.getID(), common::DAO).getToNode();

  Proposal* proposal = ProposalFactory::Factory(*this, daoID, proposal_type);
  proposal->vote(voter, vote, docprop, notes);
}

void dao::closedocprop(uint64_t proposal_id)
{
  TRACE_FUNCTION();
  EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

  Document docprop(get_self(), proposal_id);

  auto daoID = Edge::get(get_self(), docprop.getID(), common::DAO).getToNode();

  name proposal_type = docprop.getContentWrapper().getOrFail(SYSTEM, TYPE)->getAs<eosio::name>();

  Proposal* proposal = ProposalFactory::Factory(*this, daoID, proposal_type);
  proposal->close(docprop);
}

void dao::delasset(uint64_t asset_id)
{
  auto doc = Document(get_self(), asset_id);
  auto cw = doc.getContentWrapper();

  auto type = cw.getOrFail(SYSTEM, TYPE)->getAs<name>();

  EOS_CHECK(
    type == common::CIRCLE ||
    type == common::ROLE_NAME,
    "Invalid asset, no valid type"
  );

  EOS_CHECK(
    cw.getOrFail(SYSTEM, common::CATEGORY_SELF_APPROVED)->getAs<int64_t>() == 1,
    "Asset is not admin created"
  );

  auto daoId = cw.getOrFail(DETAILS, common::DAO.to_string())->getAs<int64_t>();

  checkAdminsAuth(static_cast<uint64_t>(daoId));

  m_documentGraph.eraseDocument(doc.getID(), true);
}

void dao::proposepub(const name& proposer, uint64_t proposal_id)
{
  TRACE_FUNCTION();
  EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

  Document docprop(get_self(), proposal_id);
  name proposal_type = docprop.getContentWrapper().getOrFail(SYSTEM, TYPE)->getAs<eosio::name>();

  auto daoID = Edge::get(get_self(), docprop.getID(), common::DAO).getToNode();

  Proposal* proposal = ProposalFactory::Factory(*this, daoID, proposal_type);
  proposal->publish(proposer, docprop);
}

void dao::proposerem(const name& proposer, uint64_t proposal_id)
{
  TRACE_FUNCTION();
  EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

  Document docprop(get_self(), proposal_id);
  name proposal_type = docprop.getContentWrapper().getOrFail(SYSTEM, TYPE)->getAs<eosio::name>();

  auto daoID = Edge::get(get_self(), docprop.getID(), common::DAO).getToNode();

  Proposal* proposal = ProposalFactory::Factory(*this, daoID, proposal_type);
  proposal->remove(proposer, docprop);
}

void dao::proposeupd(const name& proposer, uint64_t proposal_id, ContentGroups& content_groups)
{
  TRACE_FUNCTION();
  EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

  Document docprop(get_self(), proposal_id);
  name proposal_type = docprop.getContentWrapper().getOrFail(SYSTEM, TYPE)->getAs<eosio::name>();

  auto daoID = Edge::get(get_self(), docprop.getID(), common::DAO).getToNode();

  Proposal* proposal = ProposalFactory::Factory(*this, daoID, proposal_type);
  proposal->update(proposer, docprop, content_groups);
}

void dao::cmntadd(const name& author, const string content, const uint64_t comment_or_section_id)
{
  TRACE_FUNCTION();
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
  }
  else if (type == eosio::name(document_types::COMMENT_SECTION)) {
    Section parent(*this, comment_or_section_id);
    Comment(
      *this,
      parent,
      author,
      content
    );
  }
  else {
    eosio::check(false, "comment_or_section_id is no the id of a comment or a comment_section");
  }
}

void dao::cmntupd(const string new_content, const uint64_t comment_id)
{
  TRACE_FUNCTION();
  EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

  Comment comment(*this, comment_id);
  require_auth(comment.getAuthor());

  comment.edit(new_content);
}

void dao::cmntrem(const uint64_t comment_id)
{
  TRACE_FUNCTION();
  EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

  Comment comment(*this, comment_id);
  require_auth(comment.getAuthor());

  comment.markAsDeleted();
}

void dao::reactadd(const name &user, const name &reaction, const uint64_t document_id)
{
  TRACE_FUNCTION()
  EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");
  require_auth(user);
  TypedDocumentFactory::getLikeableDocument(*this, document_id)->like(user, reaction);
}

void dao::reactrem(const name &user, const uint64_t document_id)
{
  TRACE_FUNCTION()
  EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");
  require_auth(user);
  TypedDocumentFactory::getLikeableDocument(*this, document_id)->unlike(user);
}

void dao::withdraw(name owner, uint64_t document_id)
{
  TRACE_FUNCTION();
  EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

  RecurringActivity assignment(this, document_id);

  eosio::name assignee = assignment.getAssignee().getAccount();

  EOS_CHECK(
    assignee == owner &&
    (eosio::has_auth(owner) || eosio::has_auth(get_self())),
    to_str("Only the member [", assignee.to_string(), "] can withdraw the assignment [", document_id, "]")
  );

  auto cw = assignment.getContentWrapper();

  auto detailsGroup = cw.getGroupOrFail(DETAILS);

  auto state = cw.getOrFail(DETAILS, common::STATE)->getAs<string>();

  auto now = eosio::current_time_point();

  EOS_CHECK(
    state == common::STATE_APPROVED,
    to_str("Cannot withdraw ", state, " assignments")
  );

  //Quick check to verify if this is a self approved badge
  //which doesn't contain periods data
  if (auto [_, selfApproved] = cw.get(SYSTEM, common::CATEGORY_SELF_APPROVED);
      selfApproved && selfApproved->getAs<int64_t>()) {
    
    //If for some reason we would like to store end time for infinte assignments
    //we could just replace to isInfinte || now < endDate
    if (!assignment.isInfinite() && now < assignment.getEndDate()) {
      ContentWrapper::insertOrReplace(*detailsGroup, Content{ END_TIME, now });
    }

    auto badge = badges::getBadgeOf(*this, assignment.getID());

    badges::onBadgeArchived(*this, assignment);
  }
  else {
    auto originalPeriods = assignment.getPeriodCount();

    auto startPeriod = assignment.getStartPeriod();

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
        to_str("Withdrawal of expired assignment: ", document_id, " is not allowed")
      );

      if (cw.getOrFail(SYSTEM, TYPE)->getAs<name>() == common::ASSIGNMENT) 
      {
        modifyCommitment(assignment, 0, std::nullopt, common::MOD_WITHDRAW);
      }
    }

    ContentWrapper::insertOrReplace(*detailsGroup, Content{ PERIOD_COUNT, periodsToCurrent });
  }

  ContentWrapper::insertOrReplace(*detailsGroup, Content{ common::STATE, common::STATE_WITHDRAWED });

  assignment.update();
}

void dao::suspend(name proposer, uint64_t document_id, string reason)
{
  TRACE_FUNCTION();
  EOS_CHECK(
    !isPaused(),
    "Contract is paused for maintenance. Please try again later."
  );

  //Get the DAO edge from the hash
  auto daoID = Edge::get(get_self(), document_id, common::DAO).getToNode();

  EOS_CHECK(
    Member::isMember(*this, daoID, proposer),
    to_str("Only members are allowed to propose suspensions")
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
  TRACE_FUNCTION();

  EOS_CHECK(
    !isPaused(),
    "Contract is paused for maintenance. Please try again later."
  );

  Assignment assignment(this, assignment_id);

  eosio::name assignee = assignment.getAssignee().getAccount();

  //Check the state of the assignment is valid
  std::string state = assignment.getContentWrapper()
                                .getOrFail(DETAILS, common::STATE)
                                ->getAs<std::string>();

  //Valid states where user can claim
  std::vector<std::string> validStates = {
    common::STATE_APPROVED, 
    common::STATE_ARCHIVED,
    common::STATE_SUSPENDED, //Even if suspended or withdrawed there could be periods that weren't claimed
    common::STATE_WITHDRAWED
  };

  EOS_CHECK(
    std::any_of(
      validStates.begin(), 
      validStates.end(), 
      [&state](const std::string& x){ return x == state; }
    ),
    to_str("Cannot claim assignment with ", state, " state")
  )

  // assignee must still be a DHO member
  auto daoID = Edge::get(get_self(), assignment.getID(), common::DAO).getToNode();

  EOS_CHECK(
    Member::isMember(*this, daoID, assignee),
    "assignee must be a current member to claim pay: " + assignee.to_string()
  );

  std::optional<Period> periodToClaim = assignment.getNextClaimablePeriod();
  EOS_CHECK(periodToClaim != std::nullopt, to_str("All available periods for this assignment have been claimed: ", assignment_id));

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

  EOS_CHECK(
    daoSettings->getSettingOrDefault<int64_t>(common::CLAIM_ENABLED, 1) == 1 ||
    assignment.getContentWrapper()
              .getOrFail(DETAILS, DEFERRED)
              ->getAs<int64_t>() == 100,
    "Claiming is disabled for assignments with deferral less than 100%"
  )

  auto daoTokens = AssetBatch{
    .reward = daoSettings->getOrFail<asset>(common::REWARD_TOKEN),
    .peg = daoSettings->getOrFail<asset>(common::PEG_TOKEN),
    .voice = daoSettings->getOrFail<asset>(common::VOICE_TOKEN)
  };

  const asset pegSalary = assignment.getPegSalary();
  const asset voiceSalary = assignment.getVoiceSalary();
  const asset rewardSalary = assignment.getRewardSalary();

  const int64_t initTimeShare = assignment.getInitialTimeShare()
    .getContentWrapper()
    .getOrFail(DETAILS, TIME_SHARE)
    ->getAs<int64_t>();

  TimeShare current = assignment.getCurrentTimeShare();

  //Initialize nextOpt with current in order to have a valid initial timeShare
  auto nextOpt = std::optional<TimeShare>{ current };

  std::optional<TimeShare> lastUsedTimeShare = current;

  auto ab = calculatePeriodPayout(
      *periodToClaim, 
      AssetBatch{ .reward = rewardSalary, .peg = pegSalary, .voice = voiceSalary }, 
      daoTokens, 
      nextOpt,
      lastUsedTimeShare, 
      initTimeShare
  );

  //If the last used time share is different from current time share
  //let's update the edge
  if (lastUsedTimeShare->getID() != current.getID())
  {
    Edge::get(get_self(), assignment.getID(), common::CURRENT_TIME_SHARE).erase();
    Edge::write(get_self(), get_self(), assignment.getID(), lastUsedTimeShare->getID(), common::CURRENT_TIME_SHARE);
  }

  // EOS_CHECK(deferredSeeds.is_valid(), "fatal error: SEEDS has to be a valid asset");
  EOS_CHECK(ab.peg.is_valid(), "fatal error: PEG has to be a valid asset");
  EOS_CHECK(ab.voice.is_valid(), "fatal error: VOICE has to be a valid asset");
  EOS_CHECK(ab.reward.is_valid(), "fatal error: REWARD has to be a valid asset");

  string assignmentNodeLabel = "";
  if (auto [idx, assignmentLabel] = assignment.getContentWrapper().get(SYSTEM, NODE_LABEL); assignmentLabel)
  {
    EOS_CHECK(std::holds_alternative<std::string>(assignmentLabel->value), "fatal error: assignment content item type is expected to be a string: " + assignmentLabel->label);
    assignmentNodeLabel = std::get<std::string>(assignmentLabel->value);
  }

  //If node_label is not present for any reason fallback to the assignment hash
  if (assignmentNodeLabel.empty()) {
    assignmentNodeLabel = to_str(assignment.getID());
  }

  string memo = assignmentNodeLabel + ", period: " + periodToClaim.value().getNodeLabel();

  makePayment(daoSettings, periodToClaim.value().getID(), assignee, ab.reward, memo, eosio::name{ 0 }, daoTokens);
  makePayment(daoSettings, periodToClaim.value().getID(), assignee, ab.voice, memo, eosio::name{ 0 }, daoTokens);
  makePayment(daoSettings, periodToClaim.value().getID(), assignee, ab.peg, memo, eosio::name{ 0 }, daoTokens);
}

// void dao::simclaimall(name account, uint64_t dao_id, bool only_ids)
// {
//   auto daoSettings = getSettingsDocument(dao_id);

//   //We might reuse if called from simclaimall
//   auto daoTokens = AssetBatch{
//     .reward = daoSettings->getOrFail<asset>(common::REWARD_TOKEN),
//     .peg = daoSettings->getOrFail<asset>(common::PEG_TOKEN),
//     .voice = daoSettings->getOrFail<asset>(common::VOICE_TOKEN)
//   };

//   auto assignments = m_documentGraph.getEdgesFrom(getMemberID(account), common::ASSIGNED);

//   AssetBatch total {
//     .peg = eosio::asset{ 0, daoTokens.peg.symbol },
//     .voice = eosio::asset{ 0, daoTokens.voice.symbol },
//     .reward = eosio::asset{ 0, daoTokens.reward.symbol }
//   };

//   string ids = "[";
//   string sep = "";

//   for (auto& assignEdge : assignments) {
//     Assignment assignment(this, assignEdge.getToNode());

//     //Filter by dao
//     if (assignment.getDaoID() != dao_id) continue;

//     auto lastPeriod = assignment.getLastPeriod();

//     int64_t periods = assignment.getPeriodCount();
//     auto claimedPeriods = m_documentGraph.getEdgesFrom(assignment.getID(), common::CLAIMED);

//     //There are some assignments where the initial periods were not claimed,
//     //we should not count them
//     if (claimedPeriods.size() < periods &&
//         std::all_of(claimedPeriods.begin(), 
//                     claimedPeriods.end(), 
//                     [id = lastPeriod.getID()](const Edge& e){ return e.to_node != id; })) { 
      
//       ids += sep + to_str(assignment.getID());
//       if (sep.empty()) sep = ",";

//       if (!only_ids) {
//         total += calculatePendingClaims(assignment.getID(), daoTokens);
//       }
//     }
//   }

//   ids.push_back(']');

//   EOS_CHECK(
//     false,
//     to_str(
//         "{\n", 
//           "\"peg\":\"", total.peg, "\",\n",
//           "\"reward\":\"", total.reward, "\",\n",
//           "\"voice\":\"", total.voice, "\",\n",
//           "\"ids\":", ids,
//         "}"
//     )
//   )
// }

// void dao::simclaim(uint64_t assignment_id)
// {
//   auto daoID = Edge::get(get_self(), assignment_id, common::DAO).getToNode();
//   auto daoSettings = getSettingsDocument(daoID);

//   //We might reuse if called from simclaimall
//   auto daoTokens = AssetBatch{
//     .reward = daoSettings->getOrFail<asset>(common::REWARD_TOKEN),
//     .peg = daoSettings->getOrFail<asset>(common::PEG_TOKEN),
//     .voice = daoSettings->getOrFail<asset>(common::VOICE_TOKEN)
//   };

//   auto pay = calculatePendingClaims(assignment_id, daoTokens);

//   EOS_CHECK(
//     false,
//     to_str("{\n\"peg\":\"", pay.peg, "\",\n\"reward\":\"", pay.reward, "\",\n\"voice\":\"", pay.voice, "\"\n}")
//   )
// }

AssetBatch dao::calculatePeriodPayout(Period& period,
                                      const AssetBatch& salary,
                                      const AssetBatch& daoTokens,
                                      std::optional<TimeShare>& nextTimeShareOpt,
                                      std::optional<TimeShare>& lastUsedTimeShare,
                                      int64_t initTimeShare)
{
  int64_t periodStartSec = period.getStartTime().sec_since_epoch();
  int64_t periodEndSec = period.getEndTime().sec_since_epoch();

  AssetBatch payout {
    .reward = eosio::asset{ 0, daoTokens.reward.symbol },
    .peg = eosio::asset{ 0, daoTokens.peg.symbol },
    .voice = eosio::asset{ 0, daoTokens.voice.symbol }
  };
          
  while (nextTimeShareOpt)
  {
    ContentWrapper nextWrapper = nextTimeShareOpt->getContentWrapper();

    const int64_t timeShare = nextWrapper.getOrFail(DETAILS, TIME_SHARE)->getAs<int64_t>();
    const int64_t startDateSec = nextTimeShareOpt->getStartDate().sec_since_epoch();

    //If the time share doesn't belong to the claim period we
    //finish pro-rating
    if (periodEndSec - startDateSec <= 0)
    {
      break;
    }

    int64_t remainingTimeSec;

    lastUsedTimeShare = std::move(nextTimeShareOpt.value());

    //It's possible that time share was set on previous periods,
    //if so we should use period start date as the base date
    const int64_t baseDateSec = std::max(periodStartSec, startDateSec);

    //Check if there is another timeshare
    if ((nextTimeShareOpt = lastUsedTimeShare->getNext()))
    {
      const time_point commingStartDate = nextTimeShareOpt->getContentWrapper()
                                                          .getOrFail(DETAILS, TIME_SHARE_START_DATE)
                                                          ->getAs<time_point>();
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

    EOS_CHECK(remainingTimeSec >= 0, "Remaining time cannot be negative");

    const int64_t fullPeriodSec = periodEndSec - periodStartSec;

    //Time share could only represent a portion of the whole period
    float relativeDuration = static_cast<float>(remainingTimeSec) / static_cast<float>(fullPeriodSec);
    float relativeCommitment = static_cast<float>(timeShare) / static_cast<float>(initTimeShare);
    float commitmentMultiplier = relativeDuration * relativeCommitment;

    //Accumlate each of the currencies with the time share multiplier

    payout.peg += adjustAsset(salary.peg, commitmentMultiplier);
    payout.voice += adjustAsset(salary.voice, commitmentMultiplier);
    payout.reward += adjustAsset(salary.reward, commitmentMultiplier);
  }

  return payout;
}

AssetBatch dao::calculatePendingClaims(uint64_t assignmentID, const AssetBatch& daoTokens)
{
  Assignment assignment(this, assignmentID);
  
  AssetBatch payAmount {
    .reward = eosio::asset{ 0, daoTokens.reward.symbol },
    .peg = eosio::asset{ 0, daoTokens.peg.symbol },
    .voice = eosio::asset{ 0, daoTokens.voice.symbol }
  };

  // Ensure that the claimed period is within the approved period count
  Period period = assignment.getStartPeriod();
  int64_t periodCount = assignment.getPeriodCount();
  int64_t counter = 0;

  const asset pegSalary = assignment.getPegSalary();
  const asset voiceSalary = assignment.getVoiceSalary();
  const asset rewardSalary = assignment.getRewardSalary();

  AssetBatch salary{
    .reward = rewardSalary,
    .peg = pegSalary,
    .voice = voiceSalary
  };

  auto currentTime = eosio::current_time_point().sec_since_epoch();

  const int64_t initTimeShare = assignment.getInitialTimeShare()
                                          .getContentWrapper()
                                          .getOrFail(DETAILS, TIME_SHARE)
                                          ->getAs<int64_t>();

  TimeShare current = assignment.getCurrentTimeShare();

  //Initialize nextOpt with current in order to have a valid initial timeShare
  auto nextOpt = std::optional<TimeShare>{ current };

  std::optional<TimeShare> lastTimeShare;

  while (counter < periodCount)
  {
      int64_t periodEndSec = period.getEndTime().sec_since_epoch();
      if (periodEndSec <= currentTime &&   // if period has lapsed
          !assignment.isClaimed(&period))         // and not yet claimed
      {
          payAmount += calculatePeriodPayout(period, salary, daoTokens, nextOpt, lastTimeShare, initTimeShare);
          nextOpt = lastTimeShare;
      }
      period = period.next();
      counter++;
  }

  return payAmount;
}

asset dao::getProRatedAsset(ContentWrapper* assignment, const symbol& symbol, const string& key, const float& proration)
{
  TRACE_FUNCTION();
  asset assetToPay = asset{ 0, symbol };
  if (auto [idx, assetContent] = assignment->get(DETAILS, key); assetContent)
  {
    EOS_CHECK(std::holds_alternative<eosio::asset>(assetContent->value), "fatal error: expected token type must be an asset value type: " + assetContent->label);
    assetToPay = std::get<eosio::asset>(assetContent->value);
  }
  return adjustAsset(assetToPay, proration);
}

void dao::makePayment(Settings* daoSettings,
  uint64_t fromNode,
  const eosio::name& recipient,
  const eosio::asset& quantity,
  const string& memo,
  const eosio::name& paymentType,
  const AssetBatch& daoTokens)
{
  TRACE_FUNCTION();
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

void dao::apply(const eosio::name& applicant, uint64_t dao_id, const std::string& content)
{
  TRACE_FUNCTION();

  verifyDaoType(dao_id);

  require_auth(applicant);
  
  auto member = getOrCreateMember(applicant);

  member.apply(dao_id, content);
}

void dao::enroll(const eosio::name& enroller, uint64_t dao_id, const eosio::name& applicant, const std::string& content)
{
  TRACE_FUNCTION();

  verifyDaoType(dao_id);

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
  TRACE_FUNCTION();

  return getSettingsDocument(getRootID());
}

void dao::setsetting(const string& key, const Content::FlexValue& value, std::optional<std::string> group)
{
  TRACE_FUNCTION();
  require_auth(get_self());
  auto settings = getSettingsDocument();

  settings->setSetting(group.value_or(string{ "settings" }), Content{ key, value });
}

void dao::setdaosetting(const uint64_t& dao_id, std::map<std::string, Content::FlexValue> kvs, std::optional<std::string> group)
{
  TRACE_FUNCTION();
  EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

  checkAdminsAuth(dao_id);

  auto settings = getSettingsDocument(dao_id);
  auto dhoSettings = getSettingsDocument();

  auto daoName = settings->getOrFail<eosio::name>(DAO_NAME);

  auto daoDoc = Document(get_self(), dao_id);

  auto daoCW = daoDoc.getContentWrapper();

  auto type = daoCW.getOrFail(SYSTEM, TYPE)->getAs<name>();

  auto isDraft = type == common::DAO_DRAFT;

  //Verify ID belongs to a valid DAO
  EOS_CHECK(
    isDraft ||
    getDAOID(daoName).value_or(0) == dao_id,
    to_str("The provided ID is not valid: ", dao_id)
  );

  //Only hypha dao should be able to use this flag
  EOS_CHECK(
    kvs.count("is_hypha") == 0 ||
    daoName == eosio::name("hypha"),
    "Only hypha dao is allowed to add this setting"
  );

  std::string groupName = group.value_or(std::string{ "settings" });

  //Fixed settings that cannot be changed
  using FixedSettingsMap = std::map<std::string, const std::vector<std::string>>;

  //TODO: Add new setting to enable/disable direct setdaosetting
  //and only enable setting's changes through msig proposals

  //If the action was authorized with contract permission, let's allow changing any 
  //setting
  FixedSettingsMap fixedSettings = eosio::has_auth(get_self()) || isDraft ? 
  FixedSettingsMap{} : 
  FixedSettingsMap {
    {
      SETTINGS,
      {
        common::REWARD_TOKEN,
        common::VOICE_TOKEN,
        common::PEG_TOKEN,
        common::PERIOD_DURATION,
        common::CLAIM_ENABLED,
        common::ADD_ADMINS_ENABLED,
        common::MSIG_APPROVAL_AMOUNT,
        DAO_NAME
      }
    }
  };

  //If groupName doesn't exists in fixedSettings it will just create an empty array
  for (auto& fs : fixedSettings[groupName]) {
    EOS_CHECK(
      kvs.count(fs) == 0,
      to_str(fs, " setting cannot be modified in group: ", groupName)
    );
  }

  if (!isDraft) {
    //Verify if the URL is unique if we want to change it
    if (kvs.count(common::DAO_URL)) {
      updateDaoURL(daoName, kvs[common::DAO_URL]);
    }
    else if (kvs.count(common::VOICE_TOKEN_DECAY_PERIOD) || 
            kvs.count(common::VOICE_TOKEN_DECAY_PER_PERIOD)) {
      auto getOrDef = [&](const std::string& label) -> int64_t { 
        if (kvs.count(label)) {
          return Content{"",kvs[label]}.getAs<int64_t>();
        }
        else {
          return settings->getOrFail<int64_t>(label);
        }
      };
      
      changeDecay(
        dhoSettings,
        settings,
        static_cast<uint64_t>(getOrDef(common::VOICE_TOKEN_DECAY_PERIOD)),
        static_cast<uint64_t>(getOrDef(common::VOICE_TOKEN_DECAY_PER_PERIOD))
      );
    }
  }

  settings->setSettings(groupName, kvs);
}

//  void dao::adddaosetting(const uint64_t& dao_id, const std::string &key, const Content::FlexValue &value, std::optional<std::string> group)
//  {
//    TRACE_FUNCTION();
//    checkAdminsAuth(dao_id);
//    auto settings = getSettingsDocument(dao_id);
//    //Only hypha dao should be able to use this flag
//    EOS_CHECK(
//      key != "is_hypha" ||
//      settings->getOrFail<eosio::name>(DAO_NAME) ==  eosio::name("hypha"),
//      "Only hypha dao is allowed to add this setting"
//    );
//    settings->addSetting(group.value_or(string{"settings"}), Content{key, value});
//  }

// void dao::remdaosetting(const uint64_t& dao_id, const std::string& key, std::optional<std::string> group)
// {
//   TRACE_FUNCTION();
//   EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

//   checkAdminsAuth(dao_id);
//   auto settings = getSettingsDocument(dao_id);
//   settings->remSetting(group.value_or(string{ "settings" }), key);
// }

// void dao::remkvdaoset(const uint64_t& dao_id, const std::string& key, const Content::FlexValue& value, std::optional<std::string> group)
// {
//   TRACE_FUNCTION();
//   EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");
//   checkAdminsAuth(dao_id);
//   auto settings = getSettingsDocument(dao_id);
//   settings->remKVSetting(group.value_or(string{ "settings" }), Content{ key, value });
// }

// void dao::remsetting(const string& key)
// {
//   TRACE_FUNCTION();
//   EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");
//   require_auth(get_self());
//   auto settings = getSettingsDocument();
//   settings->remSetting(key);
// }

static void makeAutoApproveBadge(const name& contract, const std::string& title, const std::string& desc, const name& owner, uint64_t daoID, int64_t badgeID)
{
  eosio::action(
      eosio::permission_level(contract, eosio::name("active")),
      contract,
      eosio::name("propose"),
      std::make_tuple(
          daoID,
          contract,
          common::ASSIGN_BADGE,
          ContentGroups{
              ContentGroup{
                  Content{ CONTENT_GROUP_LABEL, DETAILS },
                  Content{ TITLE, title },
                  Content{ DESCRIPTION, desc },
                  Content{ ASSIGNEE, owner },
                  Content{ BADGE_STRING, badgeID },
                  Content{ common::AUTO_APPROVE, 1 }
          }},
          true
      )
  ).send();
}

static void makeEnrollerBadge(dao& dao, const name& owner, uint64_t daoID)
{
  auto enrollerBadgeEdge = Edge::get(dao.get_self(), dao.getRootID(), badges::common::links::ENROLLER_BADGE);
  auto enrollerBadge = TypedDocument::withType(dao, enrollerBadgeEdge.getToNode(), common::BADGE_NAME);
  makeAutoApproveBadge(dao.get_self(), "Enroller", "Enroller Permissions", owner, daoID, enrollerBadge.getID());
}

static void makeAdminBadge(dao& dao, const name& owner, uint64_t daoID)
{
  auto adminBadgeEdge = Edge::get(dao.get_self(), dao.getRootID(), badges::common::links::ADMIN_BADGE);
  auto adminBadge = TypedDocument::withType(dao, adminBadgeEdge.getToNode(), common::BADGE_NAME);
  makeAutoApproveBadge(dao.get_self(), "Admin", "Admin Permissions", owner, daoID, adminBadge.getID());
}

void  dao::addenroller(const uint64_t dao_id, name enroller_account)
{
  TRACE_FUNCTION();
  EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

  verifyDaoType(dao_id);

  checkAdminsAuth(dao_id);
  auto mem = getOrCreateMember(enroller_account);
  
  mem.checkMembershipOrEnroll(dao_id);

  //Edge::getOrNew(get_self(), get_self(), dao_id, getMemberID(enroller_account), common::ENROLLER);

  //Create propose action to generate badge assing
  makeEnrollerBadge(*this, enroller_account, dao_id);
}

void dao::addadmins(const uint64_t dao_id, const std::vector<name>& admin_accounts)
{
  TRACE_FUNCTION();
  EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

  verifyDaoType(dao_id);

  auto daoSettings = getSettingsDocument(dao_id);

  EOS_CHECK(
    daoSettings->getSettingOrDefault<int64_t>(common::ADD_ADMINS_ENABLED, 1) == 1,
    to_str("Adding admins feature is not enabled")
  )

  checkAdminsAuth(dao_id);

  for (auto adminAccount : admin_accounts) {
    auto mem = getOrCreateMember(adminAccount);
    
    mem.checkMembershipOrEnroll(dao_id);

    //Edge::getOrNew(get_self(), get_self(), dao_id, getMemberID(adminAccount), common::ADMIN);

    makeAdminBadge(*this, adminAccount, dao_id);
  }

  //Automatically disable the feature after using it
  daoSettings->setSetting(Content{common::ADD_ADMINS_ENABLED, int64_t(0)});
}

static bool remBadgePerm(dao& dao, const name& member, uint64_t daoID, const name& badgeEdge) {

  auto badgeAssignmentEdges = dao.getGraph().getEdgesFrom(dao.getMemberID(member), badgeEdge);

  for (auto& edge : badgeAssignmentEdges) {
    auto badgeAssignID = edge.getToNode();
    if (Edge::exists(dao.get_self(), badgeAssignID, daoID, common::DAO)) {
      eosio::action(
        eosio::permission_level{ dao.get_self(), name("active") },
        dao.get_self(),
        name("archiverecur"),
        std::make_tuple(
          badgeAssignID
        )
      ).send();
      return true;
    }
  }

  return false;
}

void dao::remenroller(const uint64_t dao_id, name enroller_account)
{
  TRACE_FUNCTION();
  EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

  verifyDaoType(dao_id);

  checkAdminsAuth(dao_id);

  // EOS_CHECK(
  //   m_documentGraph.getEdgesFrom(dao_id, common::ENROLLER).size() > 1,
  //   "Cannot remove enroller, there has to be at least 1"
  // );

  //Check first if the user has enroller badge
  if (!remBadgePerm(*this, enroller_account, dao_id, badges::common::links::ENROLLER_BADGE)) {
    //If not just remove the permission
    Edge::get(get_self(), dao_id, getMemberID(enroller_account), common::ENROLLER).erase();
  }

}

void dao::createmsig(uint64_t dao_id, name creator, std::map<std::string, Content::FlexValue> kvs)
{
  auto memberID = getMemberID(creator);

  checkAdminsAuth(dao_id);

  eosio::require_auth(creator);

  //Check there are not open msig proposals
  EOS_CHECK(
    !Edge::getIfExists(get_self(), dao_id, common::OPEN_MSIG).first,
    "There is an open mulltisig proposal already"
  );

  //Fixed settings that cannot be changed
  using FixedSettingsMap = std::map<std::string, const std::vector<std::string>>;

  //TODO: Add new setting to enable/disable direct setdaosetting
  //and only enable setting's changes through msig proposals

  //If the action was authorized with contract permission, let's allow changing any 
  //setting
  FixedSettingsMap fixedSettings = FixedSettingsMap {
    {
      SETTINGS,
      {
        common::REWARD_TOKEN,
        common::VOICE_TOKEN,
        common::PEG_TOKEN,
        common::PERIOD_DURATION,
        common::CLAIM_ENABLED,
        common::ADD_ADMINS_ENABLED,
        DAO_NAME
      }
    }
  };

  //If groupName doesn't exists in fixedSettings it will just create an empty array
  for (auto& fs : fixedSettings[SETTINGS]) {
    EOS_CHECK(
      kvs.count(fs) == 0,
      to_str(fs, " setting cannot be modified in group: ", SETTINGS)
    );
  }

  //Check for url_change
  if (kvs.count(common::DAO_URL)) {
    auto newUrlCon = kvs[common::DAO_URL];

    EOS_CHECK(
      std::holds_alternative<std::string>(newUrlCon) &&
      std::get<std::string>(newUrlCon).size() <= 56,
      "Url must be a string and less than 56 characters"
    )

    auto globalSettings = getSettingsDocument();
  
    auto globalSetCW = globalSettings->getContentWrapper();

    if (auto [_, urlsGroup] = globalSetCW.getGroup(common::URLS_GROUP); 
        urlsGroup) {
      for (auto& url : *urlsGroup) {
        EOS_CHECK(
          url.label == CONTENT_GROUP_LABEL ||
          url.value != newUrlCon,
          to_str("URL is already being used, please use a different one", " ", url.label, " ", url.getAs<std::string>())
        );
      }
    }
  }

  ContentGroup settings {
    Content{ CONTENT_GROUP_LABEL, SETTINGS }
  };

  for (auto& [key, value] : kvs) {
    settings.emplace_back( Content{ key, value} );
  }

  auto msigContents = ContentGroups({
    ContentGroup{
        Content(CONTENT_GROUP_LABEL, DETAILS), 
        //Content(ROOT_NODE, contract)
        Content{common::DAO.to_string(), static_cast<int64_t>(dao_id)},
        Content{common::OWNER.to_string(), creator},
    },
    ContentGroup{
        Content(CONTENT_GROUP_LABEL, SYSTEM), 
        Content(TYPE, common::MSIG),
        Content(NODE_LABEL, to_str("Multisig proposal"))
    },
    std::move(settings)
  });

  Document msigDoc(get_self(), creator, msigContents);

  Edge(get_self(), get_self(), dao_id, msigDoc.getID(), common::OPEN_MSIG);
}

void dao::votemsig(uint64_t msig_id, name signer, bool approve)
{
  auto memberID = getMemberID(signer);

  //Check proposal is still open for voting
  auto daoID = Edge::getTo(get_self(), msig_id, common::OPEN_MSIG).getFromNode();

  checkAdminsAuth(daoID);

  eosio::require_auth(signer);

  if (approve) {
    Edge(get_self(), get_self(), memberID, msig_id, common::APPROVE_MSIG);
    Edge(get_self(), get_self(), msig_id, memberID, common::APPROVED_BY);
  }
  else {
    Edge::get(get_self(), memberID, msig_id, common::APPROVE_MSIG).erase();
    Edge::get(get_self(), msig_id, memberID, common::APPROVED_BY).erase();
  }
}

void dao::execmsig(uint64_t msig_id, name executer)
{
  auto memberID = getMemberID(executer);

  //Check proposal is not already closed
  auto daoID = Edge::getTo(get_self(), msig_id, common::OPEN_MSIG).getFromNode();

  //Might need to add workaround for when admins are removed or added
  //when proposal already exists

  checkAdminsAuth(daoID);

  eosio::require_auth(executer);

  auto daoSettings = getSettingsDocument(daoID);

  //Default approval amount is 2
  auto requiredApprovals = daoSettings->getSettingOrDefault<int64_t>(common::MSIG_APPROVAL_AMOUNT, 1);

  //Check if enough approvals
  auto approvalsCount = Edge::getEdgesToCount(get_self(), msig_id, common::APPROVE_MSIG);

  EOS_CHECK(
    approvalsCount >= requiredApprovals,
    to_str("Not enough approvals required ", requiredApprovals, " got ", approvalsCount)
  );

  Edge::get(get_self(), daoID, msig_id, common::OPEN_MSIG).erase();

  Edge(get_self(), get_self(), msig_id, memberID, common::EXECUTED_BY);

  auto msigDoc = TypedDocument::withType(*this, msig_id, common::MSIG);

  auto misgCW = msigDoc.getContentWrapper();

  auto settings = misgCW.getGroupOrFail(SETTINGS);

  std::map<std::string, Content::FlexValue> kvs;

  for (auto& setting : *settings) {
    if (setting.label != CONTENT_GROUP_LABEL) {
      kvs.insert({ setting.label, setting.value });
    }
  }

  eosio::action(
    eosio::permission_level{ get_self(), name("active") },
    get_self(),
    name("setdaosetting"),
    std::make_tuple(
      daoID,
      kvs,
      std::optional<std::string>{SETTINGS}
    )
  ).send();
}

void dao::cancelcmsig(uint64_t msig_id, name canceler)
{
  auto memberID = getMemberID(canceler);

   //Check proposal is not already closed
  auto daoID = Edge::getTo(get_self(), msig_id, common::OPEN_MSIG).getFromNode();

  //Might need to add workaround for when admins are removed or added
  //when proposal already exists

  checkAdminsAuth(daoID);

  eosio::require_auth(canceler);

  //Check if the msig is still open and close it
  Edge::get(get_self(), daoID, msig_id, common::OPEN_MSIG).erase();

  Edge(get_self(), get_self(), msig_id, memberID, common::CANCELED_BY);
}

void dao::remadmin(const uint64_t dao_id, name admin_account)
{
  TRACE_FUNCTION();
  EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

  verifyDaoType(dao_id);

  //checkAdminsAuth(dao_id);
  eosio::require_auth(get_self());

  // EOS_CHECK(
  //   m_documentGraph.getEdgesFrom(dao_id, common::ADMIN).size() > 1,
  //   "Cannot remove admin, there has to be at least 1"
  // );

  //Check first if the user has admin badge
  if (!remBadgePerm(*this, admin_account, dao_id, badges::common::links::ADMIN_BADGE)) {
    //If not just remove the permission
    Edge::get(get_self(), dao_id, getMemberID(admin_account), common::ADMIN).erase();
  }
}

void dao::genperiods(uint64_t dao_id, int64_t period_count/*, int64_t period_duration_sec*/)
{
  TRACE_FUNCTION();
  EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

  verifyDaoType(dao_id);

  if (!eosio::has_auth(get_self())) {
    checkAdminsAuth(dao_id);
  }

  genPeriods(dao_id, period_count);
}

static void initCoreMembers(dao& dao, uint64_t daoID, eosio::name onboarder, ContentWrapper config) 
{
  std::set<eosio::name> coreMemNames = { onboarder };

  if (auto coreMemsGroup = config.getGroup("core_members").second;
      coreMemsGroup && coreMemsGroup->size() > 1) {

    EOS_CHECK(
      coreMemsGroup->at(0).label == CONTENT_GROUP_LABEL,
      to_str("Wrong format for core groups\n"
        "[min size: 2, got: ", coreMemsGroup->size(), "]\n",
        "[first item label: ", CONTENT_GROUP_LABEL, " got: ", coreMemsGroup->at(0).label)
    );

    //Skip content_group label (index 0)
    std::transform(coreMemsGroup->begin() + 1,
      coreMemsGroup->end(),
      std::inserter(coreMemNames, coreMemNames.begin()),
      [](const Content& c) { return c.getAs<name>(); });
  }

  for (auto& coreMem : coreMemNames) {

    auto member = dao.getOrCreateMember(coreMem);

    member.apply(daoID, "DAO Core member");
    member.enroll(onboarder, daoID, "DAO Core member");

    //Create owner, admin and enroller edges
    //Edge(dao.get_self(), dao.get_self(), daoID, member.getID(), common::ENROLLER);
    //Edge(dao.get_self(), dao.get_self(), daoID, member.getID(), common::ADMIN);
    makeEnrollerBadge(dao, coreMem, daoID);
    makeAdminBadge(dao, coreMem, daoID);
  }
}

static void checkEcosystemDao(dao& dao, uint64_t daoID)
{
  auto ecosystemDoc = TypedDocument::withType(dao, daoID, common::DAO);
  auto cw = ecosystemDoc.getContentWrapper();
  auto daoType =  cw.getOrFail(DETAILS, common::DAO_TYPE)
                    ->getAs<string>();
  EOS_CHECK(
    daoType == pricing::common::dao_types::ANCHOR,
    "Provided parent ID is not an ecosystem"
  );

  auto isWaitingEcosystem = cw.getOrFail(DETAILS, pricing::common::items::IS_WAITING_ECOSYSTEM)
                               ->getAs<int64_t>();

  EOS_CHECK(
    isWaitingEcosystem == 0,
    "Parent ecosystem hasn't been activated yet"
  );
}

void dao::createdao(ContentGroups& config)
{
  TRACE_FUNCTION();
  EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

  auto configCW = ContentWrapper(config);

  auto daoName = configCW.getOrFail(DETAILS, DAO_NAME);

  const name dao = daoName->getAs<name>();

  auto createDao = [&](uint64_t* parentID) {
    
    Document daoDoc(
      get_self(), 
      get_self(), 
      getDAOContent(
        daoName->getAs<name>(), 
        parentID ? pricing::common::dao_types::ANCHOR_CHILD :
                   pricing::common::dao_types::INDIVIDUAL
      )
    );
      
    //This will check if the requested DAO name already exists, otherwise will add it
    //to the daoName-id table
    //NOTE: May need to rethink this mechanism as it wouldn't allow repeated names
    //between different Ecosystems
    addNameID<dao_table>(dao, daoDoc.getID());

    //Extract mandatory configurations from DraftDao if present, or use the
    //configCW items if not
    if (auto draftDao = configCW.get(DETAILS, common::DAO_DRAFT_ID).second) {
      
      auto draftDaoID = static_cast<uint64_t>(draftDao->getAs<int64_t>());

      EOS_CHECK(
        parentID && //For now only allow Draft DAO's for child DAO's
        Edge::get(
          get_self(), 
          draftDaoID, 
          common::PARENT_DAO
        ).getToNode() == *parentID
        ,
        "Draft Dao doesn't belong to the Ecosystem"
      );

      TypedDocument::withType(
        *this, 
        draftDaoID, 
        common::DAO_DRAFT
      );

      readDaoSettings(daoDoc.getID(), dao, getSettingsDocument(draftDaoID)->getContentWrapper(), false, SETTINGS);

      //Delete DraftDao after
      eosio::action(
        eosio::permission_level{ get_self(), name("active") },
        get_self(),
        name("deletedaodft"),
        std::make_tuple(
          draftDaoID
        )
      ).send();
    }
    else {
      readDaoSettings(daoDoc.getID(), dao, configCW, false);
    }
    
    //Create start period
    Period newPeriod(this, eosio::current_time_point(), to_str(dao, " start period"));

    //Create start & end edges
    Edge(get_self(), get_self(), daoDoc.getID(), newPeriod.getID(), common::START);
    Edge(get_self(), get_self(), daoDoc.getID(), newPeriod.getID(), common::END);
    Edge(get_self(), get_self(), daoDoc.getID(), newPeriod.getID(), common::PERIOD);
    Edge(get_self(), get_self(), newPeriod.getID(), daoDoc.getID(), common::DAO);
    
#ifdef USE_TREASURY
    //Create Treasury
    eosio::action(
      eosio::permission_level{ get_self(), name("active") },
      get_self(),
      name("createtrsy"),
      std::make_tuple(
        daoDoc.getID()
      )
    ).send();
#endif

    eosio::action(
      eosio::permission_level{ get_self(), name("active") },
      get_self(),
      name("setupdefs"),
      std::make_tuple(
        daoDoc.getID()
      )
    ).send();

    auto onboarder = getSettingsDocument(daoDoc.getID())->getOrFail<name>(common::ONBOARDER_ACCOUNT);

    //Init core members should happen after scheduling setupdefs action 
    //as that is resposible for generating the "badge" edges from the DAO to
    //the badges used in admin/enroller proposals inside initCoreMembers
    initCoreMembers(*this, daoDoc.getID(), onboarder, configCW);

    return daoDoc;
  };

  auto daoParent = configCW.get(DETAILS, common::DAO_PARENT_ID).second;

  if (daoParent) {
#ifdef USE_PRICING_PLAN
    auto daoParentID = static_cast<uint64_t>(daoParent->getAs<int64_t>());
    //Only admins of parent DAO are allowed to create children DAO's
    checkAdminsAuth(daoParentID);
    checkEcosystemDao(*this, daoParentID);

    auto parentPlanManager = pricing::PlanManager::getFromDaoID(*this, daoParentID);

    auto beneficiary = configCW.getOrFail(DETAILS, common::BENEFICIARY_ACCOUNT)->getAs<name>();

    //Check Parent DAO (a.k.a. Ecosystem) has enough funds to activate this DAO
    verifyEcosystemPayment(
      parentPlanManager, 
      pricing::common::items::ECOSYSTEM_CHILD_PRICE,
      pricing::common::items::ECOSYSTEM_CHILD_PRICE_STAKED,
      to_str("Staking HYPHA on Ecosystem Child DAO creation:", dao),
      beneficiary
    );

    auto daoDoc = createDao(&daoParentID);

    Edge(get_self(), get_self(), daoParentID, daoDoc.getID(), common::CHILD_DAO);
    Edge(get_self(), get_self(), daoDoc.getID(), daoParentID, common::PARENT_DAO);
  
    //Let's assign a PlanManager and set it's pricing plan to the best one
    pricing::PlanManager planManager(*this, dao, pricing::PlanManagerData {
        .credit = eosio::asset{ 0, hypha::common::S_HYPHA },
        .type = pricing::common::types::UNLIMITED
    });

    setEcosystemPlan(planManager);
#else
    EOS_CHECK(
      false,
      "Pricing Plan module is not enabled"
    );
#endif
  }
  else {
    auto daoDoc = createDao(nullptr);
    //Verify Root exists
    Document root = Document(get_self(), getRootID());    
    Edge(get_self(), get_self(), root.getID(), daoDoc.getID(), common::DAO);
  }

  //TODO: Activate DAO Plan (?)
}

void dao::createdaodft(ContentGroups &config)
{
  TRACE_FUNCTION();
  EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

  //Extract mandatory configurations
  auto configCW = ContentWrapper(config);

  auto daoName = configCW.getOrFail(DETAILS, DAO_NAME);

  const name dao = daoName->getAs<name>();

  //Check if the requested DAO name already exists, we don't want to
  //add it to the daoName-id table as it's just a draft and could
  //prevent others from creating a real DAO with the same name
  EOS_CHECK(
    !getDAOID(dao),
    to_str(dao, " is a reserved name and can't be used to create a DAO.")
  );
  
  //We must have provided a parent ID as this action
  //is only available for Ecosystem children DAOs
  auto daoParent = configCW.getOrFail(DETAILS, common::DAO_PARENT_ID);
  auto daoParentID = static_cast<uint64_t>(daoParent->getAs<int64_t>());
  checkEcosystemDao(*this, daoParentID);

  checkAdminsAuth(daoParentID);

  Document draftDoc(
    get_self(), 
    get_self(), 
    ContentGroups {
      ContentGroup {
          Content(CONTENT_GROUP_LABEL, DETAILS), 
          Content(DAO_NAME, dao),
      },
      ContentGroup {
          Content(CONTENT_GROUP_LABEL, SYSTEM), 
          Content(TYPE, common::DAO_DRAFT), 
      }
    }
  );

  Edge(get_self(), get_self(), daoParentID, draftDoc.getID(), common::CHILD_DAO_DRAFT);
  Edge(get_self(), get_self(), draftDoc.getID(), daoParentID, common::PARENT_DAO);

  //Extract mandatory configurations
  readDaoSettings(draftDoc.getID(), dao, configCW, true);
}

void dao::deletedaodft(uint64_t dao_draft_id)
{
  TRACE_FUNCTION();

  auto draftDoc = TypedDocument::withType(
    *this, 
    dao_draft_id, 
    common::DAO_DRAFT
  );

  auto draftSettings = getSettingsDocument(dao_draft_id);

  checkAdminsAuth(
    Edge::get(get_self(), draftDoc.getID(), common::PARENT_DAO).getToNode()
  );

  getGraph().eraseDocument(draftDoc.getID());
  getGraph().eraseDocument(draftSettings->getID());
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

  auto cw = recurAct.getContentWrapper();

  auto& state = cw.getOrFail(DETAILS, common::STATE)
                  ->getAs<std::string>();

  if (state == common::STATE_APPROVED) {

    auto forceArchive = recurAct.isInfinite();

    if (forceArchive || 
        recurAct.getEndDate() <= eosio::current_time_point()/* ||
        (!userCalled && forceArchive)*/) {

      auto details = cw.getGroupOrFail(DETAILS);

      state = common::STATE_ARCHIVED;

      recurAct.update();

      auto type = cw.getOrFail(SYSTEM, TYPE)->getAs<name>();

      if (type == common::ASSIGN_BADGE) {
        badges::onBadgeArchived(*this, recurAct);
      }
    }
    //It could happen if the original recurring activity was extended}
    //so reschedule with the new end period
    //Just re-schedule if the action was called by an scheduled transaction
    else if (!userCalled) {
      recurAct.scheduleArchive();
    }
  }
}

// void dao::createroot(const std::string& notes)
// {
//   TRACE_FUNCTION();
//   require_auth(get_self());

//   Document rootDoc(get_self(), get_self(), getRootContent(get_self()));

//   // Create the settings document as well and add an edge to it
//   Settings dhoSettings(*this, rootDoc.getID());

//   addNameID<dao_table>(common::DHO_ROOT_NAME, rootDoc.getID());
// }

void dao::modalerts(uint64_t root_id, ContentGroups& alerts)
{
  //Verify if id belongs to a DAO or DHO
  Document daoDoc(get_self(), root_id);

  auto cw = ContentWrapper(alerts);

  auto type = daoDoc.getContentWrapper()
                    .getOrFail(SYSTEM, TYPE)
                    ->getAs<name>();

  EOS_CHECK(
    type == common::DAO || type == common::DHO,
    "root_id is not valid"
  );

  //Special case when we want to edit Global Alerts
  //we need to check Hypha DAO admin's permissions
  if (root_id == getRootID()) {
    
    auto hyphaID = getDAOID("hypha"_n);

    EOS_CHECK(
      hyphaID.has_value(),
      "Missing hypha DAO entry"
    );

    checkAdminsAuth(*hyphaID);
  }
  else {
    checkAdminsAuth(root_id);
  }
  

  //Check for new alerts
  if (auto [_, addGroup] = cw.getGroup(common::ADD_GROUP);
      addGroup)
  {
    for (auto& newAlert : *addGroup) {
      if (newAlert.label != CONTENT_GROUP_LABEL) {
        //Input format [description;level;enabled]
        auto& alertData = newAlert.getAs<std::string>();

        auto [level, description, enabled] = splitStringView<string, string, int64_t>(alertData, ';');

        EOS_CHECK(
          enabled == 0 || enabled == 1,
          to_str("Invalid value for 'enabled', expected 0 or 1 but got:", enabled)
        );

        Document alert(
          get_self(), get_self(),
          ContentGroups{
            ContentGroup{
                Content(CONTENT_GROUP_LABEL, DETAILS),
                Content(LEVEL, level),
                Content(CONTENT, description),
                Content(ENABLED, enabled)
            },
            ContentGroup{
                Content(CONTENT_GROUP_LABEL, SYSTEM),
                Content(TYPE, common::ALERT),
                Content(NODE_LABEL, "Alert : " + description)
            } 
          }
        );

        Edge(get_self(), get_self(), root_id, alert.getID(), common::ALERT);
        Edge(get_self(), get_self(), alert.getID(), root_id, common::NOTIFIES);
      }
    }
  }

  //Check for updates on existing alerts
  if (auto [_, editGroup] = cw.getGroup(common::EDIT_GROUP);
      editGroup)
  {
    for (auto& editedAlert : *editGroup) {
      if (editedAlert.label != CONTENT_GROUP_LABEL) {
        //Input format [id;description;level;enabled]
        auto& alertData = editedAlert.getAs<std::string>();

        auto [id, level, description, enabled] = splitStringView<int64_t, string, string, int64_t>(alertData, ';');

        EOS_CHECK(
          enabled == 0 || enabled == 1,
          to_str("Invalid value for 'enabled', expected 0 or 1 but got:", enabled)
        );

        Document alert(get_self(), static_cast<uint64_t>(id));

        auto alertCW = alert.getContentWrapper();

        EOS_CHECK(
          alertCW.getOrFail(SYSTEM, TYPE)->getAs<name>() == common::ALERT,
          to_str("Specified id for alert is not valid: ", id, "\n", alertData)
        );

        //Verify the alert belongs to the specified DAO/DHO
        EOS_CHECK(
          Edge::exists(get_self(), alert.getID(), root_id, common::NOTIFIES),
          "Alert doesn't belong to the specified root_id"
        );

        auto details = alertCW.getGroupOrFail(DETAILS);

        alertCW.insertOrReplace(*details, Content(LEVEL, level));
        alertCW.insertOrReplace(*details, Content(CONTENT, description));
        alertCW.insertOrReplace(*details, Content(ENABLED, enabled));

        alert.update();
      }
    }
  }

  //Check removal of existing alerts
  if (auto [_, delGroup] = cw.getGroup(common::DELETE_GROUP);
      delGroup)
  {
    for (auto& delAlert : *delGroup) {
      if (delAlert.label != CONTENT_GROUP_LABEL) {
        
        auto id = delAlert.getAs<int64_t>();

        Document alert(get_self(), static_cast<uint64_t>(id));

        auto alertCW = alert.getContentWrapper();

        EOS_CHECK(
          alertCW.getOrFail(SYSTEM, TYPE)->getAs<name>() == common::ALERT,
          to_str("Specified id for alert is not valid: ", id)
        );

        //Verify the alert belongs to the specified DAO/DHO
        EOS_CHECK(
          Edge::exists(get_self(), alert.getID(), root_id, common::NOTIFIES),
          "Alert doesn't belong to the specified root_id"
        );

        m_documentGraph.eraseDocument(alert.getID(), true);
      }
    }
  }
}

void dao::modsalaryband(uint64_t dao_id, ContentGroups& salary_bands)
{
  verifyDaoType(dao_id);

  checkAdminsAuth(dao_id);

  auto cw = ContentWrapper(salary_bands);

  auto checkParams = [](const string& name, const asset& amount, int64_t minDeferred) {
    EOS_CHECK(
      name.size() < 50,
      "Salary band name is too long"
    )

    EOS_CHECK(
      minDeferred >= 0 && minDeferred <= 100,
      to_str("Invalid value for 'min_deferred_x100', expected value between 0 and 100 but got:", minDeferred)
    );

    EOS_CHECK(
      amount.is_valid() && amount.symbol == common::S_USD,
      "Invalid salary band annual usd amount"
    );
  };

  auto checkSalaryBandDoc = [&](int64_t id) {
    
    Document salaryBand(get_self(), static_cast<uint64_t>(id));

    auto bandCW = salaryBand.getContentWrapper();

    EOS_CHECK(
      bandCW.getOrFail(SYSTEM, TYPE)->getAs<eosio::name>() == common::SALARY_BAND,
      to_str("Specified id for salary band is not valid: ", id)
    );

    //Verify the salary band belongs to the specified DAO
    EOS_CHECK(
      Edge::exists(get_self(), salaryBand.getID(), dao_id, common::DAO),
      "Salary doesn't belong to the specified dao_id"
    );

    return salaryBand;
  };

  //Check for new salary band
  if (auto [_, addGroup] = cw.getGroup(common::ADD_GROUP);
      addGroup)
  {
    for (auto& newBand : *addGroup) {
      if (newBand.label != CONTENT_GROUP_LABEL) {
        //Input format [description;level;enabled]
        auto& bandData = newBand.getAs<std::string>();

        auto [name, amount, minDeferred] = splitStringView<string, asset, int64_t>(bandData, ';');

        checkParams(name, amount, minDeferred);

        Document salaryBand(
          get_self(), get_self(),
          ContentGroups{
            ContentGroup{
                Content(CONTENT_GROUP_LABEL, DETAILS),
                Content(common::SALARY_BAND_NAME, name),
                Content(ANNUAL_USD_SALARY, amount),
                Content(MIN_DEFERRED, minDeferred),
                Content(common::DAO.to_string(), static_cast<int64_t>(dao_id))
            },
            ContentGroup{
                Content(CONTENT_GROUP_LABEL, SYSTEM),
                Content(TYPE, common::SALARY_BAND),
                Content(NODE_LABEL, "Salary Band: " + name)
            } 
          }
        );

        Edge(get_self(), get_self(), dao_id, salaryBand.getID(), common::SALARY_BAND);
        Edge(get_self(), get_self(), salaryBand.getID(), dao_id, common::DAO);
      }
    }
  }

  //Check for updates on existing salary bands
  if (auto [_, editGroup] = cw.getGroup(common::EDIT_GROUP);
      editGroup)
  {
    for (auto& editedBand : *editGroup) {
      if (editedBand.label != CONTENT_GROUP_LABEL) {
        //Input format [id;description;level;enabled]
        auto& bandData = editedBand.getAs<std::string>();

        auto [id, name, amount, minDeferred] = splitStringView<int64_t, string, asset, int64_t>(bandData, ';');

        checkParams(name, amount, minDeferred);

        auto salaryBand = checkSalaryBandDoc(id);

        auto bandCW = salaryBand.getContentWrapper();

        auto details = bandCW.getGroupOrFail(DETAILS);

        bandCW.insertOrReplace(*details, Content(common::SALARY_BAND_NAME, name));
        bandCW.insertOrReplace(*details, Content(ANNUAL_USD_SALARY, amount));
        bandCW.insertOrReplace(*details, Content(MIN_DEFERRED, minDeferred));

        salaryBand.update();
      }
    }
  }

  //Check removal of existing salary bands
  if (auto [_, delGroup] = cw.getGroup(common::DELETE_GROUP);
      delGroup)
  {
    for (auto& delBand : *delGroup) {
      if (delBand.label != CONTENT_GROUP_LABEL) {
        
        auto id = delBand.getAs<int64_t>();

        auto salaryBand = checkSalaryBandDoc(id);

        m_documentGraph.eraseDocument(salaryBand.getID(), true);
      }
    }
  }
}

DocumentGraph& dao::getGraph()
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
void dao::adjustcmtmnt(name issuer, ContentGroups& adjust_info)
{
  TRACE_FUNCTION();
  EOS_CHECK(!isPaused(), "Contract is paused for maintenance. Please try again later.");

  require_auth(issuer);

  ContentWrapper cw(adjust_info);

  for (size_t i = 0; i < adjust_info.size(); ++i)
  {

    Assignment assignment = Assignment(
      this,
      static_cast<uint64_t>(cw.getOrFail(i, "assignment").second->getAs<int64_t>())
    );

    EOS_CHECK(
      assignment.getAssignee().getAccount() == issuer,
      "Only the owner of the assignment can adjust it"
    );

    int64_t newTimeShare = cw.getOrFail(i, NEW_TIME_SHARE).second->getAs<int64_t>();

    auto [modifierIdx, modifier] = cw.get(i, common::ADJUST_MODIFIER);

    string_view modstr = modifier ? string_view{ modifier->getAs<string>() } : string_view{};

    auto [idx, startDate] = cw.get(i, TIME_SHARE_START_DATE);

    auto fixedStartDate = startDate ? std::optional<time_point>{startDate->getAs<time_point>()} : std::nullopt;

    auto assignmentCW = assignment.getContentWrapper();

    if (modstr.empty()) {

      auto lastPeriod = assignment.getLastPeriod();
      auto assignmentExpirationTime = lastPeriod.getEndTime();

      EOS_CHECK(
        assignmentExpirationTime.sec_since_epoch() > eosio::current_time_point().sec_since_epoch(),
        to_str("Cannot adjust expired assignment: ", assignment.getID(), " last period: ", lastPeriod.getID())
      );

      auto state = assignmentCW.getOrFail(DETAILS, common::STATE)->getAs<string>();

      EOS_CHECK(
        state == common::STATE_APPROVED,
        to_str("Cannot adjust commitment for ", state, " assignments")
      );
    }

    modifyCommitment(assignment,
      newTimeShare,
      fixedStartDate,
      modstr);
  }
}

void dao::adjustdeferr(name issuer, uint64_t assignment_id, int64_t new_deferred_perc_x100)
{
  TRACE_FUNCTION();

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
    to_str("Cannot change deferred percentage on", state, " assignments")
  );

  auto approvedDeferredPerc = cw.getOrFail(detailsIdx, common::APPROVED_DEFERRED)
    .second->getAs<int64_t>();

  EOS_CHECK(
    new_deferred_perc_x100 >= approvedDeferredPerc,
    to_str("New percentage has to be greater or equal to approved percentage: ", approvedDeferredPerc)
  );

  const int64_t UPPER_LIMIT = 100;

  EOS_CHECK(
    approvedDeferredPerc <= new_deferred_perc_x100 && new_deferred_perc_x100 <= UPPER_LIMIT,
    to_str("New percentage is out of valid range [",
      approvedDeferredPerc, " - ", UPPER_LIMIT, "]:", new_deferred_perc_x100)
  );

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

void dao::activatebdg(uint64_t assign_badge_id)
{
  RecurringActivity badgeAssing(this, assign_badge_id);
  
  EOS_CHECK(
    //eosio::has_auth(badgeAssing.getAssignee().getAccount()) ||
    eosio::has_auth(get_self()), //For now only contract can activate
    "Only authorized users can activate this badge"
  );

  if (badgeAssing.isInfinite()) {
    badges::onBadgeActivated(*this, badgeAssing);
    return;
  }

  auto now = eosio::current_time_point();

  auto startTime = badgeAssing.getStartDate();

  //We are still within the approved time periods
  if (startTime <= now) {
    if (badgeAssing.getEndDate() >= now) {
      badges::onBadgeActivated(*this, badgeAssing);
    }
  }
  else {
    //Let's reschedule then 
    //Schedule a trx to close the proposal
    eosio::transaction trx;
    trx.actions.emplace_back(eosio::action(
        eosio::permission_level(get_self(), eosio::name("active")),
        get_self(),
        eosio::name("activatebdg"),
        std::make_tuple(badgeAssing.getID())
    ));

    auto activationTime = startTime.sec_since_epoch();

    constexpr auto aditionalDelaySec = 60;
    trx.delay_sec = (activationTime - now.sec_since_epoch()) + aditionalDelaySec;

    auto dhoSettings = getSettingsDocument();

    auto nextID = dhoSettings->getSettingOrDefault("next_schedule_id", int64_t(0));

    trx.send(nextID, get_self());

    dhoSettings->setSetting(Content{"next_schedule_id", nextID + 1});
  }
}

#ifdef USE_PRICING_PLAN

void dao::addtype(uint64_t dao_id, const std::string& dao_type)
{
  auto daoDoc = TypedDocument::withType(*this, dao_id, common::DAO);

  EOS_CHECK(
    dao_type == pricing::common::dao_types::ANCHOR ||
    dao_type == pricing::common::dao_types::ANCHOR_CHILD ||
    dao_type == pricing::common::dao_types::INDIVIDUAL,
    "Invalid DAO type"
  );

  auto daoCW = daoDoc.getContentWrapper();

  daoCW.insertOrReplace(
    *daoCW.getGroupOrFail(DETAILS),
    Content{ common::DAO_TYPE, dao_type }
  );
        
  daoDoc.update();
}

#endif

void dao::modifyCommitment(RecurringActivity& assignment, int64_t commitment, std::optional<eosio::time_point> fixedStartDate, std::string_view modifier)
{
  TRACE_FUNCTION();

  ContentWrapper assignmentCW = assignment.getContentWrapper();

  //Check min_time_share_x100 <= new_time_share_x100 <= time_share_x100
  int64_t originalTimeShare = assignmentCW.getOrFail(DETAILS, TIME_SHARE)->getAs<int64_t>();
  int64_t minTimeShare = 0;

  EOS_CHECK(
    commitment >= minTimeShare,
    to_str(NEW_TIME_SHARE, " must be greater than or equal to: ", minTimeShare, " You submitted: ", commitment)
  );

  EOS_CHECK(
    commitment <= originalTimeShare,
    to_str(NEW_TIME_SHARE, " must be less than or equal to original (approved) time_share_x100: ", originalTimeShare, " You submitted: ", commitment)
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

  EOS_CHECK(
    lastStartDate.sec_since_epoch() < startDate.sec_since_epoch(),
    "New time share start date must be greater than he previous time share start date"
  );

  int64_t lastTimeSharex100 = lastTimeShare.getContentWrapper()
    .getOrFail(DETAILS, TIME_SHARE)
    ->getAs<int64_t>();

  //This allows withdrawing/suspending an assignment which has a current commitment of 0
  if (modifier != common::MOD_WITHDRAW)
  {
    EOS_CHECK(
      lastTimeSharex100 != commitment,
      to_str("New commitment: [", commitment, "] must be different than current commitment: [", lastTimeSharex100, "]")
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

uint64_t dao::getRootID() const
{
  auto rootID = getDAOID(common::DHO_ROOT_NAME);

  //Verify root entry exists
  EOS_CHECK(
    rootID.has_value(),
    to_str("Missing root document entry")
  );

  return *rootID;
}

Member dao::getOrCreateMember(const name& member)
{
  if (Member::exists(*this, member)) {
    return Member(*this, getMemberID(member));
  }
  else {
    return Member(*this, get_self(), member);
  }
}

std::optional<uint64_t> dao::getDAOID(const name& daoName) const
{
  return getNameID<dao_table>(daoName);
}

uint64_t dao::getMemberID(const name& memberName)
{
  auto id = getNameID<member_table>(memberName);

  EOS_CHECK(
    id.has_value(),
    to_str("There is no member with name:", memberName)
  );

  return *id;
}

void dao::verifyDaoType(uint64_t dao_id) 
{
  //Verify dao_id belongs to a DAO
  TypedDocument::withType(*this, dao_id, common::DAO);
}

void dao::checkEnrollerAuth(uint64_t dao_id, const name& account)
{
  TRACE_FUNCTION();

  auto memberID = getMemberID(account);

  EOS_CHECK(
    Edge::exists(get_self(), dao_id, memberID, common::ENROLLER) ||
    Edge::exists(get_self(), dao_id, memberID, common::ADMIN),
    to_str("Only enrollers of the dao are allowed to perform this action")
  );
}

void dao::checkAdminsAuth(uint64_t dao_id)
{
  TRACE_FUNCTION();

  //Contract account has admin perms over all DAO's
  if (eosio::has_auth(get_self())) {
    return;
  }

  auto adminEdges = m_documentGraph.getEdgesFrom(dao_id, common::ADMIN);

  EOS_CHECK(
    std::any_of(adminEdges.begin(), adminEdges.end(), [this](const Edge& adminEdge) {
      Member member(*this, adminEdge.to_node);
      return eosio::has_auth(member.getAccount());
    }),
    to_str("Only admins of the dao are allowed to perform this action")
  );
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
      to_str(daoName, ":", nextPeriodStart.time_since_epoch().count())
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
      eosio::permission_level(get_self(), eosio::name("active")),
      get_self(),
      eosio::name("genperiods"),
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
    eosio::permission_level{ governanceContract, name("active") },
    governanceContract,
    name("create"),
    std::make_tuple(
      daoName,
      get_self(),
      asset{ -getTokenUnit(voiceToken), voiceToken.symbol },
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
    eosio::permission_level{ contract, name("active") },
    contract,
    name("create"),
    std::make_tuple(
      issuer,
      asset{ -getTokenUnit(token), token.symbol }
    )
  ).send();
}

void dao::assigntokdao(asset token, uint64_t dao_id, bool force)
{
  eosio::require_auth(get_self());

  token_to_dao_table tok_t(get_self(), get_self().value);

  auto it = tok_t.find(token.symbol.raw());

  auto insert = [&](TokenToDao& entry){
    entry.id = dao_id;
    entry.token = token;
  };

  {
    auto by_id = tok_t.get_index<"bydocid"_n>();
    auto idIt = by_id.find(dao_id);
    EOS_CHECK(
      force || idIt == by_id.end(),
      to_str("There is already an entry for the specified dao_id: ", idIt->token)
    );
  }

  if (it != tok_t.end()) {
    EOS_CHECK(
      force,
      to_str("There is already an entry for the specified asset: ", it->id)
    );

    tok_t.modify(it, eosio::same_payer, insert);
  }
  else {
    tok_t.emplace(get_self(), insert);
  }
}

void dao::onRewardTransfer(const name& from, const name& to, const asset& amount)
{
  //TODO: Check final balances of both from and to and verify if they need to 
  //get their community membership added or removed or mantained
  if (from != get_self()) {
    updateCommunityMembership(*this, from, amount);
  }

  if (to != get_self()) {
    updateCommunityMembership(*this, to, amount);
  }
}

void dao::on_husd(const name& from, const name& to, const asset& quantity, const string& memo) {

  EOS_CHECK(quantity.amount > 0, "quantity must be > 0");
  EOS_CHECK(quantity.is_valid(), "quantity invalid");

  asset hyphaUsdVal = getSettingOrFail<eosio::asset>(common::HYPHA_USD_VALUE);
  
  EOS_CHECK(
    hyphaUsdVal.symbol.precision() == 4,
    to_str("Expected hypha_usd_value precision to be 4, but got:", hyphaUsdVal.symbol.precision())
  );

  double factor = (hyphaUsdVal.amount / 10000.0);

  EOS_CHECK(common::S_HUSD.precision() == common::S_HYPHA.precision(), "unexpected precision mismatch");

  asset hyphaAmount = asset(quantity.amount / factor, common::S_HYPHA);

  auto hyphaID = getDAOID("hypha"_n);

  EOS_CHECK(
    hyphaID.has_value(),
    "Missing hypha DAO entry"
  );

  auto daoSettings = getSettingsDocument(*hyphaID);

  auto daoTokens = AssetBatch{
    .reward = daoSettings->getOrFail<asset>(common::REWARD_TOKEN),
    .peg = daoSettings->getOrFail<asset>(common::PEG_TOKEN),
    .voice = daoSettings->getOrFail<asset>(common::VOICE_TOKEN)
  };

  std::unique_ptr<Payer> payer = std::unique_ptr<Payer>(PayerFactory::Factory(*this, daoSettings, hyphaAmount.symbol, eosio::name{ 0 }, daoTokens));
  payer->pay(from, hyphaAmount, string("Buy HYPHA for " + quantity.to_string()));
}

void dao::updateDaoURL(name dao, const Content::FlexValue& newURL)
{
  auto newUrlCon = Content{to_str(common::URL, "_", dao), newURL};

  auto globalSettings = getSettingsDocument();
  
  auto globalSetCW = globalSettings->getContentWrapper();

  if (auto [_, urlsGroup] = globalSetCW.getGroup(common::URLS_GROUP); 
      urlsGroup) {
    for (auto& url : *urlsGroup) {
      EOS_CHECK(
        url.label == CONTENT_GROUP_LABEL ||
        url.value != newUrlCon.value,
        to_str("URL is already being used, please use a different one", " ", url.label, " ", url.getAs<std::string>())
      );
    }

    globalSetCW.insertOrReplace(*urlsGroup, std::move(newUrlCon));
  }
  else {
    globalSetCW.getContentGroups()
                .push_back({
                  Content{CONTENT_GROUP_LABEL, common::URLS_GROUP},
                  std::move(newUrlCon)
                });
  }

  globalSettings->update();
}

void dao::changeDecay(Settings* dhoSettings, Settings* daoSettings, uint64_t decayPeriod, uint64_t decayPerPeriod)
{
  auto voiceContract = dhoSettings->getOrFail<name>(GOVERNANCE_TOKEN_CONTRACT);
  
  eosio::action(
    eosio::permission_level{
      voiceContract, 
      eosio::name("active")
    },
    voiceContract, 
    eosio::name("moddecay"),
    std::make_tuple(
      daoSettings->getOrFail<name>(DAO_NAME),
      daoSettings->getOrFail<asset>(common::VOICE_TOKEN).symbol,
      decayPeriod,
      decayPerPeriod
    )
  ).send();
}

void dao::addDefaultSettings(ContentGroup& settingsGroup, const string& daoTitle, const string& daoDescStr)
{
  auto& sg = settingsGroup;

  sg.push_back({ common::DAO_DOCUMENTATION_URL, "" });
  sg.push_back({ common::DAO_DISCORD_URL, "" });
  sg.push_back({ common::DAO_PATTERN, "" });
  sg.push_back({ common::DAO_EXTENDED_LOGO, "" });
  sg.push_back({ common::DAO_PATTERN_COLOR, "#3E3B46" });
  sg.push_back({ common::DAO_PATTERN_OPACITY, 30 });
  sg.push_back({ common::DAO_SPLASH_BACKGROUND_IMAGE, "" });
  sg.push_back({ common::DAO_DASHBOARD_BACKGROUND_IMAGE, "" });
  sg.push_back({ common::DAO_DASHBOARD_TITLE, "Welcome to " + daoTitle });
  sg.push_back({ common::DAO_DASHBOARD_PARAGRAPH, daoDescStr });
  sg.push_back({ "proposals_creation_enabled", 1});
  sg.push_back({ "members_application_enabled", 1});
  sg.push_back({ "removable_banners_enabled", 1});
  sg.push_back({ "multisig_enabled", 0});
  sg.push_back({ common::DAO_PROPOSALS_BACKGROUND_IMAGE, ""});
  sg.push_back({ common::DAO_PROPOSALS_TITLE, "Every vote counts"});
  sg.push_back({ common::DAO_PROPOSALS_PARAGRAPH, "Decentralized decision making is a new kind of governance framework that ensures that decisions are open, just and equitable for all participants. In " + daoTitle + " we use the 80/20 voting method as well as VOICE, our token that determines your voting power. Votes are open for 7 days." });
  sg.push_back({ common::DAO_MEMBERS_BACKGROUND_IMAGE, ""});
  sg.push_back({ common::DAO_MEMBERS_TITLE, "Find & get to know other " + daoTitle + " members"});
  sg.push_back({ common::DAO_MEMBERS_PARAGRAPH, "Learn about what other members are working on, which badges they hold, which DAO's they are part of and much more." });
  sg.push_back({ common::DAO_ORGANISATION_BACKGROUND_IMAGE, ""});
  sg.push_back({ common::DAO_ORGANISATION_TITLE, "Learn everything about " + daoTitle });
  sg.push_back({ common::DAO_ORGANISATION_PARAGRAPH, "Select from a multitude of tools to finetune how the organization works. From treasury and compensation to decision-making, from roles to badges, you have every lever at your fingertips." });
  sg.push_back({ common::ADD_ADMINS_ENABLED, int64_t(1) });
  sg.push_back({ common::CLAIM_ENABLED, int64_t(1) });

}

/*
* @brief Generates settings document for the given DAO
*/
void dao::readDaoSettings(uint64_t daoID, const name& dao, ContentWrapper configCW, bool isDraft, const string& itemsGroup) 
{
  auto [detailsIdx, _] = configCW.getGroup(itemsGroup);

  EOS_CHECK(
    detailsIdx != -1,
    to_str("Missing Details Group")
  );

  auto votingDurationSeconds = configCW.getOrFail(detailsIdx, VOTING_DURATION_SEC).second;

  EOS_CHECK(
    votingDurationSeconds->getAs<int64_t>() > 0,
    to_str(VOTING_DURATION_SEC, " has to be a positive number")
  );

  auto daoDescription = configCW.getOrFail(detailsIdx, common::DAO_DESCRIPTION).second;

  auto daoDescStr = daoDescription->getAs<std::string>();

  EOS_CHECK(
    daoDescStr.size() <= 512,
    "Dao description has be less than 512 characters"
  );

  auto daoTitle = configCW.getOrFail(detailsIdx, common::DAO_TITLE).second;

  auto daoTitleStr = daoTitle->getAs<std::string>();

  EOS_CHECK(
    daoTitleStr.size() <= 48,
    "Dao title has be less than 48 characters"
  );

  auto voiceToken = configCW.getOrFail(detailsIdx, common::VOICE_TOKEN).second;
  auto voiceTokenDecayPeriod = configCW.getOrFail(detailsIdx, common::VOICE_TOKEN_DECAY_PERIOD).second;
  auto voiceTokenDecayPerPeriodX10M = configCW.getOrFail(detailsIdx, common::VOICE_TOKEN_DECAY_PER_PERIOD).second;

  //Generate periods
  //auto inititialPeriods = configCW.getOrFail(detailsIdx, PERIOD_COUNT);

  auto periodDurationSeconds = configCW.getOrFail(detailsIdx, common::PERIOD_DURATION).second;

  EOS_CHECK(
    periodDurationSeconds->getAs<int64_t>() > 0,
    to_str(common::PERIOD_DURATION, " has to be a positive number")
  );

  auto onboarderAcc = configCW.getOrFail(detailsIdx, common::ONBOARDER_ACCOUNT).second;

  const name onboarder = onboarderAcc->getAs<name>();

  //Just do this check on mainnet
  if (!isTestnet()) {

#ifdef EOS_BUILD
    EOS_CHECK(
      onboarder == eosio::name("dao.hypha"),
      "Only authorized account can create DAO"
    )
#else
    auto hyphaId = getDAOID("hypha"_n);

    EOS_CHECK(
      hyphaId.has_value() && Member::isMember(*this, *hyphaId, onboarder),
      "You are not allowed to call this action"
    );
#endif
  }

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

  Content daoURL = Content{ common::DAO_URL, to_str(dao) };;

  if (auto [_, url] = configCW.get(detailsIdx, common::DAO_URL);
      url)
  {
    daoURL.value = std::move(url->value);
  }

  //Verify DAO URL is unique, only if we are actually creating the DAO
  if (!isDraft) {
    updateDaoURL(dao, daoURL.value);
  }

  EOS_CHECK(
    daoURL.getAs<std::string>().size() <= 30,
    "DAO URL must be less than 30 characters"
  )

  const auto styleGroup = itemsGroup == SETTINGS ? SETTINGS : "style";

  auto primaryColor =  configCW.getOrFail(styleGroup, common::DAO_PRIMARY_COLOR);
  primaryColor->getAs<std::string>();

  auto secondaryColor =  configCW.getOrFail(styleGroup, common::DAO_SECONDARY_COLOR);
  secondaryColor->getAs<std::string>();

  auto textColor = configCW.getOrFail(styleGroup, common::DAO_TEXT_COLOR);
  textColor->getAs<std::string>();

  auto logo = configCW.getOrFail(styleGroup, common::DAO_LOGO);
  logo->getAs<std::string>();

  auto settingsGroup = 
  ContentGroup {
    Content(CONTENT_GROUP_LABEL, SETTINGS),
    Content{DAO_NAME, dao},
    std::move(*daoTitle),
    std::move(*daoDescription),
    std::move(daoURL),
    *voiceToken,
    std::move(*periodDurationSeconds),
    std::move(*votingDurationSeconds),
    std::move(*onboarderAcc),
    std::move(*votingQuorum),
    std::move(*votingAllignment),
    *voiceTokenDecayPeriod,
    *voiceTokenDecayPerPeriodX10M,
    Content{common::DAO_USES_SEEDS, useSeeds},
    std::move(*primaryColor),
    std::move(*secondaryColor),
    std::move(*textColor),
    std::move(*logo)
  };

  auto skipPegTok = false;
  auto skipRewardTok = false;

  if (auto [_, skipPeg] = configCW.get(DETAILS, common::SKIP_PEG_TOKEN_CREATION); skipPeg) {
    skipPegTok = skipPeg->getAs<int64_t>() == 1;
  }

  if (auto [_, skipRew] = configCW.get(DETAILS, common::SKIP_REWARD_TOKEN_CREATION); skipRew) {
    skipRewardTok = skipRew->getAs<int64_t>() == 1;
  }

  if (!skipPegTok) {

    auto pegToken = configCW.getOrFail(detailsIdx, common::PEG_TOKEN).second;
    
    Content pegTokenName = Content{  
      common::PEG_TOKEN_NAME,
      to_str(pegToken->getAs<eosio::asset>().symbol.code())
    };

    if (auto [_, pegTokName] = configCW.get(detailsIdx, common::PEG_TOKEN_NAME);
        pegTokName) 
    {
      pegTokenName.value = std::move(pegTokName->value);
    }

    EOS_CHECK(
      pegTokenName.getAs<std::string>().size() <= 30,
      "Cash token name must be less than 30 characters"
    )

    //Don't move the token as it it will be used later on 
    settingsGroup.push_back(*pegToken);
    settingsGroup.push_back(std::move(pegTokenName));
  }

  if (!skipRewardTok) {

    auto rewardToken = configCW.getOrFail(detailsIdx, common::REWARD_TOKEN).second;

    auto rewardToPegTokenRatio = configCW.getOrFail(detailsIdx, common::REWARD_TO_PEG_RATIO).second;

    rewardToPegTokenRatio->getAs<asset>();

    auto rewardTokenMaxSupply = configCW.getOrFail(detailsIdx, "reward_token_max_supply").second;

    EOS_CHECK(
      rewardToken->getAs<asset>().symbol == rewardTokenMaxSupply->getAs<asset>().symbol,
      "Symbol mismatch between reward_token and reward_token_max_supply"
    )

    Content rewardTokenName = Content{  
      common::REWARD_TOKEN_NAME,
      to_str(rewardToken->getAs<eosio::asset>().symbol.code())
    };

    if (auto [_, rwrdTokName] = configCW.get(detailsIdx, common::REWARD_TOKEN_NAME);
        rwrdTokName)
    {
      rewardTokenName.value = std::move(rwrdTokName->value);
    }

    EOS_CHECK(
      rewardTokenName.getAs<std::string>().size() <= 30,
      "Utility token name must be less than 30 characters"
    )

    //Don't move the token as it it will be used later on 
    settingsGroup.push_back(*rewardToken);
    settingsGroup.push_back(std::move(rewardTokenName));
    settingsGroup.push_back(*rewardTokenMaxSupply);
  }

  addDefaultSettings(settingsGroup, daoTitleStr, daoDescStr);

  // Create the settings document as well and add an edge to it
  ContentGroups settingCgs{
      settingsGroup,
      ContentGroup{
          Content(CONTENT_GROUP_LABEL, SYSTEM),
          Content(TYPE, common::SETTINGS_EDGE),
          Content(NODE_LABEL, "Settings")
      }
  };

  Document settingsDoc(get_self(), get_self(), std::move(settingCgs));
  Edge::write(get_self(), get_self(), daoID, settingsDoc.getID(), common::SETTINGS_EDGE);

  //If this is a draft we don't need to do any of the following
  if (isDraft) return;

  createVoiceToken(
    dao,
    voiceToken->getAs<asset>(),
    voiceTokenDecayPeriod->getAs<int64_t>(),
    voiceTokenDecayPerPeriodX10M->getAs<int64_t>()
  );

  auto dhoSettings = getSettingsDocument();
  
  //Check if we should skip creating reward & peg tokens
  //this might be the case if the tokens already exists
  if (!skipPegTok) {
    auto pegToken = configCW.getOrFail(detailsIdx, common::PEG_TOKEN).second;

    //TODO: Add supply

    createToken(
      PEG_TOKEN_CONTRACT,
      dhoSettings->getOrFail<eosio::name>(TREASURY_CONTRACT),
      pegToken->getAs<eosio::asset>()
    );
  }
  // else {
  //   //Only dao.hypha should be able skip creating reward or peg token
  //   eosio::require_auth(get_self());
  // }

  if (!skipRewardTok) {
    auto rewardToken = configCW.getOrFail(detailsIdx, common::REWARD_TOKEN).second;

    //Assign token
    eosio::action(
      eosio::permission_level{ get_self(), name("active") },
      get_self(),
      name("assigntokdao"),
      std::make_tuple(
        rewardToken->getAs<asset>(),
        daoID,
        false
      )
    ).send();
    
    createToken(
      REWARD_TOKEN_CONTRACT,
      get_self(),
      rewardToken->getAs<eosio::asset>()
    );
  }
  // else {
  //   //Only dao.hypha should be able skip creating reward or peg token
  //   eosio::require_auth(get_self());
  // }
}

} // namespace hypha
