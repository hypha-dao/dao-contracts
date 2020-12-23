#include <payers/seeds_payer.hpp>

#include <eosio/name.hpp>
#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>
#include <document_graph/document.hpp>

#include <common.hpp>

namespace hypha
{

    Document SeedsPayer::payImpl(const eosio::name &recipient,
                                 const eosio::asset &quantity,
                                 const string &memo)
    {

        eosio::action(
            eosio::permission_level{m_dao.get_self(), name("active")},
            m_dao.getSettingOrFail<name>(SEEDS_TOKEN_CONTRACT),
            eosio::name("transfer"),
            std::make_tuple(m_dao.get_self(), recipient, quantity, memo))
            .send();

        std::vector<ContentGroup> recieptCgs{
            {Content(CONTENT_GROUP_LABEL, DETAILS),
             Content(RECIPIENT, recipient),
             Content(AMOUNT, quantity),
             Content(MEMO, memo)}};

        return Document(m_dao.get_self(), m_dao.get_self(), recieptCgs);
    }

} // namespace hypha