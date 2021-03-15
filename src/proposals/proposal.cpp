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

        std::string proposal_title = proposalContent.getOrFail(DETAILS, TITLE)->getAs<std::string>();

        contentGroups.push_back(makeSystemGroup(proposer,
                                                getProposalType(),
                                                proposal_title));
        
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
        updateVoteTally(proposalNode, proposer);

        postProposeImpl(proposalNode);

        return proposalNode;
    }

    void Proposal::postProposeImpl(Document &proposal) {}

    void Proposal::vote(const eosio::name &voter, const std::string vote, Document& proposal)
    {
        proposal.getContentWrapper().getOrFail(BALLOT_OPTIONS, vote, "Invalid vote");

        eosio::check(
            Edge::exists(m_dao.get_self(), getRoot(m_dao.get_self()), proposal.getHash(), common::PROPOSAL),
            "Only allowed to vote active proposals"
        );

        std::vector<Edge> votes = m_dao.getGraph().getEdgesFrom(proposal.getHash(), common::VOTE);
        for (auto votesIt = votes.begin(); votesIt != votes.end(); ++votesIt) {
            if (votesIt->getCreator() == voter) {
                eosio::checksum256 voterHash = Member::calcHash(voter);
                Document voteDocument(m_dao.get_self(), votesIt->getToNode());

                // Already voted, erase edges and allow to vote again.
                Edge::get(m_dao.get_self(), voterHash, voteDocument.getHash(), common::VOTE).erase();
                Edge::get(m_dao.get_self(), proposal.getHash(), voteDocument.getHash(), common::VOTE).erase();
                Edge::get(m_dao.get_self(), voteDocument.getHash(), voterHash, common::OWNED_BY).erase();
                Edge::get(m_dao.get_self(), voteDocument.getHash(), proposal.getHash(), common::VOTE_ON).erase();
            }
        }

        // Fetch vote power
        name trailContract = m_dao.getSettingOrFail<eosio::name>(TELOS_DECIDE_CONTRACT);
        trailservice::trail::voters_table v_t(trailContract, voter.value);
        auto v_itr = v_t.find(common::S_HVOICE.code().raw());
        check(v_itr != v_t.end(), "No HVOICE found");
        asset votePower = v_itr->liquid;

        ContentGroups contentGroups{
            ContentGroup{
                Content(VOTER_LABEL, voter),
                Content(VOTE_POWER, votePower),
                Content(VOTE_LABEL, vote)
            }
        };

        eosio::checksum256 voterHash = Member::calcHash(voter);
        Document voteDocument = Document::getOrNew(m_dao.get_self(), m_dao.get_self(), contentGroups);

        // an edge from the member to the vote named vote
        // Note: This edge could already exist, as voteDocument is likely to be re-used.
        Edge::getOrNew(m_dao.get_self(), voter, voterHash, voteDocument.getHash(), common::VOTE);

        // an edge from the proposal to the vote named vote
        Edge::write(m_dao.get_self(), voter, proposal.getHash(), voteDocument.getHash(), common::VOTE);

        // an edge from the vote to the member named ownedby
        // Note: This edge could already exist, as voteDocument is likely to be re-used.
        Edge::getOrNew(m_dao.get_self(), voter, voteDocument.getHash(), voterHash, common::OWNED_BY);

        // an edge from the vote to the proposal named voteon
        Edge::write(m_dao.get_self(), voter, voteDocument.getHash(), proposal.getHash(), common::VOTE_ON);

        updateVoteTally(proposal, voter);
    }

    void Proposal::close(Document &proposal)
    {
        eosio::checksum256 root = getRoot(m_dao.get_self());

        Edge edge = Edge::get(m_dao.get_self(), root, proposal.getHash(), common::PROPOSAL);
        edge.erase();

        auto [ isNew, voteTallyEdge ] = Edge::getIfExists(m_dao.get_self(), proposal.getHash(), common::VOTE_TALLY);
        bool proposalDidPass;

        if (isNew) {
            auto ballotHash = voteTallyEdge.getToNode();
            proposalDidPass = didPass(ballotHash);
        } else {
            // Backwards compatiblity to old ballots
            name ballot_id = proposal.getContentWrapper().getOrFail(SYSTEM, "ballot_id")->getAs<eosio::name>();
            proposalDidPass = oldDidPass(ballot_id);
        }

        
        if (proposalDidPass)
        {
            auto details = proposal.getContentWrapper().getGroupOrFail(DETAILS);
            ContentWrapper::insertOrReplace(*details, Content{
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

        if (!isNew) {
            name ballot_id = proposal.getContentWrapper().getOrFail(SYSTEM, "ballot_id")->getAs<eosio::name>();
            eosio::action(
            eosio::permission_level{m_dao.get_self(), name("active")},
            m_dao.getSettingOrFail<eosio::name>(TELOS_DECIDE_CONTRACT), name("closevoting"),
            std::make_tuple(ballot_id, true))
            .send();
        }
    }

    ContentGroup Proposal::makeSystemGroup(const name &proposer,
                                           const name &proposal_type,
                                           const string &proposal_title)
    {
        return ContentGroup{
            Content(CONTENT_GROUP_LABEL, SYSTEM),
            Content(CLIENT_VERSION, m_dao.getSettingOrDefault<std::string>(CLIENT_VERSION, DEFAULT_VERSION)),
            Content(CONTRACT_VERSION, m_dao.getSettingOrDefault<std::string>(CONTRACT_VERSION, DEFAULT_VERSION)),
            Content(NODE_LABEL, proposal_title),
            Content(TYPE, proposal_type)};
    }

    ContentGroup Proposal::makeBallotGroup()
    {

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

    void Proposal::updateVoteTally(Document& proposal, const eosio::name creator)
    {
        auto [exists, oldTally] = Edge::getIfExists(m_dao.get_self(), proposal.getHash(), common::VOTE_TALLY);
        if (exists) {
            oldTally.erase();
        }

        ContentGroup* contentOptions = proposal.getContentWrapper().getGroupOrFail(BALLOT_OPTIONS);

        std::map<std::string, eosio::asset> optionsTally;
        std::vector<std::string> optionsTallyOrdered;
        for (auto it = contentOptions->begin(); it != contentOptions->end(); ++it) 
        {
            if (it->label != CONTENT_GROUP_LABEL) {
                optionsTally[it->label] = asset(0, common::S_HVOICE);
                optionsTallyOrdered.push_back(it->label);
            }
        }

        
        std::vector<Edge> edges = m_dao.getGraph().getEdgesFrom(proposal.getHash(), common::VOTE);
        for (auto itr = edges.begin(); itr != edges.end(); ++itr) {
            eosio::checksum256 voteHash = itr->getToNode();
            Document voteDocument(m_dao.get_self(), voteHash);
            ContentGroup group = voteDocument.getContentGroups().front();
            std::string vote;
            eosio::asset power;
            for (ContentGroup::const_iterator contentIt = group.begin(); contentIt != group.end(); ++contentIt)  {
                if (contentIt->label == VOTE_POWER) {
                    power = contentIt->getAs<eosio::asset>();
                } else if (contentIt->label == VOTE_LABEL) {
                    vote = contentIt->getAs<std::string>();
                }
            }

            optionsTally[vote] += power;
        }
        

        ContentGroups tallyContentGroups;
        for (auto it = optionsTallyOrdered.begin(); it != optionsTallyOrdered.end(); ++it) 
        {
            tallyContentGroups.push_back(ContentGroup{
                Content(CONTENT_GROUP_LABEL, *it),
                Content(VOTE_POWER, optionsTally[*it])
            });
        }

        // Who is the creator? The last voter? the contract?
        Document document = Document::getOrNew(m_dao.get_self(), m_dao.get_self(), tallyContentGroups);
        Edge::write(m_dao.get_self(), m_dao.get_self(), proposal.getHash(), document.getHash(), common::VOTE_TALLY);
    }

    bool Proposal::didPass(const eosio::checksum256 &tallyHash)
    {
        name trailContract = m_dao.getSettingOrFail<eosio::name>(TELOS_DECIDE_CONTRACT);

        trailservice::trail::treasuries_table t_t(trailContract, trailContract.value);
        auto t_itr = t_t.find(common::S_HVOICE.code().raw());
        check(t_itr != t_t.end(), "Treasury: " + common::S_HVOICE.code().to_string() + " not found.");

        asset quorum_threshold = adjustAsset(t_itr->supply, 0.20000000);

        Document tally(m_dao.get_self(), tallyHash);

        // Currently get pass/fail
        asset votes_pass = tally.getContentWrapper().getOrFail(common::BALLOT_DEFAULT_OPTION_PASS.to_string(), VOTE_POWER)->getAs<eosio::asset>();
        asset votes_abstain = tally.getContentWrapper().getOrFail(common::BALLOT_DEFAULT_OPTION_ABSTAIN.to_string(), VOTE_POWER)->getAs<eosio::asset>();
        asset votes_fail = tally.getContentWrapper().getOrFail(common::BALLOT_DEFAULT_OPTION_FAIL.to_string(), VOTE_POWER)->getAs<eosio::asset>();

        asset total = votes_pass + votes_abstain + votes_fail;
        bool passed = false;
        if (total >= quorum_threshold &&      // must meet quorum
            adjustAsset(votes_pass, 0.2500000000) > votes_fail) // must have 80% of the vote power
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    // Copy of the old didPass method. Should be removed later and code above cleaned when old ballots 
    // are no longer supported (because all should finish eventually)
    bool Proposal::oldDidPass(const eosio::name &ballotId)
    { 
        name trailContract = m_dao.getSettingOrFail<eosio::name>(TELOS_DECIDE_CONTRACT);
        trailservice::trail::ballots_table b_t(trailContract, trailContract.value);
        auto b_itr = b_t.find(ballotId.value);
        check(b_itr != b_t.end(), "ballot_id: " + ballotId.to_string() + " not found.");

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

} // namespace hypha
