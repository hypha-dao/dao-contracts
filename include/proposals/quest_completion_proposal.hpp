#pragma once

#include <eosio/name.hpp>
#include <proposals/payout_proposal.hpp>

namespace hypha
{
    class QuestCompletionProposal : public PayoutProposal
    {
    public:
        using PayoutProposal::PayoutProposal;

    protected:
        void proposeImpl(const name &proposer, ContentWrapper &contentWrapper) override;
        void postProposeImpl(Document &proposal) override;
        void passImpl(Document &proposal) override;
        void failImpl(Document &proposal) override;
        name getProposalType() override;
    };
}
