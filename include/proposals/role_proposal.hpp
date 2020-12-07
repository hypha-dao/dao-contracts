#pragma once 

#include <eosio/name.hpp>

#include "proposal.hpp"

namespace hypha {

    class RoleProposal : public Proposal
    {
    
    public:
        using Proposal::Proposal;

        Document propose(const name &proposer, ContentGroups &contentGroups);
        void close(Document &proposal);

    protected:

        void propose_impl(const name &proposer, ContentWrapper &contentWrapper) override;
        void pass_impl(Document &proposal) override;
        string GetBallotContent (ContentWrapper &contentWrapper) override;
        name GetProposalType () override;

    };
}