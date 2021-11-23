#pragma once 

#include <eosio/name.hpp>
#include <eosio/asset.hpp>
#include <document_graph/content_wrapper.hpp>
#include <util.hpp>

#include "proposal.hpp"

namespace hypha {

    class AssignmentProposal : public Proposal
    {
    
    public:
        using Proposal::Proposal;

    protected:

        void proposeImpl(const name &proposer, ContentWrapper &contentWrapper) override;
        void passImpl(Document &proposal) override;
        string getBallotContent (ContentWrapper &contentWrapper) override;
        name getProposalType () override;

        void postProposeImpl(Document &proposal) override;
        
    private: 

        asset calculateTimeShareUsdPerPeriod(const SalaryConfig& salaryConf);
    };
}