#pragma once

#include <cstring>
#include <eosio/name.hpp>

#include <document_graph/content_wrapper.hpp>
#include <document_graph/document.hpp>

#include "proposal.hpp"

namespace hypha {
    class BadgeProposal : public Proposal
    {
public:
        using Proposal::Proposal;

protected:
        void proposeImpl(const eosio::name& proposer, ContentWrapper& contentWrapper) override;
        void passImpl(Document& proposal) override;
        std::string getBallotContent(ContentWrapper& contentWrapper) override;
        eosio::name getProposalType() override;

private:
        void checkCoefficient(ContentWrapper& badge, const std::string& key);
    };
}
