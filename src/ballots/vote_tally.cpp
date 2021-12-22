#include <ballots/vote_tally.hpp>
#include <dao.hpp>
#include <common.hpp>
#include <document_graph/edge.hpp>
#include <ballots/vote.hpp>
#include <logger/logger.hpp>
#include <settings.hpp>

namespace hypha
{

    VoteTally::VoteTally(dao& dao, uint64_t id)
    : TypedDocument(dao, id)
    {
      TRACE_FUNCTION()
    }

    VoteTally::VoteTally(
        dao& dao,
        Document& proposal, 
        Settings* daoSettings
    )
    : TypedDocument(dao)
    {
        TRACE_FUNCTION()
        auto [exists, oldTally] = Edge::getIfExists(dao.get_self(), proposal.getID(), common::VOTE_TALLY);
        if (exists) {
            auto oldTallyNode = oldTally.to_node;
            oldTally.erase();

            if (!dao.getGraph().hasEdges(oldTallyNode)) {
                dao.getGraph().eraseDocument(oldTallyNode, false);
            }
            
        }

        ContentGroup* contentOptions = proposal.getContentWrapper().getGroupOrFail(BALLOT_OPTIONS);

        auto voiceToken = daoSettings->getOrFail<eosio::asset>(common::VOICE_TOKEN);

        std::map<std::string, eosio::asset> optionsTally;
        std::vector<std::string> optionsTallyOrdered;
        for (auto option : *contentOptions) 
        {
            if (option.label != CONTENT_GROUP_LABEL) {
                optionsTally[option.label] = asset(0, voiceToken.symbol);
                optionsTallyOrdered.push_back(option.label);
            }
        }

        std::vector<Edge> edges = dao.getGraph().getEdgesFrom(proposal.getID(), common::VOTE);
        for (auto edge : edges) {
            uint64_t voteID = edge.getToNode();
            Vote voteDocument(dao, voteID);

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

        Edge::write(dao.get_self(), dao.get_self(), proposal.getID(), getDocument().getID(), common::VOTE_TALLY);
    }

    const std::string VoteTally::buildNodeLabel(ContentGroups &content)
    {
        return "VoteTally";
    }
}