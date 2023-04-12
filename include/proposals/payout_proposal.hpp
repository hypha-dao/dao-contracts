#pragma once

#include <eosio/name.hpp>

#include "proposal.hpp"

namespace hypha
{

    /**
     * @brief Payout proposal is used as a generic wrapper for different types of proposals
     * Contribution (Request for a payment)
     * Policy (Policy document)
     * Quest Start (start of a quest)
     * Quest Completion (Completion of Quest)
     * we should try to move those to their own classes as soon as we can, but for now they will 
     * be implemented here
     */
    class PayoutProposal : public Proposal
    {

    public:
        using Proposal::Proposal;
    protected:
        void proposeImpl(const name &proposer, ContentWrapper &contentWrapper) override;
        bool checkMembership(const eosio::name& proposer, ContentGroups &contentGroups) override;
        void passImpl(Document &proposal) override;
        string getBallotContent(ContentWrapper &contentWrapper) override;
        name getProposalType() override;

        void pay(Document &proposal, eosio::name edgeName);

        Document getParent(uint64_t from, const name& edgeName, const name& parentType);
        std::optional<Document> getParent(const char* parentItem, const name& parentType, ContentWrapper &contentWrapper, bool mustExist = false);

        void markRelatives(const name& link, const name& fromRelationship, const name& toRelationship, uint64_t docId, bool clear = false);
        //void markAncestry(const name& ancestryLink, uint64_t docId);
    private:
        asset calculateHusd(const asset &usd, const int64_t &deferred);
        asset calculateHypha(const asset &usd, const int64_t &deferred);
    };
} // namespace hypha