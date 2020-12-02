#pragma once

#include <eosio/name.hpp>
#include "proposal.hpp"

namespace hypha {

    class BadgeProposal : public Proposal
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

    private:
        void checkCoefficient(Content coefficient);

    };
}