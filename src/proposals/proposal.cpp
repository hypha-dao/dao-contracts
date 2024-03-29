#include <eosio/action.hpp>

#include <proposals/proposal.hpp>
#include <ballots/vote.hpp>
#include <ballots/vote_tally.hpp>
#include <document_graph/content_wrapper.hpp>
#include <document_graph/content.hpp>
#include <document_graph/document.hpp>
#include <member.hpp>
#include <common.hpp>
#include <document_graph/edge.hpp>
#include <dao.hpp>
#include <voice/currency_stats.hpp>
#include <util.hpp>
#include <logger/logger.hpp>
#include <recurring_activity.hpp>
#include <comments/section.hpp>
#include <badges/badges.hpp>

using namespace eosio;

namespace hypha
{
    constexpr char COMMENT_NAME[] = "comment_name";

    constexpr name STAGING_PROPOSAL = name("stagingprop");

    Proposal::Proposal(dao &contract, uint64_t daoID)
    : m_dao{contract},
      m_daoSettings(contract.getSettingsDocument(daoID)),
      m_dhoSettings(contract.getSettingsDocument()),
      m_daoID(daoID) {}

    Proposal::~Proposal() {}

    bool Proposal::checkMembership(const eosio::name& proposer, ContentGroups &contentGroups)
    { 
        return Member::isMember(m_dao, m_daoID, proposer);
    }

    Document Proposal::propose(const eosio::name &proposer, ContentGroups &contentGroups, bool publish)
    {
        TRACE_FUNCTION()

        EOS_CHECK(
            // Contract can always create proposal
            proposer == m_dao.get_self() ||
            checkMembership(proposer, contentGroups),
            "Invalid memership for user: " + proposer.to_string()
        );
        return this->internalPropose(proposer, contentGroups, publish, nullptr);
    }

    Document Proposal::internalPropose(const eosio::name &proposer, ContentGroups &contentGroups, bool publish, Section* commentSection)
    {
        ContentWrapper proposalContent(contentGroups);
        proposeImpl(proposer, proposalContent);

        const std::string title = getTitle(proposalContent);

        // Suspend proposal's title is based on the original proposal title, skip the following check for suspend proposals.
        if (getProposalType() != common::SUSPEND) {
            //Verify title length is less or equal than 50 chars.
            EOS_CHECK(
                title.length() <= common::MAX_PROPOSAL_TITLE_CHARS,
                to_str("Proposal title length has to be less or equal to ", common::MAX_PROPOSAL_TITLE_CHARS, " characters")
            )
        }

        const std::string description = getDescription(proposalContent);
        
         //Verify description lenght is less or equal than 50 chars
        EOS_CHECK(
            description.length() <= common::MAX_PROPOSAL_DESC_CHARS,
            to_str("Proposal description length has to be less or equal to ", common::MAX_PROPOSAL_DESC_CHARS, " characters")
        )

        EOS_CHECK(
            proposalContent.getGroup(SYSTEM).second == nullptr,
            "System group must not be specified"
        )

        contentGroups.push_back(makeSystemGroup(proposer,
                                                getProposalType(),
                                                title,
                                                description));

        ContentWrapper::insertOrReplace(*proposalContent.getGroupOrFail(DETAILS),
                                        Content { common::STATE,
                                                  publish ? common::STATE_PROPOSED : common::STATE_DRAFTED });

        ContentWrapper::insertOrReplace(*proposalContent.getGroupOrFail(DETAILS),
                                        Content { common::DAO.to_string(),
                                                  static_cast<int64_t>(m_daoID) });

        if (selfApprove) {
            ContentWrapper::insertOrReplace(*proposalContent.getGroupOrFail(SYSTEM),
                                            Content { common::CATEGORY_SELF_APPROVED, 1 });
        }

        Document proposalNode(m_dao.get_self(), proposer, contentGroups);

        // Creates comment section
        if (commentSection == nullptr) {
            Section(m_dao, proposalNode);
        } else {
            commentSection->move(proposalNode);
        }

        // creates the document, or the graph NODE
        auto memberID = m_dao.getMemberID(proposer);

        uint64_t root = m_daoID;

        // the proposer OWNS the proposal; this creates the graph EDGE
        Edge::write(m_dao.get_self(), proposer, memberID, proposalNode.getID (), common::OWNS);

        // the proposal was PROPOSED_BY proposer; this creates the graph EDGE
        Edge::write(m_dao.get_self(), proposer, proposalNode.getID (), memberID, common::OWNED_BY);

        Edge::write(m_dao.get_self(), proposer, proposalNode.getID (), root, common::DAO);

        Edge::write(m_dao.get_self(), proposer, root, proposalNode.getID(), common::VOTABLE);

        postProposeImpl(proposalNode);

        if (publish) {
            _publish(proposer, proposalNode, root);
        } else {
            Edge::write(m_dao.get_self(), proposer, root, proposalNode.getID(), STAGING_PROPOSAL);
        }

        if (selfApprove) {
            internalClose(proposalNode, true);
        }

        return proposalNode;
    }

