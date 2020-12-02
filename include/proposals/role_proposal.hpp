#pragma once 

#include <eosio/name.hpp>

#include "proposal.hpp"

namespace hypha {

    class RoleProposal : public Proposal
    {
    
    public:
        using Proposal::Proposal;

        Document propose(const name &proposer, ContentGroups &content_groups);
        void close(Document proposal);

    protected:

        ContentGroups propose_impl(const name &proposer, ContentGroups &content_groups) override;
        Document pass_impl(Document proposal) override;
        string GetBallotContent (ContentGroups proposal_details) override;
        name GetProposalType () override;

    };
}