#include <member.hpp>

#include <cmath>

#include <common.hpp>
#include <document_graph/document_graph.hpp>
#include <document_graph/document.hpp>
#include <document_graph/content.hpp>
#include <document_graph/edge.hpp>
#include <document_graph/util.hpp>
#include <payers/payer.hpp>
#include <util.hpp>
#include <dao.hpp>
#include <payers/payer.hpp>
#include <logger/logger.hpp>

namespace hypha
{
    Member::Member(dao& dao, const eosio::name &creator, const eosio::name &member)
        : Document(dao.get_self(), dao.get_self(), defaultContent(member)), m_dao(dao)
    {
    }

    Member::Member(dao& dao, const eosio::checksum256 &hash)
        : Document(dao.get_self(), hash), m_dao(dao)
    {
    }

    Member::Member(dao& dao, uint64_t docID)
        : Document(dao.get_self(), docID), m_dao(dao)
    {
    }

    Member Member::get(dao& dao, const eosio::name &member)
    {
        return Member(dao, Member::calcHash(member));
    }

    ContentGroups Member::defaultContent(const eosio::name &member)
    {
        return ContentGroups{
            ContentGroup{
                Content(CONTENT_GROUP_LABEL, DETAILS),
                Content(MEMBER_STRING, member)},
            ContentGroup{
                Content(CONTENT_GROUP_LABEL, SYSTEM),
                Content(TYPE, common::MEMBER),
                Content(NODE_LABEL, member.to_string())}};
    }

    const eosio::checksum256 Member::calcHash(const eosio::name &member)
    {
        auto cgs = Member::defaultContent(member);
        return Document::hashContents(cgs);
    }

    const bool Member::isMember(const eosio::name& contract, uint64_t daoID, const eosio::name &member)
    {
        // create hash to represent this member account
        auto memberHash = Member::calcHash(member);
        
        Document memDoc(contract, memberHash);

        return Edge::exists(contract, daoID, memDoc.primary_key(), common::MEMBER);
    }

    // Member Member::getOrNew(eosio::name contract, const eosio::name &creator, const eosio::name &member)
    // {
    //     return (Member) Document::getOrNew(contract, creator, Document::rollup(Content(MEMBER_STRING, member)));
    // }

    void Member::apply(uint64_t applyTo, const std::string content)
    {
        TRACE_FUNCTION()
        Edge::write(getContract(), getAccount(), applyTo, primary_key(), common::APPLICANT);
        Edge::write(getContract(), getAccount(), primary_key(), applyTo, common::APPLICANT_OF);
    }

    void Member::enroll(const eosio::name &enroller, uint64_t appliedTo, const std::string &content)
    {
        TRACE_FUNCTION()
        
        uint64_t rootID = appliedTo;

        // create the new member edges
        Edge::write(getContract(), enroller, rootID, primary_key(), common::MEMBER);
        Edge::write(getContract(), enroller, primary_key(), rootID, common::MEMBER_OF);

        // remove the old applicant edges
        Edge rootApplicantEdge = Edge::get(getContract(), rootID, primary_key(), common::APPLICANT);
        rootApplicantEdge.erase();

        Edge applicantRootEdge = Edge::get(getContract(), primary_key(), rootID, common::APPLICANT_OF);
        applicantRootEdge.erase();

        // TODO: add as configuration setting for genesis amount
        // TODO: connect the payment receipt to the period also
        // TODO: change Payer.hpp to NOT require m_dao so this payment can be made using payer factory

        Document daoDoc(getContract(), appliedTo);
        auto daoCW = daoDoc.getContentWrapper();

        auto daoName = daoCW.getOrFail(DETAILS, DAO_NAME)->getAs<name>();

        auto voiceToken = m_dao.getSettingOrFail<asset>(daoName, common::VOICE_TOKEN);
      
        eosio::asset genesis_voice{getTokenUnit(voiceToken), voiceToken.symbol};
        std::string memo = util::to_str("genesis voice issuance during enrollment to ", daoName);

        name hyphaHvoice = m_dao.getSettingOrFail<eosio::name>(GOVERNANCE_TOKEN_CONTRACT);

        hypha::issueToken(
            hyphaHvoice,
            getContract(),
            getAccount(),
            genesis_voice,
            memo
        );

        Document paymentReceipt(getContract(), getContract(), Payer::defaultReceipt(getAccount(), genesis_voice, memo));

        Edge::write(getContract(), getAccount(), primary_key(), paymentReceipt.primary_key(), common::PAYMENT);

        // eosio::action(
        //     eosio::permission_level{getContract(), name("active")},
        //     name("eosio"), name("buyram"),
        //     std::make_tuple(getContract(), getAccount(), common::RAM_ALLOWANCE))
        //     .send();
    }

    eosio::name Member::getAccount()
    {
        TRACE_FUNCTION()
        return getContentWrapper().getOrFail(DETAILS, MEMBER_STRING)->getAs<eosio::name>();
    }

} // namespace hypha