    void Proposal::postProposeImpl(Document &proposal) {}

    void Proposal::vote(const eosio::name &voter, const std::string vote, Document& proposal, std::optional<std::string> notes)
    {
        TRACE_FUNCTION()
        
        EOS_CHECK(
            Edge::exists(m_dao.get_self(), m_daoID, proposal.getID(), common::PROPOSAL), 
            "Only published proposals can be voted"
        );

        EOS_CHECK(
            Member::isMember(m_dao, m_daoID, voter),
            "Only members of this DAO can vote proposals: " + voter.to_string()
        );

        //TODO: Add parameter to check voting type (community or core)
        Vote(m_dao, voter, vote, proposal, notes);
        
        VoteTally(m_dao, proposal, m_daoSettings);
    }

    void Proposal::internalClose(Document &proposal, bool pass)
    {
        auto details = proposal.getContentWrapper().getGroupOrFail(DETAILS);

        ContentWrapper::insertOrReplace(*details, Content{
          common::STATE,
          pass ? common::STATE_APPROVED : common::STATE_REJECTED
        });

        //Since getting supply might be a heavy computaion we store it in other places, so if that's the case
        //don't calculate it again
        if (!proposal.getContentWrapper().exists(DETAILS, common::BALLOT_SUPPLY)) {
            ContentWrapper::insertOrReplace(*details, Content {
                common::BALLOT_SUPPLY,
                getVoiceSupply(proposal)
            });
        }

        ContentWrapper::insertOrReplace(*details, Content {
            common::BALLOT_QUORUM,
            m_daoSettings->getOrFail<int64_t>(VOTING_QUORUM_FACTOR_X100)
        });

        ContentWrapper::insertOrReplace(*details, Content {
            common::BALLOT_ALIGNMENT,
            m_daoSettings->getOrFail<int64_t>(VOTING_ALIGNMENT_FACTOR_X100)
        });

        Edge edge = Edge::get(m_dao.get_self(), m_daoID, proposal.getID (), common::PROPOSAL);
        edge.erase();

        if (pass)
        {
            auto system = proposal.getContentWrapper().getGroupOrFail(SYSTEM);
            
            ContentWrapper::insertOrReplace(*system, Content{
              common::APPROVED_DATE,
              eosio::current_time_point()
            });

            // INVOKE child class close logic
            passImpl(proposal);

            proposal.update();
            // if proposal passes, create an edge for PASSED_PROPS
            Edge::write(m_dao.get_self(), m_dao.get_self(), m_daoID, proposal.getID (), common::PASSED_PROPS);

            if (isRecurring()) {
                RecurringActivity recurAct(&m_dao, proposal.getID());
                recurAct.scheduleArchive();
            }
        }
        else
        {
            failImpl(proposal);

            proposal.update();

            // create edge for FAILED_PROPS
            Edge::write(m_dao.get_self(), m_dao.get_self(), m_daoID, proposal.getID (), common::FAILED_PROPS);
        }

        Edge::write(m_dao.get_self(), m_dao.get_self(), m_daoID, proposal.getID (), common::CLOSED_PROPS);
    }

    void Proposal::close(Document &proposal)
    {
        TRACE_FUNCTION()

        EOS_CHECK(
            Edge::exists(m_dao.get_self(), m_daoID, proposal.getID(), common::PROPOSAL), 
            "Only published proposals can be closed"
        );

        auto voteTallyEdge = Edge::get(m_dao.get_self(), proposal.getID (), common::VOTE_TALLY);

        auto expiration = proposal.getContentWrapper().getOrFail(BALLOT, EXPIRATION_LABEL, "Proposal has no expiration")->getAs<eosio::time_point>();
        EOS_CHECK(
            eosio::time_point_sec(eosio::current_time_point()) > expiration,
            "Voting is still active for this proposal"
        );

        bool proposalDidPass;

        auto vetoByEdges = m_dao.getGraph()
                                .getEdgesFrom(proposal.getID(), common::VETO_BY);
        
        auto ballotID = voteTallyEdge.getToNode();
        
        //Currently if 2 North Start badge holders veto the proposal
        //it should not pass
        proposalDidPass = (vetoByEdges.size() < 2) && didPass(proposal, ballotID);

        internalClose(proposal, proposalDidPass);
    }

