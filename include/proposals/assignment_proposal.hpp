#pragma once 

#include <eosio/name.hpp>
#include <eosio/asset.hpp>
#include <document_graph/content_wrapper.hpp>

#include "proposal.hpp"

namespace hypha {

    class AssignmentProposal : public Proposal
    {
    
    public:
        using Proposal::Proposal;

        Document propose(const name &proposer, ContentGroups &contentGroups);
        void close(Document &proposal);

    protected:

        void proposeImpl(const name &proposer, ContentWrapper &contentWrapper) override;
        void passImpl(Document &proposal) override;
        string getBallotContent (ContentWrapper &contentWrapper) override;
        name getProposalType () override;

    private: 

        asset calculateTimeShareUsdPerPeriod(const asset &annualUsd, const int64_t &timeShare);
        asset calculateHusd(const asset &annualUsd, const int64_t &timeShare, const int64_t &deferred);
        asset calculateHypha(const asset &annualUsd, const int64_t &timeShare, const int64_t &deferred);
        asset calculateHvoice(const asset &annualUsd, const int64_t &timeShare);
    };
}