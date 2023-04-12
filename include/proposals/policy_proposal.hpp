#pragma once

#include <eosio/name.hpp>
#include <proposals/payout_proposal.hpp>

namespace hypha
{
    class PolicyProposal : public PayoutProposal
    {
    public:
        using PayoutProposal::PayoutProposal;

    protected:
        bool checkMembership(const eosio::name& proposer, ContentGroups &contentGroups) override;
        void proposeImpl(const name &proposer, ContentWrapper &contentWrapper) override;
        void failImpl(Document &proposal) override;
        void postProposeImpl(Document &proposal) override;
        name getProposalType() override;
    };
}
