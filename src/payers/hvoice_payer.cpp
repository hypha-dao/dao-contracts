#include <payers/hvoice_payer.hpp>

#include <eosio/name.hpp>
#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>
#include <document_graph/document.hpp>

#include <common.hpp>

namespace hypha
{

    Document HvoicePayer::payImpl(const eosio::name &recipient,
                                  const eosio::asset &quantity,
                                  const string &memo)
    {
        eosio::action(
            eosio::permission_level{m_dao.get_self(), eosio::name("active")},
            m_dao.getSettingOrFail<name>(TELOS_DECIDE_CONTRACT), eosio::name("mint"),
            std::make_tuple(recipient, quantity, memo))
            .send();

        return Document(m_dao.get_self(),
                        m_dao.get_self(),
                        defaultReceipt(recipient, quantity, memo));
    }

} // namespace hypha