    void Proposal::publish(const eosio::name &proposer, Document &proposal)
    {
        TRACE_FUNCTION()
        
        auto ownerID =  Edge::get(m_dao.get_self(), proposal.getID (), common::OWNED_BY).getToNode();

        Member memberDoc(m_dao, ownerID);

        EOS_CHECK(
            Edge::exists(m_dao.get_self(), m_daoID, proposal.getID(), STAGING_PROPOSAL), 
            "Only proposes in staging can be published"
        );

        EOS_CHECK(
            proposer == memberDoc.getAccount(), 
            "Only the proposer can publish the proposal"
        );
        
        Edge::get(m_dao.get_self(), m_daoID, proposal.getID(), STAGING_PROPOSAL).erase();
        _publish(proposer, proposal, m_daoID);
    }

    void Proposal::remove(const eosio::name &proposer, Document &proposal)
    {
        TRACE_FUNCTION()
        
        auto ownerID =  Edge::get(m_dao.get_self(), proposal.getID (), common::OWNED_BY).getToNode();

        Member memberDoc(m_dao, ownerID);

        EOS_CHECK(
            Edge::exists(m_dao.get_self(), m_daoID, proposal.getID(), STAGING_PROPOSAL), 
            "Only proposes in staging can be removed"
        );

        EOS_CHECK(
            proposer == memberDoc.getAccount(), 
            "Only the proposer can remove the proposal"
        );

        Section commentSection(m_dao, Edge::get(m_dao.get_self(), proposal.getID(), common::COMMENT_SECTION).getToNode());
        commentSection.remove();

        m_dao.getGraph().eraseDocument(proposal.getID(), true);
    }

    void Proposal::update(const eosio::name &proposer, Document &proposal, ContentGroups &contentGroups)
    {
        TRACE_FUNCTION()

        auto ownerID =  Edge::get(m_dao.get_self(), proposal.getID (), common::OWNED_BY).getToNode();

        Member memberDoc(m_dao, ownerID);

        EOS_CHECK(
            Edge::exists(m_dao.get_self(), m_daoID, proposal.getID(), STAGING_PROPOSAL),
            "Only proposes in staging can be updated"
        );

        EOS_CHECK(
            proposer == memberDoc.getAccount(),
            "Only the proposer can update the proposal"
        );

        Section commentSection(m_dao, Edge::get(m_dao.get_self(), proposal.getID(), common::COMMENT_SECTION).getToNode());

        this->internalPropose(proposer, contentGroups, false, &commentSection);
        m_dao.getGraph().eraseDocument(proposal.getID(), true);
    }

    ContentGroup Proposal::makeSystemGroup(const name &proposer,
                                           const name &proposal_type,
                                           const string &proposal_title,
                                           const string &proposal_description)
    {
        return ContentGroup{
            Content(CONTENT_GROUP_LABEL, SYSTEM),
            Content(CLIENT_VERSION, m_dhoSettings->getSettingOrDefault<std::string>(CLIENT_VERSION, DEFAULT_VERSION)),
            Content(CONTRACT_VERSION, m_dhoSettings->getSettingOrDefault<std::string>(CONTRACT_VERSION, DEFAULT_VERSION)),
            Content(NODE_LABEL, proposal_title),
            Content(DESCRIPTION, proposal_description),
            Content(TYPE, proposal_type)};
    }

    ContentGroup Proposal::makeBallotGroup()
    {
        TRACE_FUNCTION()
        auto expiration = time_point_sec(current_time_point()) + m_daoSettings->getOrFail<int64_t>(VOTING_DURATION_SEC);
        return ContentGroup{
            Content(CONTENT_GROUP_LABEL, BALLOT),
            Content(EXPIRATION_LABEL, expiration)
        };
    }

    ContentGroup Proposal::makeBallotOptionsGroup()
    {
        return ContentGroup{
            Content(CONTENT_GROUP_LABEL, BALLOT_OPTIONS),
            Content(common::BALLOT_DEFAULT_OPTION_PASS.to_string(), common::BALLOT_DEFAULT_OPTION_PASS),
            Content(common::BALLOT_DEFAULT_OPTION_ABSTAIN.to_string(), common::BALLOT_DEFAULT_OPTION_ABSTAIN),
            Content(common::BALLOT_DEFAULT_OPTION_FAIL.to_string(), common::BALLOT_DEFAULT_OPTION_FAIL)
        };
    }

