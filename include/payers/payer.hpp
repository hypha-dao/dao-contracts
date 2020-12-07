#pragma once
#include <string>
#include <eosio/name.hpp>
#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>
#include <dao.hpp>
#include <document_graph/document.hpp>

namespace hypha
{

    class Payer
    {

    public:
        Payer(dao &dao);
        virtual ~Payer();

        Document *pay(const eosio::name &recipient,
                      const eosio::asset &quantity,
                      const string &memo);

    protected:
        virtual Document *payImpl(const eosio::name &recipient,
                                  const eosio::asset &quantity,
                                  const string &memo) = 0;

        void issueToken(const name &token_contract,
                               const name &issuer,
                               const name &to,
                               const asset &token_amount,
                               const string &memo);

        dao &m_dao;

    private:
    };
} // namespace hypha