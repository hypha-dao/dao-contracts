#pragma once

#include <eosio/name.hpp>
#include <common.hpp>

namespace hypha::treasury::common {
    
    namespace types {
        inline constexpr auto TREASURY = "treasury"_n;
        inline constexpr auto TREASURY_PAYMENT = "trsy.payment"_n;
        inline constexpr auto REDEMPTION = "redemption"_n;
        inline constexpr auto ATTESTATION = "attestation"_n;
        inline constexpr auto BALANCE = "balance"_n;
    }

    namespace links {
        inline constexpr auto TREASURY_OF = "treasuryof"_n;
        inline constexpr auto TREASURER = "treasurer"_n;
        inline constexpr auto& TREASURY = types::TREASURY;
        inline constexpr auto TREASURER_OF = "treasurerof"_n;
        inline constexpr auto& PAYMENT = hypha::common::PAYMENT;
        inline constexpr auto& ATTESTATION = types::ATTESTATION;
        inline constexpr auto ATTESTED = "attested"_n;
        inline constexpr auto ATTESTED_BY = "attestedby"_n;
        inline constexpr auto PAYS = "pays"_n;
        inline constexpr auto PAID_BY = "paidby"_n;
        inline constexpr auto& REDEMPTION = types::REDEMPTION;
        inline constexpr auto REDEEM_BALANCE = "redeembal"_n;
        inline constexpr auto BALANCE_OWNER = "owner"_n;
    }

    namespace fields {
        inline constexpr auto THRESHOLD = "threshold";
        inline constexpr auto REDEMPTIONS_ENABLED = "redemptions_enabled";
    }
}