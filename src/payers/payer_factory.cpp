#include <payers/payer_factory.hpp>
#include <payers/husd_payer.hpp>
#include <payers/reward_payer.hpp>
#include <payers/seeds_payer.hpp>
#include <payers/voice_payer.hpp>
#include <payers/escrow_payer.hpp>
#include <logger/logger.hpp>

#include <common.hpp>

namespace hypha
{

    Payer *PayerFactory::Factory(dao &dao, const eosio::symbol &symbol, const eosio::name &paymentType)
    {
        TRACE_FUNCTION()

        if (paymentType == common::ESCROW)
        {
            return new EscrowPayer(dao);
        }

        switch (symbol.code().raw())
        {
        case common::S_PEG.code().raw():
            return new HusdPayer(dao);

        case common::S_VOICE.code().raw():
            return new VoicePayer(dao);

        case common::S_REWARD.code().raw():
            return new RewardPayer(dao);

        case common::S_SEEDS.code().raw():
            return new SeedsPayer(dao);
        }

        EOS_CHECK(false, "Unknown - symbol: " + symbol.code().to_string() + " payment type: " + paymentType.to_string());
        return nullptr;
    }
} // namespace hypha