    bool Proposal::didPass(Document& proposal, uint64_t tallyID)
    {
        TRACE_FUNCTION()

        int64_t quorumFactor = m_daoSettings->getOrFail<int64_t>(VOTING_QUORUM_FACTOR_X100);
        int64_t alignmentFactor = m_daoSettings->getOrFail<int64_t>(VOTING_ALIGNMENT_FACTOR_X100);

        //quorumFactor = std::max(0.00f, std::min(1.00f, quorumFactor));
        //alignmentFactor = std::max(0.00f, std::min(1.00f, alignmentFactor));

        const int64_t maxFactor = 100;

        EOS_CHECK(
            quorumFactor >= 0 && quorumFactor <= maxFactor,
            to_str("Quorum Factor out of valid range", quorumFactor)
        );

        EOS_CHECK(
            alignmentFactor >= 0 && alignmentFactor <= maxFactor,
            to_str("Alginment Factor out of valid range", alignmentFactor)
        );

        asset voiceSupply;

        voiceSupply = getVoiceSupply(proposal);

        auto cw = proposal.getContentWrapper();

        ContentWrapper::insertOrReplace(
            *cw.getGroupOrFail(DETAILS), 
            Content {
                common::BALLOT_SUPPLY,
                voiceSupply
            }
        );
        
        asset quorum_threshold = voiceSupply * quorumFactor;

        VoteTally tally(m_dao, tallyID);

        //TODO: Check for North Badge veto

        // Currently get pass/fail
        // Todo: Abstract this part into VoteTally class
        asset votes_pass = tally.getDocument().getContentWrapper().getOrFail(common::BALLOT_DEFAULT_OPTION_PASS.to_string(), VOTE_POWER)->getAs<eosio::asset>();
        asset votes_abstain = tally.getDocument().getContentWrapper().getOrFail(common::BALLOT_DEFAULT_OPTION_ABSTAIN.to_string(), VOTE_POWER)->getAs<eosio::asset>();
        asset votes_fail = tally.getDocument().getContentWrapper().getOrFail(common::BALLOT_DEFAULT_OPTION_FAIL.to_string(), VOTE_POWER)->getAs<eosio::asset>();

        asset total = votes_pass + votes_abstain + votes_fail;

        // pass / ( pass + fail ) > alignmentFactor
        bool passesAlignment = (votes_pass * maxFactor) >= ((votes_pass + votes_fail) * alignmentFactor);
        bool passQuorum = (total * maxFactor) >= quorum_threshold;

        return passQuorum && passesAlignment;
    }
    string Proposal::getTitle(ContentWrapper cw) const
    {
        TRACE_FUNCTION()
        auto [titleIdx, title] = cw.get(DETAILS, TITLE);

        auto [ballotTitleIdx, ballotTitle] = cw.get(DETAILS, common::BALLOT_TITLE);

        EOS_CHECK(
          title != nullptr || ballotTitle != nullptr,
          to_str("Proposal [details] group must contain at least one of the following items [", 
                  TITLE, ", ", common::BALLOT_TITLE, "]")
        );

        return title != nullptr ? title->getAs<std::string>() : 
                                  ballotTitle->getAs<std::string>();
    }

    string Proposal::getDescription(ContentWrapper cw) const
    {
        TRACE_FUNCTION()
        
        auto [descIdx, desc] = cw.get(DETAILS, DESCRIPTION);

        auto [ballotDescIdx, ballotDesc] = cw.get(DETAILS, common::BALLOT_DESCRIPTION);

        EOS_CHECK(
          desc != nullptr || ballotDesc != nullptr,
          to_str("Proposal [details] group must contain at least one of the following items [", 
                  DESCRIPTION, ", ", common::BALLOT_DESCRIPTION, "]")
        );

        return desc != nullptr ? desc->getAs<std::string>() : 
                                 ballotDesc->getAs<std::string>();
    }

    std::pair<bool, uint64_t> Proposal::hasOpenProposal(name proposalType, uint64_t docID)
    {
      TRACE_FUNCTION()
      auto proposalEdges = m_dao.getGraph().getEdgesTo(docID, proposalType);
      
      //Check if there is another existing suspend proposal open for the given document
      for (auto& edge : proposalEdges) {
        Document proposal(m_dao.get_self(), edge.getFromNode());

        auto cw = proposal.getContentWrapper();
        if (cw.getOrFail(DETAILS, common::STATE)->getAs<string>() == common::STATE_PROPOSED) {
          return { true,  edge.getFromNode() };
        }
      }

      return { false, uint64_t{} };
    }

