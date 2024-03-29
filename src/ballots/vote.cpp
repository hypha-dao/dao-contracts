#include <ballots/vote.hpp>
#include <dao.hpp>
#include <common.hpp>
#include <document_graph/edge.hpp>
#include <member.hpp>
#include <util.hpp>
#include <voice/account.hpp>
#include <logger/logger.hpp>

#include <badges/badges.hpp>

#define CONTENT_GROUP_LABEL_VOTE "vote"
#define VOTE_DATE "date"
#define VOTE_NOTE "notes"

#define TYPED_DOCUMENT_TYPE document_types::VOTE

namespace hypha
{
    Vote::Vote(hypha::dao& dao, uint64_t id)
    : TypedDocument(dao, id, TYPED_DOCUMENT_TYPE)
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
    : TypedDocument(dao, TYPED_DOCUMENT_TYPE)
    {
        TRACE_FUNCTION()
        // Could be replaced by i.e. proposal.hasVote(vote) when proposal is no longer just a "Document"
        // the goal is to have an easier API
        proposal.getContentWrapper().getOrFail(BALLOT_OPTIONS, vote, "Invalid vote");

        auto daoHash = Edge::get(dao.get_self(), proposal.getID(), common::DAO).getToNode();

        EOS_CHECK(
            Edge::exists(dao.get_self(), daoHash, proposal.getID(), common::PROPOSAL),
            "Only allowed to vote active proposals"
        );

        auto expiration = proposal.getContentWrapper().getOrFail(BALLOT, EXPIRATION_LABEL, "Proposal has no expiration")->getAs<eosio::time_point>();

        EOS_CHECK(
            eosio::time_point_sec(eosio::current_time_point()) <= expiration,
            "Voting has expired for this proposal"
        );

        Document voterDoc(dao.get_self(), dao.getMemberID(voter));

        std::vector<Edge> votes = dao.getGraph().getEdgesFrom(proposal.getID(), common::VOTE);
        for (auto vote : votes) {
            if (vote.getCreator() == voter) {

                Document voteDocument(dao.get_self(), vote.getToNode());

                // Already voted, erase edges and allow to vote again.
                Edge::get(dao.get_self(), voterDoc.getID(), voteDocument.getID(), common::VOTE).erase();
                Edge::get(dao.get_self(), proposal.getID(), voteDocument.getID(), common::VOTE).erase();
                Edge::get(dao.get_self(), voteDocument.getID(), voterDoc.getID(), common::OWNED_BY).erase();
                Edge::get(dao.get_self(), voteDocument.getID(), proposal.getID(), common::VOTE_ON).erase();

                if (!dao.getGraph().hasEdges(voteDocument.getID())) {
                    dao.getGraph().eraseDocument(voteDocument.getID(), false);
                }

                break;
            }
        }
        
        asset votePower;

        Settings* daoSettings = dao.getSettingsDocument(daoHash);

        asset voiceToken = daoSettings->getOrFail<asset>(common::VOICE_TOKEN);

        //If community voting is active for this proposal, every vote is just 1
        if (auto [_, communityVote] = proposal.getContentWrapper().get(SYSTEM, common::COMMUNITY_VOTING);
            communityVote && communityVote->getAs<int64_t>()) {
            votePower = denormalizeToken(1.0, voiceToken);
        }
        //Else fetch vote power from the voice contract
        else {
            // Todo: Need to ensure that the balance does not need a decay.
            name hvoiceContract = dao.getSettingOrFail<eosio::name>(GOVERNANCE_TOKEN_CONTRACT);
            hypha::voice::accounts acnts(hvoiceContract, voter.value);
            auto account_index = acnts.get_index<name("bykey")>();

            auto v_itr = account_index.find(
                voice::accountv2::build_key(
                    daoSettings->getOrFail<name>(DAO_NAME),
                    voiceToken.symbol.code()
                )
            );

            eosio::check(v_itr != account_index.end(), "No VOICE found");

            votePower = v_itr->balance;
        }

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

        initializeDocument(dao, contentGroups);

        // an edge from the member to the vote named vote
        // Note: This edge could already exist, as voteDocument is likely to be re-used.
        Edge::getOrNew(dao.get_self(), voter, voterDoc.getID(), getDocument().getID(), common::VOTE);

        // an edge from the proposal to the vote named vote
        Edge::write(dao.get_self(), voter, proposal.getID(), getDocument().getID(), common::VOTE);

        // an edge from the vote to the member named ownedby
        // Note: This edge could already exist, as voteDocument is likely to be re-used.
        Edge::getOrNew(dao.get_self(), voter, getDocument().getID(), voterDoc.getID(), common::OWNED_BY);

        // an edge from the vote to the proposal named voteon
        Edge::write(dao.get_self(), voter, getDocument().getID(), proposal.getID(), common::VOTE_ON);

        if (badges::hasNorthStarBadge(dao, daoHash, voterDoc.getID())) {
            if (vote == VOTE_FAIL){
                Edge(dao.get_self(), dao.get_self(), voterDoc.getID(), proposal.getID(), common::VETO);
                Edge(dao.get_self(), dao.get_self(), proposal.getID(), voterDoc.getID(), common::VETO_BY);
            }
            else if (Edge::exists(dao.get_self(), voterDoc.getID(), proposal.getID(), common::VETO)) {
                Edge::get(dao.get_self(), voterDoc.getID(), proposal.getID(), common::VETO).erase();
                Edge::get(dao.get_self(), proposal.getID(), voterDoc.getID(), common::VETO_BY).erase();
            }
        }
    }

    const std::string& Vote::getVote()
    {
        TRACE_FUNCTION()
        return getDocument().getContentWrapper().getOrFail(
            CONTENT_GROUP_LABEL_VOTE,
            VOTE_LABEL,
            "Vote does not have " CONTENT_GROUP_LABEL_VOTE " content group"
        )->getAs<std::string>();
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
