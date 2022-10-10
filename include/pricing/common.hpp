#pragma once

#include <eosio/name.hpp>

namespace hypha::pricing::common {
    namespace types {
        inline constexpr auto PRICING_PLAN = "pricing.plan"_n;
        inline constexpr auto PRICE_OFFER = "price.offer"_n;
        inline constexpr auto PLAN_MANAGER = "plan.manager"_n;
        inline constexpr auto BILLING_INFO = "billing.info"_n;
    } //namespace types

    namespace links {
        inline constexpr auto PRICING_PLAN = "pricingplan"_n;
        inline constexpr auto DEFAULT_PRICING_PLAN = "defpriceplan"_n;
        inline constexpr auto BILL = "bill"_n;
        inline constexpr auto START_BILL = "startbill"_n;
        inline constexpr auto LAST_BILL = "lastbill"_n;
        inline constexpr auto CURRENT_BILL = "currentbill"_n;
        inline constexpr auto NEXT_BILL = "nextbill"_n;
        inline constexpr auto PLAN_MANAGER = "planmanager"_n;
        inline constexpr auto PRICE_OFFER = "priceoffer"_n;

    } // namespace links
    
    namespace items {
        inline constexpr auto DAO_ID = "dao_id";
        inline constexpr auto PLAN_ID = "plan_id";
        inline constexpr auto OFFER_ID = "offer_id";
        inline constexpr auto START_DATE = "start_date";
        inline constexpr auto PERIODS = "periods";
        inline constexpr auto END_DATE = "end_date";
        inline constexpr auto BILLING_DAY = "billing_day";
    } // namespace items
    
}