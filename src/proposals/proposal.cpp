#include <proposals/proposal.hpp>
#include <document_graph/content_group.hpp>
#include <document_graph/content.hpp>
#include <document_graph/document.hpp>
#include <member.hpp>
#include <common.hpp>
#include <document_graph/edge.hpp>
#include <eosio/action.hpp>
#include <dao.hpp>
#include <trail.hpp>

using namespace eosio;

namespace hypha
{
    Proposal::Proposal(dao &contract) : m_dao{contract} {}
    Proposal::~Proposal() {}

    Document Proposal::propose(const eosio::name &proposer, ContentGroups &contentGroups)
    {
        verify_membership(proposer);
        contentGroups = propose_impl(proposer, contentGroups);

        contentGroups.push_back(create_system_group(proposer,
                                                    GetProposalType(),
                                                    ContentWrapper::getContent(contentGroups, common::DETAILS, common::TITLE).getAs<std::string>(),
                                                    ContentWrapper::getContent(contentGroups, common::DETAILS, common::DESCRIPTION).getAs<std::string>(),
                                                    GetBallotContent(contentGroups)));

        // creates the document, or the graph NODE
        auto memberHash = Member::getHash(proposer);
        ContentGroups rootCgs = Document::rollup(Content(common::ROOT_NODE, m_dao.get_self()));
        auto rootNode = Document::hashContents(rootCgs);

        Document proposalNode(m_dao.get_self(), proposer, contentGroups);
        proposalNode.emplace();
        // proposalNode.emplace();

        // the proposer OWNS the proposal; this creates the graph EDGE
        Edge memberProposalEdge(m_dao.get_self(), proposer, memberHash, proposalNode.getHash(), common::OWNS);
        memberProposalEdge.emplace();

        // the proposal was PROPOSED_BY proposer; this creates the graph EDGE
        Edge proposalMemberEdge(m_dao.get_self(), proposer, proposalNode.getHash(), memberHash, common::OWNED_BY);
        proposalMemberEdge.emplace();

        // the DHO also links to the document as a proposal, another graph EDGE
        Edge rootProposalEdge(m_dao.get_self(), proposer, rootNode, proposalNode.getHash(), common::PROPOSAL);
        rootProposalEdge.emplace();

        return proposalNode;
    }

    void Proposal::close(Document proposal)
    {
        ContentGroups rootCgs = Document::rollup(Content(common::ROOT_NODE, m_dao.get_self()));
        auto rootNode = Document::hashContents(rootCgs);
        Edge edge (m_dao.get_self(), m_dao.get_self(), rootNode, proposal.getHash(), common::PROPOSAL);
        edge.erase();

        name ballot_id = ContentWrapper::getContent(proposal.getContentGroups(), common::SYSTEM, common::BALLOT_ID).getAs<eosio::name>();
        if (did_pass(ballot_id))
        {
            // INVOKE child class close logic
            pass_impl(proposal);

            // if proposal passes, create an edge for PASSED_PROPS
            Edge rootPassedProposal(m_dao.get_self(), m_dao.get_self(), rootNode, proposal.getHash(), common::PASSED_PROPS);
            rootPassedProposal.emplace();
        }
        else
        {
            // create edge for FAILED_PROPS
            Edge rootFailedProposal(m_dao.get_self(), m_dao.get_self(), rootNode, proposal.getHash(), common::FAILED_PROPS);
            rootFailedProposal.emplace();
        }

        eosio::action(
            eosio::permission_level{m_dao.get_self(), name("active")},
            m_dao.getSettingOrFail<eosio::name>(common::TELOS_DECIDE_CONTRACT), name("closevoting"),
            std::make_tuple(ballot_id, true))
            .send();
    }

    ContentGroup Proposal::create_system_group(const name &proposer,
                                               const name &proposal_type,
                                               const string &decide_title,
                                               const string &decide_desc,
                                               const string &decide_content)

    {
        // create the system content_group and populate with system details
        name ballot_id = register_ballot(proposer, decide_title, decide_desc, decide_content);

        ContentGroup system_cg = ContentGroup{};
        system_cg.push_back(Content(CONTENT_GROUP_LABEL, common::SYSTEM));
        system_cg.push_back(Content(common::CLIENT_VERSION, ""));   // TODO: call get_setting
        system_cg.push_back(Content(common::CONTRACT_VERSION, "")); // TODO call get_setting
        system_cg.push_back(Content(common::BALLOT_ID, ballot_id));
        // system_cg.push_back(Content(common::OWNED_BY, proposer));
        system_cg.push_back(Content(common::TYPE, proposal_type));
        return system_cg;
    }

