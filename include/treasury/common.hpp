#pragma once

#include <eosio/name.hpp>
#include <common.hpp>

namespace hypha::treasury::common {

#ifdef USE_TREASURY
    
namespace types {
    inline constexpr auto TREASURY = eosio::name("treasury");
    inline constexpr auto TREASURY_PAYMENT = eosio::name("trsy.payment");
    inline constexpr auto REDEMPTION = eosio::name("redemption");
    inline constexpr auto ATTESTATION = eosio::name("attestation");
    inline constexpr auto BALANCE = eosio::name("balance");
}

namespace links {
    inline constexpr auto TREASURY_OF = eosio::name("treasuryof");
    inline constexpr auto TREASURER = eosio::name("treasurer");
    inline constexpr auto& TREASURY = types::TREASURY;
    inline constexpr auto TREASURER_OF = eosio::name("treasurerof");
    inline constexpr auto& PAYMENT = hypha::common::PAYMENT;
    inline constexpr auto& ATTESTATION = types::ATTESTATION;
    inline constexpr auto ATTESTED = eosio::name("attested");
    inline constexpr auto ATTESTED_BY = eosio::name("attestedby");
    inline constexpr auto PAYS = eosio::name("pays");
    inline constexpr auto PAID_BY = eosio::name("paidby");
    inline constexpr auto& REDEMPTION = types::REDEMPTION;
    inline constexpr auto REDEEM_BALANCE = eosio::name("redeembal");
    inline constexpr auto BALANCE_OWNER = eosio::name("owner");
}

namespace fields {
    inline constexpr auto THRESHOLD = "threshold";
    inline constexpr auto REDEMPTIONS_ENABLED = "redemptions_enabled";
}

namespace payment_state {
    inline constexpr auto NONE = "none";
    inline constexpr auto PENDING = "pending";
    inline constexpr auto EXECUTED = "executed";
    //Just remove the payment document for now
    //inline constexpr auto CANCELED = "canceled";
}

#else //Required fields constants even when treasury is off

namespace links {
    inline constexpr auto TREASURER = eosio::name("treasurer");
}

#endif

}