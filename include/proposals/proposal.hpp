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

        void close(Document &proposal);

        dao &m_dao;

    protected:
        virtual void proposeImpl(const eosio::name &proposer,
                                 ContentWrapper &contentWrapper) = 0;

        virtual void postProposeImpl(Document &proposal);

        virtual void passImpl(Document &proposal) = 0;

        virtual string getBallotContent(ContentWrapper &contentWrapper) = 0;

        virtual name getProposalType() = 0;

        ContentGroup createSystemGroup(const name &proposer,
                                       const name &proposal_type,
                                       const string &decide_title,
                                       const string &decide_desc,
                                       const string &decide_content);

        bool didPass(const name &ballot_id);

        name registerBallot(const name &proposer,
                            const std::map<string, string> &strings);

        name registerBallot(const name &proposer,
                            const string &title, const string &description, const string &content);
    };
} // namespace hypha