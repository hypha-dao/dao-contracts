
#include <payers/payer.hpp>
#include <member.hpp>
#include <util.hpp>
#include <logger/logger.hpp>

namespace hypha
{

    Payer::Payer(dao &dao) : m_dao(dao) {}
    Payer::~Payer() {}

    Document Payer::pay(const eosio::name &recipient,
                        const eosio::asset &quantity,
                        const string &memo)

    {
        TRACE_FUNCTION()
        return payImpl(recipient, quantity, memo);
    }

    void Payer::issueToken(const eosio::name &token_contract,
                           const eosio::name &issuer,
                           const eosio::name &to,
                           const eosio::asset &token_amount,
                           const string &memo)
    {
        TRACE_FUNCTION()
        hypha::issueToken(
            token_contract,
            issuer,
            to,
            token_amount,
            memo
        );
    }

    ContentGroups Payer::defaultReceipt(const eosio::name &recipient,
                                        const eosio::asset &quantity,
                                        const string &memo)
    {
        return ContentGroups{
            ContentGroup{
                Content(CONTENT_GROUP_LABEL, DETAILS),
                Content(RECIPIENT, recipient),
                Content(AMOUNT, quantity),
                Content(MEMO, memo)},
            ContentGroup{
                Content(CONTENT_GROUP_LABEL, SYSTEM),
                Content(TYPE, common::PAYMENT),
                Content(NODE_LABEL, quantity.to_string() + " to " + recipient.to_string())}};
    }

} // namespace hypha