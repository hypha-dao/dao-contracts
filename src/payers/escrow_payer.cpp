#include <payers/escrow_payer.hpp>

#include <eosio/name.hpp>
#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>
#include <document_graph/document.hpp>

#include <common.hpp>

namespace hypha
{

    Document EscrowPayer::payImpl(const eosio::name &recipient,
                                  const eosio::asset &quantity,
                                  const string &memo)
    {

        // if the DSEEDS symbol is used, convert it to SEEDS
        eosio::asset updatedQuantity = quantity;
        if (quantity.symbol.code().raw() == common::S_DSEEDS.code().raw())
        {
            updatedQuantity = eosio::asset{quantity.amount, common::S_SEEDS};
        }

        eosio::action(
            eosio::permission_level{m_dao.get_self(), name("active")},
            m_dao.getSettingOrFail<name>(SEEDS_TOKEN_CONTRACT),
            eosio::name("transfer"),
            std::make_tuple(m_dao.get_self(), m_dao.getSettingOrFail<eosio::name>(SEEDS_ESCROW_CONTRACT), updatedQuantity, memo))
            .send();

        eosio::action(
            eosio::permission_level{m_dao.get_self(), name("active")},
            m_dao.getSettingOrFail<eosio::name>(SEEDS_ESCROW_CONTRACT), name("lock"),
            std::make_tuple(eosio::name("event"),
                            m_dao.get_self(),
                            recipient,
                            updatedQuantity,
                            eosio::name("golive"),
                            m_dao.get_self(),
                            eosio::time_point(eosio::current_time_point().time_since_epoch() +
                                              eosio::current_time_point().time_since_epoch()), // long time from now
                            memo))
            .send();

        ContentGroups recieptCgs{
            {Content(CONTENT_GROUP_LABEL, DETAILS),
             Content(RECIPIENT, recipient),
             Content(AMOUNT, updatedQuantity),
             Content(MEMO, memo),
             Content(PAYMENT_TYPE, ESCROW_SEEDS_AMOUNT),
             Content(EVENT, eosio::name("golive"))}};

        return Document(m_dao.get_self(), m_dao.get_self(), recieptCgs);
    }

} // namespace hypha