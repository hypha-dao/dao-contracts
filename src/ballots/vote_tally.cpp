#include <ballots/vote_tally.hpp>
#include <dao.hpp>
#include <common.hpp>
#include <document_graph/edge.hpp>
#include <ballots/vote.hpp>
#include <logger/logger.hpp>

namespace hypha
{

    VoteTally::VoteTally(dao& dao, const eosio::checksum256& hash)
    : TypedDocument(dao, hash)
    {
      TRACE_FUNCTION()
    }

    VoteTally::VoteTally(
        dao& dao,
        Document& proposal
    )
    : TypedDocument(dao)
    {
        TRACE_FUNCTION()
        auto [exists, oldTally] = Edge::getIfExists(dao.get_self(), proposal.getHash(), common::VOTE_TALLY);
        if (exists) {
            auto oldTallyNode = oldTally.to_node;
            oldTally.erase();

            if (!dao.getGraph().hasEdges(oldTallyNode)) {
                dao.getGraph().eraseDocument(oldTallyNode, false);
            }
            
        }

        ContentGroup* contentOptions = proposal.getContentWrapper().getGroupOrFail(BALLOT_OPTIONS);

        std::map<std::string, eosio::asset> optionsTally;
        std::vector<std::string> optionsTallyOrdered;
        for (auto option : *contentOptions) 
        {
            if (option.label != CONTENT_GROUP_LABEL) {
                optionsTally[option.label] = asset(0, common::S_VOICE);
                optionsTallyOrdered.push_back(option.label);
            }
        }

        std::vector<Edge> edges = dao.getGraph().getEdgesFrom(proposal.getHash(), common::VOTE);
        for (auto edge : edges) {
            eosio::checksum256 voteHash = edge.getToNode();
            Vote voteDocument(dao, voteHash);

            optionsTally[voteDocument.getVote()] += voteDocument.getPower();
        }

        ContentGroups tallyContentGroups;
        for (auto option : optionsTallyOrdered) 
        {
            tallyContentGroups.push_back(ContentGroup{
                Content(CONTENT_GROUP_LABEL, option),
                Content(VOTE_POWER, optionsTally[option])
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