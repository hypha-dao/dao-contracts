#pragma once

#include <eosio/name.hpp>
#include <proposals/payout_proposal.hpp>

namespace hypha
{
    class QuestStartProposal : public PayoutProposal
    {
    public:
        using PayoutProposal::PayoutProposal;

    protected:
        void passImpl(Document &proposal) override;
        name getProposalType() override;

    };
}
