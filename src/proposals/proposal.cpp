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

using namespace eosio;

namespace hypha
{
    constexpr char COMMENT_NAME[]         = "comment_name";
    constexpr char NEXT_COMMENT_SECTION[] = "next_comment_section";
    //constexpr name COMMENTS_CONTRACT("comments");

    constexpr name STAGING_PROPOSAL = name("stagingprop");

    Proposal::Proposal(dao&contract, uint64_t daoID)
        : m_dao{contract},
        m_daoSettings(contract.getSettingsDocument(daoID)),
        m_dhoSettings(contract.getSettingsDocument()),
        m_daoID(daoID)
    {
    }

    Proposal::~Proposal()
    {
    }

    Document Proposal::propose(const eosio::name&proposer, ContentGroups&contentGroups, bool publish)
    {
        TRACE_FUNCTION()
        EOS_CHECK(Member::isMember(m_dao, m_daoID, proposer), "only members can make proposals: " + proposer.to_string());
        ContentWrapper proposalContent(contentGroups);

        proposeImpl(proposer, proposalContent);

        const name commentSection = _newCommentSection();

        const std::string title = getTitle(proposalContent);

        // Suspend proposal's title is based on the original proposal title, skip the following check for suspend proposals.
        if (getProposalType() != common::SUSPEND)
        {
            //Verify title length is less or equal than 50 chars.
            EOS_CHECK(
                title.length() <= common::MAX_PROPOSAL_TITLE_CHARS,
                util::to_str("Proposal title length has to be less or equal to ", common::MAX_PROPOSAL_TITLE_CHARS, " characters")
                )
        }

        const std::string description = getDescription(proposalContent);

        //Verify description lenght is less or equal than 50 chars
        EOS_CHECK(
            description.length() <= common::MAX_PROPOSAL_DESC_CHARS,
            util::to_str("Proposal description length has to be less or equal to ", common::MAX_PROPOSAL_DESC_CHARS, " characters")
            )

        contentGroups.push_back(makeSystemGroup(proposer,
                                                getProposalType(),
                                                title,
                                                description,
                                                commentSection));

        contentGroups.push_back(makeBallotGroup());
        contentGroups.push_back(makeBallotOptionsGroup());

        ContentWrapper::insertOrReplace(*proposalContent.getGroupOrFail(DETAILS),
                                        Content { common::STATE, common::STATE_PROPOSED });

        Document proposalNode(m_dao.get_self(), proposer, contentGroups);

        // creates the document, or the graph NODE
        auto memberID = m_dao.getMemberID(proposer);

        uint64_t root = m_daoID;

        // the proposer OWNS the proposal; this creates the graph EDGE
        Edge::write(m_dao.get_self(), proposer, memberID, proposalNode.getID(), common::OWNS);

        // the proposal was PROPOSED_BY proposer; this creates the graph EDGE
        Edge::write(m_dao.get_self(), proposer, proposalNode.getID(), memberID, common::OWNED_BY);

        name commentsContract = m_dhoSettings->getOrFail <eosio::name>(COMMENTS_CONTRACT);

        eosio::action(
            eosio::permission_level{ m_dao.get_self(), name("active") },
            commentsContract, name("addsection"),
            std::make_tuple(
                m_dao.get_self(),
                m_daoSettings->getOrFail <name>(DAO_NAME), // scope. todo: use tenant here
                commentSection,                            // section
                proposer                                   // author
                )
            ).send();

        if (publish)
        {
            _publish(proposer, proposalNode, root);
        }
        else
        {
            Edge::write(m_dao.get_self(), proposer, root, proposalNode.getID(), STAGING_PROPOSAL);
        }

        Edge::write(m_dao.get_self(), proposer, proposalNode.getID(), root, common::DAO);

        Edge::write(m_dao.get_self(), proposer, root, proposalNode.getID(), common::VOTABLE);

        return(proposalNode);
    }

    void Proposal::postProposeImpl(Document&proposal)
    {
    }

