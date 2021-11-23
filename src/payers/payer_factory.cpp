#include <payers/payer_factory.hpp>
#include <payers/peg_payer.hpp>
#include <payers/reward_payer.hpp>
#include <payers/seeds_payer.hpp>
#include <payers/voice_payer.hpp>
#include <logger/logger.hpp>
#include <map>

#include <common.hpp>

namespace hypha
{

    Payer *PayerFactory::Factory(dao &dao, const eosio::symbol &symbol, const eosio::name &paymentType, const AssetBatch& daoTokens)
    {
        TRACE_FUNCTION()

        if (symbol.raw() == daoTokens.peg.symbol.raw()) {
            return new PegPayer(dao);
        }
        else if (symbol.raw() == daoTokens.reward.symbol.raw()) {
            return new RewardPayer(dao);
        }
        else if (symbol.raw() == daoTokens.voice.symbol.raw()) {
            return new VoicePayer(dao);
        }

        EOS_CHECK(false, "Unknown - symbol: " + symbol.code().to_string() + " payment type: " + paymentType.to_string());

        return nullptr;
    }
} // namespace hypha
