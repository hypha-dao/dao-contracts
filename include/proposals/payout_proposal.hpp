#pragma once

#include <eosio/name.hpp>

#include "proposal.hpp"

namespace hypha
{

    class PayoutProposal : public Proposal
    {

    public:
        using Proposal::Proposal;

        std::optional<name> getSubType(ContentWrapper &contentWrapper);
    protected:
        void proposeImpl(const name &proposer, ContentWrapper &contentWrapper) override;
        void postProposeImpl(Document &proposal) override;
        void passImpl(Document &proposal) override;
        void failImpl(Document &proposal) override;
        string getBallotContent(ContentWrapper &contentWrapper) override;
        name getProposalType() override;

        void pay(Document &proposal, eosio::name edgeName);

    private:
        asset calculateHusd(const asset &usd, const int64_t &deferred);
        asset calculateHypha(const asset &usd, const int64_t &deferred);

        bool isPayable(ContentWrapper &contentWrapper);

        std::optional<Document> getParent(const char* parentItem, const name& parentType, ContentWrapper &contentWrapper, bool mustExist = false);
        Document getParent(uint64_t from, const name& edgeName, const name& parentType);

        //void checkPayoutFields(const name &proposer, ContentWrapper &contentWrapper);
        void checkPaymentFields(const name& proposer, ContentWrapper &contentWrapper, bool onlyCoreMembers);
        void checkPolicyFields(const name &proposer, ContentWrapper &contentWrapper);
        void checkQuestStartFields(const name &proposer, ContentWrapper &contentWrapper);
        void checkQuestCompletionFields(const name &proposer, ContentWrapper &contentWrapper);
    };
} // namespace hypha