    void Proposal::vote(const eosio::name&voter, const std::string vote, Document& proposal, std::optional <std::string> notes)
    {
        TRACE_FUNCTION()

        EOS_CHECK(
            Edge::exists(m_dao.get_self(), m_daoID, proposal.getID(), common::PROPOSAL),
            "Only published proposals can be voted"
            );

        Vote(m_dao, voter, vote, proposal, notes);

        VoteTally(m_dao, proposal, m_daoSettings);
    }

    void Proposal::close(Document&proposal)
    {
        TRACE_FUNCTION()

        EOS_CHECK(
            Edge::exists(m_dao.get_self(), m_daoID, proposal.getID(), common::PROPOSAL),
            "Only published proposals can be closed"
            );

        auto voteTallyEdge = Edge::get(m_dao.get_self(), proposal.getID(), common::VOTE_TALLY);

        auto expiration = proposal.getContentWrapper().getOrFail(BALLOT, EXPIRATION_LABEL, "Proposal has no expiration")->getAs <eosio::time_point>();

        EOS_CHECK(
            eosio::time_point_sec(eosio::current_time_point()) > expiration,
            "Voting is still active for this proposal"
            );

        Edge edge = Edge::get(m_dao.get_self(), m_daoID, proposal.getID(), common::PROPOSAL);

        edge.erase();

        bool proposalDidPass;

        auto ballotID = voteTallyEdge.getToNode();

        proposalDidPass = didPass(ballotID);

        auto details = proposal.getContentWrapper().getGroupOrFail(DETAILS);

        ContentWrapper::insertOrReplace(*details, Content{
            common::STATE,
            proposalDidPass ? common::STATE_APPROVED : common::STATE_REJECTED
        });

        ContentWrapper::insertOrReplace(*details, Content {
            common::BALLOT_QUORUM,
            getVoiceSupply()
        });

        if (proposalDidPass)
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
            Edge::write(m_dao.get_self(), m_dao.get_self(), m_daoID, proposal.getID(), common::PASSED_PROPS);
        }
        else
        {
            //TODO: Add failImpl()
            proposal.update();

            // create edge for FAILED_PROPS
            Edge::write(m_dao.get_self(), m_dao.get_self(), m_daoID, proposal.getID(), common::FAILED_PROPS);
        }
    }

    void Proposal::publish(const eosio::name&proposer, Document&proposal)
    {
        TRACE_FUNCTION()

        auto ownerID = Edge::get(m_dao.get_self(), proposal.getID(), common::OWNED_BY).getToNode();

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

    void Proposal::remove(const eosio::name&proposer, Document&proposal)
    {
        TRACE_FUNCTION()

        auto ownerID = Edge::get(m_dao.get_self(), proposal.getID(), common::OWNED_BY).getToNode();

        Member memberDoc(m_dao, ownerID);

        EOS_CHECK(
            Edge::exists(m_dao.get_self(), m_daoID, proposal.getID(), STAGING_PROPOSAL),
            "Only proposes in staging can be removed"
            );

        EOS_CHECK(
            proposer == memberDoc.getAccount(),
            "Only the proposer can remove the proposal"
            );

        m_dao.getGraph().eraseDocument(proposal.getID(), true);

        name commentsContract = m_dhoSettings->getOrFail <eosio::name>(COMMENTS_CONTRACT);

        eosio::action(
            eosio::permission_level{ this->m_dao.get_self(), name("active") },
            commentsContract, name("delsection"),
            std::make_tuple(
                m_dao.get_self(),
                m_daoSettings->getOrFail <name>(DAO_NAME),
                proposal.getContentWrapper().getOrFail(SYSTEM, COMMENT_NAME, "Proposal has no comment section")->getAs <eosio::name>() // section
                )
            ).send();
    }

    void Proposal::update(const eosio::name&proposer, Document&proposal, ContentGroups&contentGroups)
    {
        TRACE_FUNCTION()

        auto ownerID = Edge::get(m_dao.get_self(), proposal.getID(), common::OWNED_BY).getToNode();

        Member memberDoc(m_dao, ownerID);

        EOS_CHECK(
            Edge::exists(m_dao.get_self(), m_daoID, proposal.getID(), STAGING_PROPOSAL),
            "Only proposes in staging can be updated"
            );

        EOS_CHECK(
            proposer == memberDoc.getAccount(),
            "Only the proposer can update the proposal"
            );

        m_dao.getGraph().eraseDocument(proposal.getID(), true);

        propose(proposer, contentGroups, false);
    }

    ContentGroup Proposal::makeSystemGroup(const name&proposer,
                                           const name&proposal_type,
                                           const string&proposal_title,
                                           const string&proposal_description,
                                           const name&comment_name)
    {
        return(ContentGroup{
            Content(CONTENT_GROUP_LABEL, SYSTEM),
            Content(CLIENT_VERSION, m_dhoSettings->getSettingOrDefault <std::string>(CLIENT_VERSION, DEFAULT_VERSION)),
            Content(CONTRACT_VERSION, m_dhoSettings->getSettingOrDefault <std::string>(CONTRACT_VERSION, DEFAULT_VERSION)),
            Content(NODE_LABEL, proposal_title),
            Content(DESCRIPTION, proposal_description),
            Content(TYPE, proposal_type),
            Content(COMMENT_NAME, comment_name) });
    }

    ContentGroup Proposal::makeBallotGroup()
    {
        TRACE_FUNCTION()
        auto expiration = time_point_sec(current_time_point()) + m_daoSettings->getOrFail <int64_t>(VOTING_DURATION_SEC);

        return(ContentGroup{
            Content(CONTENT_GROUP_LABEL, BALLOT),
            Content(EXPIRATION_LABEL, expiration)
        });
    }

    ContentGroup Proposal::makeBallotOptionsGroup()
    {
        return(ContentGroup{
            Content(CONTENT_GROUP_LABEL, BALLOT_OPTIONS),
            Content(common::BALLOT_DEFAULT_OPTION_PASS.to_string(), common::BALLOT_DEFAULT_OPTION_PASS),
            Content(common::BALLOT_DEFAULT_OPTION_ABSTAIN.to_string(), common::BALLOT_DEFAULT_OPTION_ABSTAIN),
            Content(common::BALLOT_DEFAULT_OPTION_FAIL.to_string(), common::BALLOT_DEFAULT_OPTION_FAIL)
        });
    }

    bool Proposal::didPass(uint64_t tallyID)
    {
        TRACE_FUNCTION()

        float quorumFactor = m_daoSettings->getOrFail <int64_t>(VOTING_QUORUM_FACTOR_X100) / 100.0f;
        float alignmentFactor = m_daoSettings->getOrFail <int64_t>(VOTING_ALIGNMENT_FACTOR_X100) / 100.0f;

        auto voiceSupply = getVoiceSupply();

        asset quorum_threshold = adjustAsset(voiceSupply, quorumFactor);

        VoteTally tally(m_dao, tallyID);

        // Currently get pass/fail
        // Todo: Abstract this part into VoteTally class
        asset votes_pass    = tally.getDocument().getContentWrapper().getOrFail(common::BALLOT_DEFAULT_OPTION_PASS.to_string(), VOTE_POWER)->getAs <eosio::asset>();
        asset votes_abstain = tally.getDocument().getContentWrapper().getOrFail(common::BALLOT_DEFAULT_OPTION_ABSTAIN.to_string(), VOTE_POWER)->getAs <eosio::asset>();
        asset votes_fail    = tally.getDocument().getContentWrapper().getOrFail(common::BALLOT_DEFAULT_OPTION_FAIL.to_string(), VOTE_POWER)->getAs <eosio::asset>();

        asset total = votes_pass + votes_abstain + votes_fail;
        // pass / ( pass + fail ) > alignmentFactor
        bool passesAlignment = votes_pass > (alignmentFactor * (votes_pass + votes_fail));

        if (total >= quorum_threshold &&      // must meet quorum
            passesAlignment)
        {
            return(true);
        }
        else
        {
            return(false);
        }
    }

    string Proposal::getTitle(ContentWrapper cw) const
    {
        TRACE_FUNCTION()
        auto [titleIdx, title] = cw.get(DETAILS, TITLE);

        auto [ballotTitleIdx, ballotTitle] = cw.get(DETAILS, common::BALLOT_TITLE);

        EOS_CHECK(
            title != nullptr || ballotTitle != nullptr,
            util::to_str("Proposal [details] group must contain at least one of the following items [",
                         TITLE, ", ", common::BALLOT_TITLE, "]")
            );

        return(title != nullptr ? title->getAs <std::string>() :
               ballotTitle->getAs <std::string>());
    }

    string Proposal::getDescription(ContentWrapper cw) const
    {
        TRACE_FUNCTION()

        auto [descIdx, desc] = cw.get(DETAILS, DESCRIPTION);

        auto [ballotDescIdx, ballotDesc] = cw.get(DETAILS, common::BALLOT_DESCRIPTION);

        EOS_CHECK(
            desc != nullptr || ballotDesc != nullptr,
            util::to_str("Proposal [details] group must contain at least one of the following items [",
                         DESCRIPTION, ", ", common::BALLOT_DESCRIPTION, "]")
            );

        return(desc != nullptr ? desc->getAs <std::string>() :
               ballotDesc->getAs <std::string>());
    }

    std::pair <bool, uint64_t> Proposal::hasOpenProposal(name proposalType, uint64_t docID)
    {
        TRACE_FUNCTION()
        auto proposalEdges = m_dao.getGraph().getEdgesTo(docID, proposalType);

        //Check if there is another existing suspend proposal open for the given document
        for (auto& edge : proposalEdges)
        {
            Document proposal(m_dao.get_self(), edge.getFromNode());

            auto cw = proposal.getContentWrapper();
            if (cw.getOrFail(DETAILS, common::STATE)->getAs <string>() == common::STATE_PROPOSED)
            {
                return { true, edge.getFromNode() };
            }
        }

        return { false, uint64_t{} };
    }

    eosio::asset Proposal::getVoiceSupply()
    {
        asset voiceToken    = m_daoSettings->getOrFail <eosio::asset>(common::VOICE_TOKEN);
        name  voiceContract = m_dhoSettings->getOrFail <eosio::name>(GOVERNANCE_TOKEN_CONTRACT);
        hypha::voice::stats statstable(voiceContract, voiceToken.symbol.code().raw());
        auto stats_index = statstable.get_index <name("bykey")>();

        auto daoName = m_daoSettings->getOrFail <name>(DAO_NAME);

        auto stat_itr = stats_index.find(
            voice::currency_stats::build_key(
                daoName,
                voiceToken.symbol.code()
                )
            );

        EOS_CHECK(stat_itr != stats_index.end(), util::to_str("No VOICE found token: ", voiceToken, " DAO: ", daoName));

        return(stat_itr->supply);
    }

    void Proposal::_publish(const eosio::name&proposer, Document&proposal, uint64_t rootID)
    {
        TRACE_FUNCTION()
        // the DHO also links to the document as a proposal, another graph EDGE
        Edge::write(m_dao.get_self(), proposer, rootID, proposal.getID(), common::PROPOSAL);

        // Sets an empty tally
        VoteTally(m_dao, proposal, m_daoSettings);

        postProposeImpl(proposal);

        //Schedule a trx to close the proposal
        eosio::transaction trx;

        trx.actions.emplace_back(eosio::action(
                                     permission_level(m_dao.get_self(), "active"_n),
                                     m_dao.get_self(),
                                     "closedocprop"_n,
                                     std::make_tuple(proposal.getID())
                                     ));

        auto expiration = proposal.getContentWrapper().getOrFail(BALLOT, EXPIRATION_LABEL, "Proposal has no expiration")->getAs <eosio::time_point>();

        constexpr auto aditionalDelaySec = 60;

        trx.delay_sec = (expiration.sec_since_epoch() - eosio::current_time_point().sec_since_epoch()) + aditionalDelaySec;

        auto nextID = m_dhoSettings->getSettingOrDefault("next_schedule_id", int64_t(0));

        trx.send(nextID, m_dao.get_self());

        m_dhoSettings->setSetting(Content{ "next_schedule_id", nextID + 1 });
    }

    name Proposal::_newCommentSection()
    {
        name next = name(m_daoSettings->getSettingOrDefault <name>(NEXT_COMMENT_SECTION, name()).value + 1);

        m_daoSettings->setSetting(Content{ NEXT_COMMENT_SECTION, next });
        return(next);
    }
} // namespace hypha
