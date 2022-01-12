#include <payers/hypha_payer.hpp>

#include <eosio/name.hpp>
#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>
#include <document_graph/document.hpp>
#include <logger/logger.hpp>

#include <common.hpp>

// 3 year expiry
#define EXPIRY_MILLISECONDS eosio::microseconds(3 * 365 * 24 * 60 * 60 * 1000)
namespace hypha
{

    Document HyphaPayer::payImpl(const eosio::name &recipient,
                                 const eosio::asset &quantity,
                                 const string &memo)
    {
        TRACE_FUNCTION()
        issueToken(m_dao.getSettingOrFail<eosio::name>(HYPHA_TOKEN_CONTRACT),
                   m_dao.get_self(),
                   m_dao.getSettingOrFail<eosio::name>(HYPHA_COSALE_CONTRACT),
                   quantity,
                   "");

        eosio::action(
            eosio::permission_level{m_dao.get_self(), name("active")},
            m_dao.getSettingOrFail<eosio::name>(HYPHA_COSALE_CONTRACT), name("createlock"),
            std::make_tuple(m_dao.get_self(),
                            recipient,
                            quantity,
                            memo,
                            eosio::time_point(eosio::current_time_point().time_since_epoch() + EXPIRY_MILLISECONDS)))
            .send();

        return Document(m_dao.get_self(),
                        m_dao.get_self(),
                        defaultReceipt(recipient, quantity, memo));
    }

} // namespace hypha