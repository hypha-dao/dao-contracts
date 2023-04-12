#pragma once

#include <eosio/name.hpp>
#include <proposals/payout_proposal.hpp>

namespace hypha
{
    class PollProposal : public PayoutProposal
    {
    public:
        using PayoutProposal::PayoutProposal;
    protected:
        void postProposeImpl(Document &proposal) override;
        name getProposalType() override;
    };
}
