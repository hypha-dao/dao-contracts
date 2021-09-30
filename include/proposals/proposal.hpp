#pragma once
#include <eosio/name.hpp>
#include <document_graph/document_graph.hpp>
#include <document_graph/content_wrapper.hpp>
#include <dao.hpp>

using eosio::asset;
using eosio::name;
using std::string;

namespace hypha
{

    class Proposal
    {

    public:
        Proposal(dao &contract);
        virtual ~Proposal();

        Document propose(const eosio::name &proposer, ContentGroups &contentGroups, bool publish);

        void vote(const eosio::name &voter, const std::string vote, Document& proposal, std::optional<std::string> notes);
        void close(Document &proposal);
        void publish(const eosio::name &proposer, Document &proposal);
        void remove(const eosio::name &proposer, Document &proposal);
        void update(const eosio::name &proposer, Document &proposal, ContentGroups &contentGroups);

        dao &m_dao;

    protected:
        virtual void proposeImpl(const eosio::name &proposer,
                                 ContentWrapper &contentWrapper) = 0;

        virtual void postProposeImpl(Document &proposal);

        virtual void passImpl(Document &proposal) = 0;

        virtual string getBallotContent(ContentWrapper &contentWrapper) = 0;

        virtual name getProposalType() = 0;

        ContentGroup makeSystemGroup(const name &proposer,
                                       const name &proposal_type,
                                       const string &proposal_title,
                                       const string &proposal_description);

        ContentGroup makeBallotGroup();
        ContentGroup makeBallotOptionsGroup();

        bool didPass(const eosio::checksum256 &tallyHash);

        string getTitle(ContentWrapper cw) const;
        string getDescription(ContentWrapper cw) const;

        std::pair<bool, checksum256> hasOpenProposal(name proposalType, checksum256 docHash);
    private:
        bool oldDidPass(const eosio::name &ballotId);

        void _publish(const eosio::name &proposer, Document &proposal, eosio::checksum256 root);

    };
} // namespace hypha