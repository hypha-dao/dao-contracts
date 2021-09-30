#pragma once

#include <eosio/name.hpp>
#include <document_graph/document.hpp>
#include <document_graph/content_wrapper.hpp>
#include "proposal.hpp"

namespace hypha {

    class BadgeAssignmentProposal : public hypha::Proposal
    {

    public:
        using Proposal::Proposal;
        
    protected:
        
        void proposeImpl(const name &proposer, ContentWrapper &contentWrapper) override;
        void passImpl(Document &proposal) override;
        string getBallotContent (ContentWrapper &contentWrapper) override;
        name getProposalType () override;
    };
}