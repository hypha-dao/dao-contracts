#include <payers/hypha_payer.hpp>

#include <eosio/name.hpp>
#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>
#include <document_graph/document.hpp>

#include <common.hpp>

namespace hypha
{

    Document *HyphaPayer::payImpl(const eosio::name &recipient,
                                  const eosio::asset &quantity,
                                  const string &memo)
    {

        issueToken(m_dao.getSettingOrFail<eosio::name>(common::HYPHA_TOKEN_CONTRACT),
                   m_dao.get_self(),
                   recipient,
                   quantity,
                   memo);

        ContentGroups recieptCgs{
            {Content(CONTENT_GROUP_LABEL, common::DETAILS),
             Content(common::RECIPIENT, recipient),
             Content(common::AMOUNT, quantity),
             Content(common::MEMO, memo)}};

        Document receipt(m_dao.get_self(), m_dao.get_self(), recieptCgs);
        return &receipt;
    }

} // namespace hypha