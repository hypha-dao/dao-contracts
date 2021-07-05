#pragma once
#include <string>
#include <eosio/name.hpp>
#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>
#include <document_graph/document.hpp>

#include "payer.hpp"

namespace hypha
{

    class VoicePayer : public Payer
    {
    public:
        using Payer::Payer;

        Document pay(const eosio::name &recipient,
                      const eosio::asset &quantity,
                      const string &memo);

    protected:
        Document payImpl(const eosio::name &recipient,
                          const eosio::asset &quantity,
                          const string &memo) override;
    };
} // namespace hypha