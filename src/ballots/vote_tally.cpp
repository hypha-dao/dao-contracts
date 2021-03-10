#include <ballots/vote_tally.hpp>
#include <dao.hpp>
#include <common.hpp>
#include <document_graph/edge.hpp>
#include <ballots/vote.hpp>

namespace hypha
{

    VoteTally::VoteTally(dao& dao, const eosio::checksum256& hash)
    : TypedDocument(dao, hash)
    {

    }

    VoteTally::VoteTally(
        dao& dao,
        Document& proposal
    )
    : TypedDocument(dao)
    {
        auto [exists, oldTally] = Edge::getIfExists(dao.get_self(), proposal.getHash(), common::VOTE_TALLY);
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

        std::vector<Edge> edges = dao.getGraph().getEdgesFrom(proposal.getHash(), common::VOTE);
        for (auto itr = edges.begin(); itr != edges.end(); ++itr) {
            eosio::checksum256 voteHash = itr->getToNode();
            Vote voteDocument(dao, voteHash);

            optionsTally[voteDocument.getVote()] += voteDocument.getPower();
        }

        ContentGroups tallyContentGroups;
        for (auto it = optionsTallyOrdered.begin(); it != optionsTallyOrdered.end(); ++it) 
        {
            tallyContentGroups.push_back(ContentGroup{
                Content(CONTENT_GROUP_LABEL, *it),
                Content(VOTE_POWER, optionsTally[*it])
            });
        }

        initializeDocument(dao, tallyContentGroups, false);

        Edge::write(dao.get_self(), dao.get_self(), proposal.getHash(), getDocument().getHash(), common::VOTE_TALLY);
    }

    const std::string VoteTally::buildNodeLabel(ContentGroups &content)
    {
        return "VoteTally";
    }
}