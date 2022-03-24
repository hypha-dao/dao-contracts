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
    Member::Member(dao& dao, const eosio::name& creator, const eosio::name& member)
        : Document(dao.get_self(), dao.get_self(), defaultContent(member)), m_dao(dao)
    {
        m_dao.addNameID<dao::member_table>(member, getID());
    }


    Member::Member(dao& dao, uint64_t docID)
        : Document(dao.get_self(), docID), m_dao(dao)
    {
    }


    ContentGroups Member::defaultContent(const eosio::name& member)
    {
        return(ContentGroups{
            ContentGroup{
                Content(CONTENT_GROUP_LABEL, DETAILS),
                Content(MEMBER_STRING, member) },
            ContentGroup{
                Content(CONTENT_GROUP_LABEL, SYSTEM),
                Content(TYPE, common::MEMBER),
                Content(NODE_LABEL, member.to_string()) } });
    }


    bool Member::isMember(dao& dao, uint64_t daoID, const eosio::name& member)
    {
        return(Edge::exists(dao.get_self(), daoID, dao.getMemberID(member), common::MEMBER));
    }


    bool Member::exists(dao& dao, const eosio::name& memberName)
    {
        dao::member_table m_t(dao.get_self(), dao.get_self().value);

        return(m_t.find(memberName.value) != m_t.end());
    }


    void Member::apply(uint64_t applyTo, const std::string content)
    {
        TRACE_FUNCTION()

        EOS_CHECK(
            !Edge::exists(getContract(), applyTo, getID(), common::MEMBER),
            util::to_str("You are a member of this DAO already")
            )

        Edge::write(getContract(), getAccount(), applyTo, getID(), common::APPLICANT);
        Edge::write(getContract(), getAccount(), getID(), applyTo, common::APPLICANT_OF);
    }


    void Member::enroll(const eosio::name& enroller, uint64_t appliedTo, const std::string& content)
    {
        TRACE_FUNCTION()

        uint64_t rootID = appliedTo;

        // create the new member edges
        Edge::write(getContract(), enroller, rootID, getID(), common::MEMBER);
        Edge::write(getContract(), enroller, getID(), rootID, common::MEMBER_OF);

        // remove the old applicant edges
        Edge rootApplicantEdge = Edge::get(getContract(), rootID, getID(), common::APPLICANT);

        rootApplicantEdge.erase();

        Edge applicantRootEdge = Edge::get(getContract(), getID(), rootID, common::APPLICANT_OF);

        applicantRootEdge.erase();

        // TODO: add as configuration setting for genesis amount
        // TODO: connect the payment receipt to the period also
        // TODO: change Payer.hpp to NOT require m_dao so this payment can be made using payer factory

        Document daoDoc(getContract(), appliedTo);
        auto     daoCW = daoDoc.getContentWrapper();

        auto daoName = daoCW.getOrFail(DETAILS, DAO_NAME)->getAs<name>();

        auto daoSettings = m_dao.getSettingsDocument(appliedTo);

        auto voiceToken = daoSettings->getOrFail<asset>(common::VOICE_TOKEN);

        eosio::asset genesis_voice{ getTokenUnit(voiceToken), voiceToken.symbol };
        std::string  memo = util::to_str("genesis voice issuance during enrollment to ", daoName);

        name hyphaHvoice = m_dao.getSettingOrFail<eosio::name>(GOVERNANCE_TOKEN_CONTRACT);

        hypha::issueTenantToken(
            hyphaHvoice,
            daoName,
            getContract(),
            getAccount(),
            genesis_voice,
            memo
            );

        Document paymentReceipt(getContract(), getContract(), Payer::defaultReceipt(getAccount(), genesis_voice, memo));

        Edge::write(getContract(), getAccount(), getID(), paymentReceipt.getID(), common::PAYMENT);

        // eosio::action(
        //     eosio::permission_level{getContract(), name("active")},
        //     name("eosio"), name("buyram"),
        //     std::make_tuple(getContract(), getAccount(), common::RAM_ALLOWANCE))
        //     .send();
    }


    eosio::name Member::getAccount()
    {
        TRACE_FUNCTION()
        return(getContentWrapper().getOrFail(DETAILS, MEMBER_STRING)->getAs<eosio::name>());
    }
} // namespace hypha
