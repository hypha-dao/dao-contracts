#include <payers/reward_payer.hpp>

#include <eosio/name.hpp>
#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>
#include <document_graph/document.hpp>
#include <logger/logger.hpp>

#include <common.hpp>

namespace hypha
{
    Document RewardPayer::payImpl(const eosio::name&recipient,
                                  const eosio::asset&quantity,
                                  const string&memo)
    {
        TRACE_FUNCTION()
        issueTenantToken(m_dao.getSettingOrFail <eosio::name>(REWARD_TOKEN_CONTRACT),
                         m_daoSettings->getOrFail <name>(DAO_NAME),
                         m_dao.get_self(),
                         recipient,
                         quantity,
                         memo);

        return(Document(m_dao.get_self(),
                        m_dao.get_self(),
                        defaultReceipt(recipient, quantity, memo)));
    }
} // namespace hypha
