#include <eosio/action.hpp>

#include <proposals/proposal.hpp>
#include <document_graph/content_wrapper.hpp>
#include <document_graph/content.hpp>
#include <document_graph/document.hpp>
#include <member.hpp>
#include <common.hpp>
#include <document_graph/edge.hpp>
#include <dao.hpp>
#include <trail.hpp>
#include <util.hpp>

using namespace eosio;

namespace hypha
{
    Proposal::Proposal(dao &contract) : m_dao{contract} {}
    Proposal::~Proposal() {}

    Document Proposal::propose(const eosio::name &proposer, ContentGroups &contentGroups)
    {
        eosio::check(Member::isMember(m_dao.get_self(), proposer), "only members can make proposals: " + proposer.to_string());
        ContentWrapper proposalContent(contentGroups);
        proposeImpl(proposer, proposalContent);

        std::string decide_title = proposalContent.getOrFail(DETAILS, TITLE)->getAs<std::string>();

        contentGroups.push_back(makeSystemGroup(proposer,
                                                getProposalType(),
                                                decide_title));

        Document proposalNode(m_dao.get_self(), proposer, contentGroups);
        eosio::checksum256 ballotHash = registerBallot(proposer, 
                                            decide_title, 
                                            proposalContent.getOrFail(DETAILS, DESCRIPTION)->getAs<std::string>(), 
                                            getBallotContent(proposalContent));

        // creates the document, or the graph NODE
        eosio::checksum256 memberHash = Member::calcHash(proposer);
        eosio::checksum256 root = getRoot(m_dao.get_self());

        // the proposer OWNS the proposal; this creates the graph EDGE
        Edge::write(m_dao.get_self(), proposer, memberHash, proposalNode.getHash(), common::OWNS);

        // the proposal was PROPOSED_BY proposer; this creates the graph EDGE
        Edge::write(m_dao.get_self(), proposer, proposalNode.getHash(), memberHash, common::OWNED_BY);

        // the DHO also links to the document as a proposal, another graph EDGE
        Edge::write(m_dao.get_self(), proposer, root, proposalNode.getHash(), common::PROPOSAL);
        
        // the proposal HAS_BALLOT ballot; this creates the graph EDGE
        Edge::write(m_dao.get_self(), proposer, proposalNode.getHash(), ballotHash, common::HAS_BALLOT);

        postProposeImpl(proposalNode);

        return proposalNode;
    }

    void Proposal::postProposeImpl(Document &proposal) {}

    void Proposal::close(Document &proposal)
    {
        eosio::checksum256 root = getRoot(m_dao.get_self());

        Edge edge = Edge::get(m_dao.get_self(), root, proposal.getHash(), common::PROPOSAL);
        edge.erase();

        name ballot_id = proposal.getContentWrapper().getOrFail(SYSTEM, BALLOT_ID)->getAs<eosio::name>();
        if (didPass(ballot_id))
        {
            // INVOKE child class close logic
            passImpl(proposal);

            // if proposal passes, create an edge for PASSED_PROPS
            Edge::write(m_dao.get_self(), m_dao.get_self(), root, proposal.getHash(), common::PASSED_PROPS);
        }
        else
        {
            // create edge for FAILED_PROPS
            Edge::write(m_dao.get_self(), m_dao.get_self(), root, proposal.getHash(), common::FAILED_PROPS);
        }

        eosio::action(
            eosio::permission_level{m_dao.get_self(), name("active")},
            m_dao.getSettingOrFail<eosio::name>(TELOS_DECIDE_CONTRACT), name("closevoting"),
            std::make_tuple(ballot_id, true))
            .send();
    }

    ContentGroup Proposal::makeSystemGroup(const name &proposer,
                                           const name &proposal_type,
                                           const string &decide_title)
    {
        return ContentGroup{
            Content(CONTENT_GROUP_LABEL, SYSTEM),
            Content(CLIENT_VERSION, m_dao.getSettingOrDefault<std::string>(CLIENT_VERSION, DEFAULT_VERSION)),
            Content(CONTRACT_VERSION, m_dao.getSettingOrDefault<std::string>(CONTRACT_VERSION, DEFAULT_VERSION)),
            Content(NODE_LABEL, decide_title),
            Content(TYPE, proposal_type)};
    }

    bool Proposal::didPass(const name &ballot_id)
    {
        name trailContract = m_dao.getSettingOrFail<eosio::name>(TELOS_DECIDE_CONTRACT);
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

    eosio::checksum256 Proposal::registerBallot(const name &proposer,
                                  const string &title, const string &description, const string &content)
    {
        check(has_auth(proposer) || has_auth(m_dao.get_self()), "Authentication failed. Must have authority from proposer: " +
                                                                    proposer.to_string() + "@active or " + m_dao.get_self().to_string() + "@active.");

        auto expiration = time_point_sec(current_time_point()) + m_dao.getSettingOrFail<int64_t>(VOTING_DURATION_SEC);

        ContentGroups contentGroups{
            ContentGroup{
                Content(CONTENT_GROUP_LABEL, "settings"),
                Content("expiration", expiration)
            },
            ContentGroup{
                Content(CONTENT_GROUP_LABEL, "options"),
                Content("name", name("pass")),
                Content("fail", name("fail")),
                Content("abstain", name("abstain")),
            }
        };

        Document ballotNode(m_dao.get_self(), proposer, contentGroups);
        return ballotNode.getHash();
    }

    eosio::checksum256 Proposal::registerBallot(const name &proposer,
                                  const map<string, string> &strings)
    {
        return registerBallot(proposer,
                              strings.at(TITLE),
                              strings.at(DESCRIPTION),
                              strings.at(CONTENT));
    }

} // namespace hypha