    eosio::asset Proposal::getVoiceSupply(Document& proposal)
    {
        asset voiceToken = m_daoSettings->getOrFail<eosio::asset>(common::VOICE_TOKEN);

        //If community voting is enabled supply should be equal to amount of members (both community and core)
        if (auto [_, communityVote] = proposal.getContentWrapper().get(SYSTEM, common::COMMUNITY_VOTING);
            communityVote && communityVote->getAs<int64_t>()) {
            
            //Get total amount of members and community members
            auto totalMembers = Edge::getEdgesFromCount(m_dao.get_self(), m_daoID, common::MEMBER);
            totalMembers += Edge::getEdgesFromCount(m_dao.get_self(), m_daoID, common::COMMEMBER);
            
            return denormalizeToken(static_cast<double>(totalMembers), voiceToken);
        }

        name voiceContract = m_dhoSettings->getOrFail<eosio::name>(GOVERNANCE_TOKEN_CONTRACT);
        hypha::voice::stats statstable(voiceContract, voiceToken.symbol.code().raw());
        auto stats_index = statstable.get_index<name("bykey")>();

        auto daoName = m_daoSettings->getOrFail<name>(DAO_NAME);

        auto stat_itr = stats_index.find(
            voice::currency_statsv2::build_key(
                daoName,
                voiceToken.symbol.code()
            )
        );

        EOS_CHECK(stat_itr != stats_index.end(), to_str("No VOICE found token: ", voiceToken, " DAO: ", daoName));

        return stat_itr->supply;
    }

    void Proposal::_publish(const eosio::name &proposer, Document &proposal, uint64_t rootID)
    {
        TRACE_FUNCTION()
        // the DHO also links to the document as a proposal, another graph EDGE
        Edge::write(m_dao.get_self(), proposer, rootID, proposal.getID (), common::PROPOSAL);

        proposal.getContentGroups().push_back(makeBallotGroup());
        proposal.getContentGroups().push_back(makeBallotOptionsGroup());

        // Sets an empty tally
        VoteTally(m_dao, proposal, m_daoSettings);

        ContentWrapper::insertOrReplace(
            *proposal.getContentWrapper().getGroupOrFail(DETAILS),
            Content { common::STATE, common::STATE_PROPOSED }
        );

        publishImpl(proposal);

        proposal.update();
        
        //Schedule a trx to close the proposal
        eosio::action act(
            permission_level(m_dao.get_self(), eosio::name("active")),
            m_dao.get_self(),
            eosio::name("closedocprop"),
            std::make_tuple(proposal.getID())
        );

        auto expiration = proposal.getContentWrapper().getOrFail(BALLOT, EXPIRATION_LABEL, "Proposal has no expiration")->getAs<eosio::time_point>();

        m_dao.schedule_deferred_action(expiration + eosio::seconds(4), act);

    }

    std::optional<Document> Proposal::getItemDocOpt(const char* docItem, const name& docType, ContentWrapper &contentWrapper)
    {
        if (auto [_, item] = contentWrapper.get(DETAILS, docItem);
            item) 
        {
            auto doc = TypedDocument::withType(m_dao, item->getAs<int64_t>(), docType);
            
            //Verify that Document is approved
            auto docCW = doc.getContentWrapper();

            auto state = docCW.get(DETAILS, common::STATE).second;

            //Only approved documents can be used as relatives
            EOS_CHECK(
                state == nullptr ||
                state->getAs<string>() == common::STATE_APPROVED,
                "Only approved Documents can be used as reference"
            );

            return doc;
        }

        return std::nullopt;
    }

    Document Proposal::getItemDoc(const char* docItem, const name& docType, ContentWrapper &contentWrapper)
    {
        auto docOpt = getItemDocOpt(docItem, docType, contentWrapper);

        EOS_CHECK(
            docOpt,
            to_str("Required item not found: ", docItem)
        )

        return std::move(*docOpt);
    }

    Document Proposal::getLinkDoc(uint64_t from, const name& link, const name& docType)
    {
        auto linkEdge = Edge::get(m_dao.get_self(), from, link);

        auto doc = TypedDocument::withType(m_dao, linkEdge.getToNode(), docType);

        return doc;
    }
} // namespace hypha
