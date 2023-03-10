#pragma once

#include <eosio/name.hpp>

namespace hypha::pricing::common {

namespace groups {
    inline constexpr auto ECOSYSTEM = "ecosystem";
}

namespace types {
    inline constexpr auto PRICING_PLAN = eosio::name("pricing.plan");
    inline constexpr auto PRICE_OFFER = eosio::name("price.offer");
    inline constexpr auto PLAN_MANAGER = eosio::name("plan.manager");
    inline constexpr auto BILLING_INFO = eosio::name("billing.info");
    inline constexpr auto PREPAID = "prepaid";
    inline constexpr auto UNLIMITED = "unlimited";
} //namespace types

namespace dao_types {
    inline constexpr auto ANCHOR = "anchor";
    inline constexpr auto INDIVIDUAL = "individual";
    inline constexpr auto ANCHOR_CHILD = "anchor-child";
}

namespace links {
    inline constexpr auto PRICING_PLAN = eosio::name("pricingplan");
    inline constexpr auto DEFAULT_PRICING_PLAN = eosio::name("defpriceplan");
    inline constexpr auto ECOSYSTEM_PRICING_PLAN = eosio::name("ecopriceplan");
    inline constexpr auto BILL = eosio::name("bill");
    inline constexpr auto START_BILL = eosio::name("startbill");
    inline constexpr auto LAST_BILL = eosio::name("lastbill");
    inline constexpr auto CURRENT_BILL = eosio::name("currentbill");
    inline constexpr auto PREV_START_BILL = eosio::name("prevstartbil");
    inline constexpr auto NEXT_BILL = eosio::name("nextbill");
    inline constexpr auto PLAN_MANAGER = eosio::name("planmanager");
    inline constexpr auto PRICE_OFFER = eosio::name("priceoffer");
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
    inline constexpr auto ECOSYSTEM_PRICE = "ecosystem_price";
    inline constexpr auto ECOSYSTEM_PRICE_STAKED = "ecosystem_price_staked";
    inline constexpr auto ECOSYSTEM_CHILD_PRICE = "ecosystem_child_price";
    inline constexpr auto ECOSYSTEM_CHILD_PRICE_STAKED = "ecosystem_child_price_staked";
    inline constexpr auto REACTIVATION_PERIOD = "reactivation_period_sec";
    inline constexpr auto DISCOUNT_PERCENTAGE = "discount_perc_x10000";
    inline constexpr auto MAX_MEMBER_COUNT = "max_member_count";
    inline constexpr auto IS_WAITING_ECOSYSTEM = "is_waiting_ecosystem";
    inline constexpr auto SALE_HYPHA_USD_VALUE = "sale_hypha_usd_value";
} // namespace items
    
}