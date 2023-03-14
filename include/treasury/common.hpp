#pragma once

#include <eosio/name.hpp>
#include <common.hpp>

namespace hypha::treasury::common {

#ifdef USE_TREASURY
    
namespace types {
    constexpr auto TREASURY = eosio::name("treasury");
    constexpr auto TREASURY_PAYMENT = eosio::name("trsy.payment");
    constexpr auto REDEMPTION = eosio::name("redemption");
//     constexpr auto ATTESTATION = eosio::name("attestation");
    constexpr auto BALANCE = eosio::name("balance");
}

namespace links {
    constexpr auto TREASURY_OF = eosio::name("treasuryof");
    constexpr auto TREASURER = eosio::name("treasurer");
    constexpr auto TREASURER_OF = eosio::name("treasurerof");
//     constexpr auto& PAYMENT = hypha::common::PAYMENT;
    // constexpr auto& ATTESTATION = types::ATTESTATION;
    constexpr auto ATTESTED = eosio::name("attested");
    constexpr auto ATTESTED_BY = eosio::name("attestedby");
    constexpr auto PAYS = eosio::name("pays");
    constexpr auto PAID_BY = eosio::name("paidby");
    constexpr auto REDEEM_BALANCE = eosio::name("redeembal");
    constexpr auto BALANCE_OWNER = eosio::name("owner");
}

namespace fields {
    constexpr auto THRESHOLD = "threshold";
    constexpr auto REDEMPTIONS_ENABLED = "redemptions_enabled";
}

#else //Required fields constants even when treasury is off

namespace links {
    constexpr auto TREASURER = eosio::name("treasurer");
}

#endif

}