#pragma once

#include <eosio/name.hpp>

namespace hypha::pricing::common {

    namespace groups {
        inline constexpr auto ECOSYSTEM = "ecosystem";
    }

    namespace types {
        inline constexpr auto PRICING_PLAN = "pricing.plan"_n;
        inline constexpr auto PRICE_OFFER = "price.offer"_n;
        inline constexpr auto PLAN_MANAGER = "plan.manager"_n;
        inline constexpr auto BILLING_INFO = "billing.info"_n;
        inline constexpr auto PREPAID = "prepaid";
        inline constexpr auto UNLIMITED = "unlimited";
    } //namespace types

    namespace dao_types {
        inline constexpr auto ANCHOR = "anchor";
        inline constexpr auto INDIVIDUAL = "individual";
        inline constexpr auto ANCHOR_CHILD = "anchor-child";
    }

    namespace links {
        inline constexpr auto PRICING_PLAN = "pricingplan"_n;
        inline constexpr auto DEFAULT_PRICING_PLAN = "defpriceplan"_n;
        inline constexpr auto ECOSYSTEM_PRICING_PLAN = "ecopriceplan"_n;
        inline constexpr auto BILL = "bill"_n;
        inline constexpr auto START_BILL = "startbill"_n;
        inline constexpr auto LAST_BILL = "lastbill"_n;
        inline constexpr auto CURRENT_BILL = "currentbill"_n;
        inline constexpr auto PREV_START_BILL = "prevstartbil"_n;
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
        inline constexpr auto NAME = "name";
        inline constexpr auto TAG = "tag";
        inline constexpr auto PRICE = "price";
        inline constexpr auto& ECOSYSTEM_PRICE = PRICE;
        inline constexpr auto ECOSYSTEM_PRICE_STAKED = "price_staked";
        inline constexpr auto ECOSYSTEM_CHILD_PRICE = "child_price";
        inline constexpr auto ECOSYSTEM_CHILD_PRICE_STAKED = "child_price_staked";
        inline constexpr auto REACTIVATION_PERIOD = "reactivation_period_sec";
        inline constexpr auto DISCOUNT_PERCENTAGE = "discount_perc_x10000";
        inline constexpr auto MAX_MEMBER_COUNT = "max_member_count";
        inline constexpr auto IS_WAITING_ECOSYSTEM = "is_waiting_ecosystem";
    } // namespace items
    
}