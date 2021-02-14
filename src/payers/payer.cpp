
#include <payers/payer.hpp>
#include <member.hpp>

namespace hypha
{

    Payer::Payer(dao &dao) : m_dao(dao) {}
    Payer::~Payer() {}

    Document Payer::pay(const eosio::name &recipient,
                        const eosio::asset &quantity,
                        const string &memo)

    {
        return payImpl(recipient, quantity, memo);
    }

    void Payer::issueToken(const eosio::name &token_contract,
                           const eosio::name &issuer,
                           const eosio::name &to,
                           const eosio::asset &token_amount,
                           const string &memo)
    {
        eosio::action(
            eosio::permission_level{issuer, eosio::name("active")},
            token_contract, eosio::name("issue"),
            std::make_tuple(issuer, token_amount, memo))
            .send();

        eosio::action(
            eosio::permission_level{issuer, eosio::name("active")},
            token_contract, eosio::name("transfer"),
            std::make_tuple(issuer, to, token_amount, memo))
            .send();
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