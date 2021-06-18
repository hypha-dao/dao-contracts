#include <ballots/vote.hpp>
#include <dao.hpp>
#include <common.hpp>
#include <document_graph/edge.hpp>
#include <member.hpp>
#include <util.hpp>
#include <hypha_voice.hpp>
#include <logger/logger.hpp>

#define CONTENT_GROUP_LABEL_VOTE "vote"
#define VOTE_DATE "date"
#define VOTE_NOTE "notes"

namespace hypha 
{
    Vote::Vote(hypha::dao& dao, const eosio::checksum256& hash)
    : TypedDocument(dao, hash)
    {
        TRACE_FUNCTION()
    }
    
    Vote::Vote(
        hypha::dao& dao, 
        const eosio::name voter, 
        std::string vote, 
        Document& proposal,
        std::optional<std::string> notes
    )
    : TypedDocument(dao)
    {
        TRACE_FUNCTION()
        // Could be replaced by i.e. proposal.hasVote(vote) when proposal is no longer just a "Document"
        // the goal is to have an easier API
        proposal.getContentWrapper().getOrFail(BALLOT_OPTIONS, vote, "Invalid vote");
        
        EOS_CHECK(
            Edge::exists(dao.get_self(), getRoot(dao.get_self()), proposal.getHash(), common::PROPOSAL),
            "Only allowed to vote active proposals"
        );

        auto expiration = proposal.getContentWrapper().getOrFail(BALLOT, EXPIRATION_LABEL, "Proposal has no expiration")->getAs<eosio::time_point>();

        EOS_CHECK(
            eosio::time_point_sec(eosio::current_time_point()) <= expiration,
            "Voting has expired for this proposal"
        );

        std::vector<Edge> votes = dao.getGraph().getEdgesFrom(proposal.getHash(), common::VOTE);
        for (auto vote : votes) {
            if (vote.getCreator() == voter) {
                eosio::checksum256 voterHash = Member::calcHash(voter);
                Document voteDocument(dao.get_self(), vote.getToNode());

                // Already voted, erase edges and allow to vote again.
                Edge::get(dao.get_self(), voterHash, voteDocument.getHash(), common::VOTE).erase();
                Edge::get(dao.get_self(), proposal.getHash(), voteDocument.getHash(), common::VOTE).erase();
                Edge::get(dao.get_self(), voteDocument.getHash(), voterHash, common::OWNED_BY).erase();
                Edge::get(dao.get_self(), voteDocument.getHash(), proposal.getHash(), common::VOTE_ON).erase();

                if (!dao.getGraph().hasEdges(voteDocument.getHash())) {
                    dao.getGraph().eraseDocument(voteDocument.getHash(), false);
                }

                break;
            }
        }

        // Fetch vote power
        // Todo: Need to ensure that the balance does not need a decay.
        name hvoiceContract = dao.getSettingOrFail<eosio::name>(GOVERNANCE_TOKEN_CONTRACT);
        hypha::voice::accounts v_t(hvoiceContract, voter.value);

        auto v_itr = v_t.find(common::S_VOICE.code().raw());
        eosio::check(v_itr != v_t.end(), "No HVOICE found");

        asset votePower = v_itr->balance;

        ContentGroups contentGroups{
            ContentGroup{
                Content(CONTENT_GROUP_LABEL, CONTENT_GROUP_LABEL_VOTE),
                Content(VOTER_LABEL, voter),
                Content(VOTE_POWER, votePower),
                Content(VOTE_LABEL, vote),
                Content(VOTE_DATE, eosio::time_point_sec(eosio::current_time_point())),
                Content(VOTE_NOTE, notes.value_or(""))
            }
        };

        eosio::checksum256 voterHash = Member::calcHash(voter);
        initializeDocument(dao, contentGroups, false);

        // an edge from the member to the vote named vote
        // Note: This edge could already exist, as voteDocument is likely to be re-used.
        Edge::getOrNew(dao.get_self(), voter, voterHash, getDocument().getHash(), common::VOTE);

        // an edge from the proposal to the vote named vote
        Edge::write(dao.get_self(), voter, proposal.getHash(), getDocument().getHash(), common::VOTE);

        // an edge from the vote to the member named ownedby
        // Note: This edge could already exist, as voteDocument is likely to be re-used.
        Edge::getOrNew(dao.get_self(), voter, getDocument().getHash(), voterHash, common::OWNED_BY);

        // an edge from the vote to the proposal named voteon
        Edge::write(dao.get_self(), voter, getDocument().getHash(), proposal.getHash(), common::VOTE_ON);
    }

    const std::string& Vote::getVote()
    {
        TRACE_FUNCTION()
        return getDocument().getContentWrapper().getOrFail(
            CONTENT_GROUP_LABEL_VOTE, 
            VOTE_LABEL, 
            "Vote does not have " CONTENT_GROUP_LABEL_VOTE " content group"
        )->template getAs<std::string>();
    }

    const eosio::asset& Vote::getPower()
    {
        TRACE_FUNCTION()
        return getDocument().getContentWrapper().getOrFail(
            CONTENT_GROUP_LABEL_VOTE, 
            VOTE_POWER, 
            "Vote does not have " CONTENT_GROUP_LABEL_VOTE " content group"
        )->template getAs<eosio::asset>();
    }

    const eosio::name& Vote::getVoter()
    {
        TRACE_FUNCTION()
        return getDocument().getContentWrapper().getOrFail(
            CONTENT_GROUP_LABEL_VOTE, 
            VOTER_LABEL, 
            "Vote does not have " CONTENT_GROUP_LABEL_VOTE " content group"
        )->template getAs<eosio::name>();
    }

    const std::string Vote::buildNodeLabel(ContentGroups &content)
    {
        TRACE_FUNCTION()
        std::string vote = ContentWrapper(content).getOrFail(
            CONTENT_GROUP_LABEL_VOTE, 
            VOTE_LABEL, 
            "Vote does not have " CONTENT_GROUP_LABEL_VOTE " content group"
        )->template getAs<std::string>(); 
        return "Vote: " + vote;
    }

}