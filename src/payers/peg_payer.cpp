#include <payers/peg_payer.hpp>

#include <eosio/name.hpp>
#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>
#include <document_graph/document.hpp>
#include <logger/logger.hpp>

#include <common.hpp>

namespace hypha
{

    Document PegPayer::payImpl(const eosio::name &recipient,
                                 const eosio::asset &quantity,
                                 const string &memo)
    {
        TRACE_FUNCTION()
        issueTenantToken(m_dao.getSettingOrFail<eosio::name>(PEG_TOKEN_CONTRACT),
                         m_daoSettings->getOrFail<name>(DAO_NAME),
                         m_dao.getSettingOrFail<eosio::name>(TREASURY_CONTRACT),
                         recipient,
                         quantity,
                         memo);

        return Document(m_dao.get_self(),
                        m_dao.get_self(),
                        defaultReceipt(recipient, quantity, memo));
    }

} // namespace hypha