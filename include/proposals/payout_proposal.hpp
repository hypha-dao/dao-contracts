#pragma once

#include <eosio/name.hpp>

#include "proposal.hpp"

namespace hypha
{

    class PayoutProposal : public Proposal
    {

    public:
        using Proposal::Proposal;

        Document propose(const name &proposer, ContentGroups &contentGroups);
        void close(Document &proposal);

    protected:
        void proposeImpl(const name &proposer, ContentWrapper &contentWrapper) override;
        void passImpl(Document &proposal) override;
        string getBallotContent(ContentWrapper &contentWrapper) override;
        name getProposalType() override;

        void pay(Document &proposal, eosio::name edgeName);

    private:
        asset calculateHusd(const asset &usd, const int64_t &deferred);
        asset calculateHypha(const asset &usd, const int64_t &deferred);
    };
} // namespace hypha