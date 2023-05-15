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
    class Section;

    class Proposal
    {

    public:
        Proposal(dao &contract, uint64_t daoID);
        virtual ~Proposal();

        Document propose(const eosio::name &proposer, ContentGroups &contentGroups, bool publish);

        void vote(const eosio::name &voter, const std::string vote, Document& proposal, std::optional<std::string> notes);
        void close(Document &proposal);
        void publish(const eosio::name &proposer, Document &proposal);
        void remove(const eosio::name &proposer, Document &proposal);
        void update(const eosio::name &proposer, Document &proposal, ContentGroups &contentGroups);

        dao &m_dao;

    protected:

        virtual bool checkMembership(const eosio::name& proposer, ContentGroups &contentGroups)
        { return false; }

        virtual void proposeImpl(const eosio::name &proposer,
                                 ContentWrapper &contentWrapper) = 0;

        virtual void postProposeImpl(Document &proposal);

        virtual void passImpl(Document &proposal) = 0;

        virtual void failImpl(Document &proposal) {};

        virtual void publishImpl(Document& proposal) {}

        virtual string getBallotContent(ContentWrapper &contentWrapper) = 0;

        virtual name getProposalType() = 0;

        ContentGroup makeSystemGroup(const name &proposer,
                                       const name &proposal_type,
                                       const string &proposal_title,
                                       const string &proposal_description);

        ContentGroup makeBallotGroup();
        ContentGroup makeBallotOptionsGroup();

        bool didPass(Document& proposal, uint64_t tallyHash);

        string getTitle(ContentWrapper cw) const;
        string getDescription(ContentWrapper cw) const;

        std::pair<bool, uint64_t> hasOpenProposal(name proposalType, uint64_t docID);
        
        eosio::asset getVoiceSupply(Document& proposal);

        std::optional<Document> getItemDocOpt(const char* docItem, const name& docType, ContentWrapper &contentWrapper);
        Document getItemDoc(const char* docItem, const name& docType, ContentWrapper &contentWrapper);
        Document getLinkDoc(uint64_t from, const name& link, const name& docType);
    private:
        bool oldDidPass(const eosio::name &ballotId);

        Document internalPropose(const eosio::name &proposer, ContentGroups &contentGroups, bool publish, Section* commentSection);

        void internalClose(Document &proposal, bool pass);

        virtual bool isRecurring() { return false; }
    protected:
        Settings* m_daoSettings;
        Settings* m_dhoSettings;
        uint64_t m_daoID;
        //Should be marked if we require the proposal to pass automatically
        bool selfApprove = false;
    
        void _publish(const eosio::name &proposer, Document &proposal, uint64_t rootID);
        name _newCommentSection();

    };
} // namespace hypha