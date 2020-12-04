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

        Document propose(const eosio::name &proposer, ContentGroups &contentGroups);
        void close(Document &proposal);

    protected:
        void propose_impl(const eosio::name &proposer, ContentWrapper &contentWrapper) override;
        void pass_impl(Document &proposal) override;
        std::string GetBallotContent (ContentWrapper &contentWrapper) override;
        eosio::name GetProposalType () override;

    private:
        void checkCoefficient(ContentWrapper &badge, const std::string &key);

    };
}