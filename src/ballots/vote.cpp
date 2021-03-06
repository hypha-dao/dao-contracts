#include <ballots/vote.hpp>
#include <dao.hpp>
#include <common.hpp>
#include <document_graph/edge.hpp>
#include <member.hpp>
#include <util.hpp>
#include <trail.hpp>

#define CONTENT_GROUP_LABEL_VOTE "vote"

namespace hypha 
{
    Vote::Vote(hypha::dao& dao, const eosio::checksum256& hash)
    : TypedDocument(dao, hash)
    {

    }
    
    Vote Vote::build(
        hypha::dao& dao, 
        const eosio::name voter, 
        std::string vote, 
        Document& proposal
    )
    {
        // Could be replaced by i.e. proposal.hasVote(vote) when proposal is no longer just a "Document"
        // the goal is to have an easier API
        proposal.getContentWrapper().getOrFail(BALLOT_OPTIONS, vote, "Invalid vote");
        
        eosio::check(
            Edge::exists(dao.get_self(), getRoot(dao.get_self()), proposal.getHash(), common::PROPOSAL),
            "Only allowed to vote active proposals"
        );

        std::vector<Edge> votes = dao.getGraph().getEdgesFrom(proposal.getHash(), common::VOTE);
        for (auto votesIt = votes.begin(); votesIt != votes.end(); ++votesIt) {
            if (votesIt->getCreator() == voter) {
                eosio::checksum256 voterHash = Member::calcHash(voter);
                Document voteDocument(dao.get_self(), votesIt->getToNode());

                // Already voted, erase edges and allow to vote again.
                Edge::get(dao.get_self(), voterHash, voteDocument.getHash(), common::VOTE).erase();
                Edge::get(dao.get_self(), proposal.getHash(), voteDocument.getHash(), common::VOTE).erase();
                Edge::get(dao.get_self(), voteDocument.getHash(), voterHash, common::OWNED_BY).erase();
                Edge::get(dao.get_self(), voteDocument.getHash(), proposal.getHash(), common::VOTE_ON).erase();
            }
        }

        // Fetch vote power
        name trailContract = dao.getSettingOrFail<eosio::name>(TELOS_DECIDE_CONTRACT);
        trailservice::trail::voters_table v_t(trailContract, voter.value);
        auto v_itr = v_t.find(common::S_HVOICE.code().raw());
        check(v_itr != v_t.end(), "No HVOICE found");
        asset votePower = v_itr->liquid;

        ContentGroups contentGroups{
            ContentGroup{
                Content(CONTENT_GROUP_LABEL, CONTENT_GROUP_LABEL_VOTE),
                Content(VOTER_LABEL, voter),
                Content(VOTE_POWER, votePower),
                Content(VOTE_LABEL, vote)
            }
        };

        eosio::checksum256 voterHash = Member::calcHash(voter);
        // Document voteDocument = Document::getOrNew(m_dao.get_self(), m_dao.get_self(), contentGroups);
        Vote voteDocument(dao, contentGroups);

        // an edge from the member to the vote named vote
        // Note: This edge could already exist, as voteDocument is likely to be re-used.
        Edge::getOrNew(dao.get_self(), voter, voterHash, voteDocument.getHash(), common::VOTE);

        // an edge from the proposal to the vote named vote
        Edge::write(dao.get_self(), voter, proposal.getHash(), voteDocument.getHash(), common::VOTE);

        // an edge from the vote to the member named ownedby
        // Note: This edge could already exist, as voteDocument is likely to be re-used.
        Edge::getOrNew(dao.get_self(), voter, voteDocument.getHash(), voterHash, common::OWNED_BY);

        // an edge from the vote to the proposal named voteon
        Edge::write(dao.get_self(), voter, voteDocument.getHash(), proposal.getHash(), common::VOTE_ON);

        return voteDocument;
    }

    Vote::Vote(dao& dao, ContentGroups &content)
    : TypedDocument(dao, content)
    {

    }

    const std::string& Vote::getVote()
    {
        return getContentWrapper().getOrFail(
            CONTENT_GROUP_LABEL_VOTE, 
            VOTE_LABEL, 
            "Vote does not have " CONTENT_GROUP_LABEL_VOTE " content group"
        )->template getAs<std::string>();
    }

    const eosio::asset& Vote::getPower()
    {
        return getContentWrapper().getOrFail(
            CONTENT_GROUP_LABEL_VOTE, 
            VOTE_POWER, 
            "Vote does not have " CONTENT_GROUP_LABEL_VOTE " content group"
        )->template getAs<eosio::asset>();
    }

    const eosio::name& Vote::getVoter()
    {
        return getContentWrapper().getOrFail(
            CONTENT_GROUP_LABEL_VOTE, 
            VOTER_LABEL, 
            "Vote does not have " CONTENT_GROUP_LABEL_VOTE " content group"
        )->template getAs<eosio::name>();
    }

}