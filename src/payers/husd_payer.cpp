#include <payers/husd_payer.hpp>

#include <eosio/name.hpp>
#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>
#include <document_graph/document.hpp>

#include <common.hpp>

namespace hypha
{

    Document HusdPayer::payImpl(const eosio::name &recipient,
                                 const eosio::asset &quantity,
                                 const string &memo)
    {

        issueToken(m_dao.getSettingOrFail<eosio::name>(HUSD_TOKEN_CONTRACT),
                   m_dao.getSettingOrFail<eosio::name>(TREASURY_CONTRACT),
                   recipient,
                   quantity,
                   memo);

        ContentGroups recieptCgs{
            {Content(CONTENT_GROUP_LABEL, DETAILS),
             Content(RECIPIENT, recipient),
             Content(AMOUNT, quantity),
             Content(MEMO, memo)}};

        return Document(m_dao.get_self(), m_dao.get_self(), recieptCgs);
    }

} // namespace hypha