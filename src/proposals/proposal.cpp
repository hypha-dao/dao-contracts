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
#include <hypha_voice.hpp>
#include <util.hpp>
#include <logger/logger.hpp>

using namespace eosio;

namespace hypha
{
    Proposal::Proposal(dao &contract) : m_dao{contract} {}
    Proposal::~Proposal() {}

    Document Proposal::propose(const eosio::name &proposer, ContentGroups &contentGroups)
    {
        TRACE_FUNCTION()
        EOS_CHECK(Member::isMember(m_dao.get_self(), proposer), "only members can make proposals: " + proposer.to_string());
        ContentWrapper proposalContent(contentGroups);
        proposeImpl(proposer, proposalContent);

        contentGroups.push_back(makeSystemGroup(proposer,
                                                getProposalType(),
                                                getTitle(proposalContent),
                                                getDescription(proposalContent)));
        
        contentGroups.push_back(makeBallotGroup());
        contentGroups.push_back(makeBallotOptionsGroup());

        Document proposalNode(m_dao.get_self(), proposer, contentGroups);

        // creates the document, or the graph NODE
        eosio::checksum256 memberHash = Member::calcHash(proposer);
        eosio::checksum256 root = getRoot(m_dao.get_self());

        // the proposer OWNS the proposal; this creates the graph EDGE
        Edge::write(m_dao.get_self(), proposer, memberHash, proposalNode.getHash(), common::OWNS);

        // the proposal was PROPOSED_BY proposer; this creates the graph EDGE
        Edge::write(m_dao.get_self(), proposer, proposalNode.getHash(), memberHash, common::OWNED_BY);

        // the DHO also links to the document as a proposal, another graph EDGE
        Edge::write(m_dao.get_self(), proposer, root, proposalNode.getHash(), common::PROPOSAL);

        // Sets an empty tally
        VoteTally(m_dao, proposalNode);

        postProposeImpl(proposalNode);

        return proposalNode;
    }

    void Proposal::postProposeImpl(Document &proposal) {}

    void Proposal::vote(const eosio::name &voter, const std::string vote, Document& proposal, std::optional<std::string> notes)
    {
        TRACE_FUNCTION()
        Vote(m_dao, voter, vote, proposal, notes);
        
        VoteTally(m_dao, proposal);
    }

    void Proposal::close(Document &proposal)
    {
        TRACE_FUNCTION()
        auto voteTallyEdge = Edge::get(m_dao.get_self(), proposal.getHash(), common::VOTE_TALLY);

        auto expiration = proposal.getContentWrapper().getOrFail(BALLOT, EXPIRATION_LABEL, "Proposal has no expiration")->getAs<eosio::time_point>();
        EOS_CHECK(
            eosio::time_point_sec(eosio::current_time_point()) > expiration,
            "Voting is still active for this proposal"
        );

        eosio::checksum256 root = getRoot(m_dao.get_self());

        Edge edge = Edge::get(m_dao.get_self(), root, proposal.getHash(), common::PROPOSAL);
        edge.erase();

        bool proposalDidPass;

        auto ballotHash = voteTallyEdge.getToNode();
        proposalDidPass = didPass(ballotHash);

        if (proposalDidPass)
        {

            auto system = proposal.getContentWrapper().getGroupOrFail(SYSTEM);
            ContentWrapper::insertOrReplace(*system, Content{
              common::APPROVED_DATE,
              eosio::current_time_point()
            });
            // INVOKE child class close logic
            passImpl(proposal);

            proposal = m_dao.getGraph().updateDocument(proposal.getCreator(), 
                                                       proposal.getHash(),
                                                       std::move(proposal.getContentGroups()));
            // if proposal passes, create an edge for PASSED_PROPS
            Edge::write(m_dao.get_self(), m_dao.get_self(), root, proposal.getHash(), common::PASSED_PROPS);
        }
        else
        {
            // create edge for FAILED_PROPS
            Edge::write(m_dao.get_self(), m_dao.get_self(), root, proposal.getHash(), common::FAILED_PROPS);
        }
    }

    ContentGroup Proposal::makeSystemGroup(const name &proposer,
                                           const name &proposal_type,
                                           const string &proposal_title,
                                           const string &proposal_description)
    {
        return ContentGroup{
            Content(CONTENT_GROUP_LABEL, SYSTEM),
            Content(CLIENT_VERSION, m_dao.getSettingOrDefault<std::string>(CLIENT_VERSION, DEFAULT_VERSION)),
            Content(CONTRACT_VERSION, m_dao.getSettingOrDefault<std::string>(CONTRACT_VERSION, DEFAULT_VERSION)),
            Content(NODE_LABEL, proposal_title),
            Content(DESCRIPTION, proposal_description),
            Content(TYPE, proposal_type)};
    }

    ContentGroup Proposal::makeBallotGroup()
    {
        TRACE_FUNCTION()
        auto expiration = time_point_sec(current_time_point()) + m_dao.getSettingOrFail<int64_t>(VOTING_DURATION_SEC);
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

    bool Proposal::didPass(const eosio::checksum256 &tallyHash)
    {
        TRACE_FUNCTION()
        name hvoiceContract = m_dao.getSettingOrFail<eosio::name>(GOVERNANCE_TOKEN_CONTRACT);
        float quorumFactor = m_dao.getSettingOrFail<uint64_t>(VOTING_QUORUM_FACTOR_X100) / 100.0f;
        float alignmentFactor = m_dao.getSettingOrFail<uint64_t>(VOTING_ALIGNMENT_FACTOR_X100) / 100.0f;

        hypha::voice::stats stats_t(hvoiceContract, common::S_VOICE.code().raw());
        auto stat_itr = stats_t.find(common::S_VOICE.code().raw());
        EOS_CHECK(stat_itr != stats_t.end(), "No HVOICE found");

        asset quorum_threshold = adjustAsset(stat_itr->supply, quorumFactor);

        VoteTally tally(m_dao, tallyHash);

        // Currently get pass/fail
        // Todo: Abstract this part into VoteTally class
        asset votes_pass = tally.getDocument().getContentWrapper().getOrFail(common::BALLOT_DEFAULT_OPTION_PASS.to_string(), VOTE_POWER)->getAs<eosio::asset>();
        asset votes_abstain = tally.getDocument().getContentWrapper().getOrFail(common::BALLOT_DEFAULT_OPTION_ABSTAIN.to_string(), VOTE_POWER)->getAs<eosio::asset>();
        asset votes_fail = tally.getDocument().getContentWrapper().getOrFail(common::BALLOT_DEFAULT_OPTION_FAIL.to_string(), VOTE_POWER)->getAs<eosio::asset>();

        asset total = votes_pass + votes_abstain + votes_fail;
        // pass / ( pass + fail ) > alignmentFactor
        bool passesAlignment = votes_pass > (alignmentFactor * (votes_pass + votes_fail));
        if (total >= quorum_threshold &&      // must meet quorum
            passesAlignment)
        {
            return true;
        }
        else
        {
            return false;
        }
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
} // namespace hypha
