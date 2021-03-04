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

        Document propose(const eosio::name &proposer, ContentGroups &contentGroups);

        void vote(const eosio::name &voter, const std::string vote, Document& proposal);
        void close(Document &proposal);

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
                                       const string &proposal_title);

        ContentGroup makeBallotGroup();
        ContentGroup makeBallotOptionsGroup();

        void updateVoteTally(Document& proposal, const eosio::name creator);

        bool didPass(const eosio::checksum256 &tallyHash);

    private:
        bool oldDidPass(const eosio::name &ballotId);

    };
} // namespace hypha