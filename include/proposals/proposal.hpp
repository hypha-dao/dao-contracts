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
        virtual void propose_impl(const eosio::name &proposer,
                                  ContentWrapper &contentWrapper) = 0;

        virtual void pass_impl(Document &proposal) = 0;

        virtual string GetBallotContent(ContentWrapper &contentWrapper) = 0;

        virtual name GetProposalType() = 0;

        ContentGroup create_system_group(const name &proposer,
                                         const name &proposal_type,
                                         const string &decide_title,
                                         const string &decide_desc,
                                         const string &decide_content);

        void verify_membership(const name &member);

        bool did_pass(const name &ballot_id);

        name register_ballot(const name &proposer,
                             const std::map<string, string> &strings);

        name register_ballot(const name &proposer,
                             const string &title, const string &description, const string &content);

        asset adjustAsset(const asset &originalAsset, const float &adjustment);
    };
} // namespace hypha