#pragma once

#include <eosio/name.hpp>

#include "proposal.hpp"

namespace hypha {
    class EditProposal : public Proposal
    {
        public:
            using Proposal::Proposal;

        protected:

            void proposeImpl(const name&proposer, ContentWrapper&contentWrapper) override;
            void postProposeImpl(Document&proposal) override;

            void passImpl(Document&proposal) override;
            string getBallotContent(ContentWrapper&contentWrapper) override;
            name getProposalType() override;
    };
}
