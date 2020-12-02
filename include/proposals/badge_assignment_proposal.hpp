#pragma once

#include <eosio/name.hpp>
#include <document_graph/document.hpp>
#include <document_graph/content_group.hpp>
#include "proposal.hpp"

namespace hypha {

    class BadgeAssignmentProposal : public hypha::Proposal
    {

    public:
        using Proposal::Proposal;

        Document propose(const name &proposer, ContentGroups &content_groups);
        void close(Document proposal);

    protected:
        
        ContentGroups propose_impl(const name &proposer, ContentGroups &content_groups) override;
        Document pass_impl(Document proposal) override;
        string GetBallotContent (ContentGroups contentGroups) override;
        name GetProposalType () override;
    };
}