    void Proposal::verify_membership(const name &member)
    {
        // create hash to represent this member account
        auto memberHash = Member::getHash(member);
        ContentGroups rootCgs = Document::rollup(Content(common::ROOT_NODE, m_dao.get_self()));
        auto rootNode = Document::hashContents(rootCgs);

        Edge::get(m_dao.get_self(), rootNode, memberHash, common::MEMBER);

        //check(itr != e_t.end(), "account: " + member.to_string() + " is not a member of " + document_graph::readable_hash(root_hash));
    }

    bool Proposal::did_pass(const name &ballot_id)
    {
        name trailContract = m_dao.getSettingOrFail<eosio::name>(common::TELOS_DECIDE_CONTRACT);
        trailservice::trail::ballots_table b_t(trailContract, trailContract.value);
        auto b_itr = b_t.find(ballot_id.value);
        check(b_itr != b_t.end(), "ballot_id: " + ballot_id.to_string() + " not found.");

        trailservice::trail::treasuries_table t_t(trailContract, trailContract.value);
        auto t_itr = t_t.find(common::S_HVOICE.code().raw());
        check(t_itr != t_t.end(), "Treasury: " + common::S_HVOICE.code().to_string() + " not found.");

        asset quorum_threshold = adjustAsset(t_itr->supply, 0.20000000);
        map<name, asset> votes = b_itr->options;
        asset votes_pass = votes.at(name("pass"));
        asset votes_fail = votes.at(name("fail"));
        asset votes_abstain = votes.at(name("abstain"));

        bool passed = false;
        if (b_itr->total_raw_weight >= quorum_threshold &&      // must meet quorum
            adjustAsset(votes_pass, 0.2500000000) > votes_fail) // must have 80% of the vote power
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    name Proposal::register_ballot(const name &proposer,
                                   const string &title, const string &description, const string &content)
    {
        check(has_auth(proposer) || has_auth(m_dao.get_self()), "Authentication failed. Must have authority from proposer: " +
                                                              proposer.to_string() + "@active or " + m_dao.get_self().to_string() + "@active.");

        // increment the ballot_id
        name ballotId = name(m_dao.getSettingOrFail<eosio::name>(common::LAST_BALLOT_ID).value + 1);
        m_dao.setsetting(common::LAST_BALLOT_ID, ballotId);

        name trailContract = m_dao.getSettingOrFail<eosio::name>(common::TELOS_DECIDE_CONTRACT);

        trailservice::trail::ballots_table b_t(trailContract, trailContract.value);
        auto b_itr = b_t.find(ballotId.value);
        check(b_itr == b_t.end(), "ballot_id: " + ballotId.to_string() + " has already been used.");

        vector<name> options;
        options.push_back(name("pass"));
        options.push_back(name("fail"));
        options.push_back(name("abstain"));

        action(
            permission_level{m_dao.get_self(), name("active")},
            trailContract, name("newballot"),
            std::make_tuple(
                ballotId,
                name("poll"),
                m_dao.get_self(),
                common::S_HVOICE,
                name("1token1vote"),
                options))
            .send();

        //	  // default is to vote all tokens, not just staked tokens
        //    action (
        //       permission_level{m_dao._document_graph.contract, "active"_n},
        //       c.telos_decide_contract, "togglebal"_n,
        //       std::make_tuple(new_ballot_id, "votestake"_n))
        //    .send();

        action(
            permission_level{m_dao.get_self(), name("active")},
            trailContract, name("editdetails"),
            std::make_tuple(
                ballotId,
                title,
                description.substr(0, std::min(description.length(), size_t(25))),
                content))
            .send();

        auto expiration = time_point_sec(current_time_point()) + m_dao.getSettingOrFail<int64_t>(common::VOTING_DURATION_SEC);

        action(
            permission_level{m_dao.get_self(), name("active")},
            trailContract, name("openvoting"),
            std::make_tuple(ballotId, expiration))
            .send();

        return ballotId;
    }

    name Proposal::register_ballot(const name &proposer,
                                   const map<string, string> &strings)
    {
        return register_ballot(proposer,
                               strings.at(common::TITLE),
                               strings.at(common::DESCRIPTION),
                               strings.at(common::CONTENT));
    }

    asset Proposal::adjustAsset(const asset &originalAsset, const float &adjustment)
    {
        return asset{static_cast<int64_t>(originalAsset.amount * adjustment), originalAsset.symbol};
    }

} // namespace hypha
