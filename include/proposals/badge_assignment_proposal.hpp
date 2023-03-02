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
        bool checkMembership(const eosio::name& proposer, ContentGroups &contentGroups) override;
        void proposeImpl(const name &proposer, ContentWrapper &contentWrapper) override;
        void passImpl(Document &proposal) override;
        void postProposeImpl(Document &proposal) override;
        void publishImpl(Document& proposal) override;
        string getBallotContent (ContentWrapper &contentWrapper) override;
        name getProposalType () override;
        bool isRecurring() override
        {
            return true;
        }
    private:
        Document getBadgeDoc(ContentGroups& cgs) const;
    };
}