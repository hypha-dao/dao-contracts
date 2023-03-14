#pragma once

#include <eosio/name.hpp>

namespace hypha::pricing::common {

namespace groups {
    constexpr auto ECOSYSTEM = "ecosystem";
}

namespace types {
    constexpr auto PRICING_PLAN = eosio::name("pricing.plan");
    constexpr auto PRICE_OFFER = eosio::name("price.offer");
    constexpr auto PLAN_MANAGER = eosio::name("plan.manager");
    constexpr auto BILLING_INFO = eosio::name("billing.info");
    constexpr auto PREPAID = "prepaid";
    constexpr auto UNLIMITED = "unlimited";
} //namespace types

namespace dao_types {
    constexpr auto ANCHOR = "anchor";
    constexpr auto INDIVIDUAL = "individual";
    constexpr auto ANCHOR_CHILD = "anchor-child";
}

namespace links {
    constexpr auto PRICING_PLAN = eosio::name("pricingplan");
    constexpr auto DEFAULT_PRICING_PLAN = eosio::name("defpriceplan");
    constexpr auto ECOSYSTEM_PRICING_PLAN = eosio::name("ecopriceplan");
    constexpr auto BILL = eosio::name("bill");
    constexpr auto START_BILL = eosio::name("startbill");
    constexpr auto LAST_BILL = eosio::name("lastbill");
    constexpr auto CURRENT_BILL = eosio::name("currentbill");
    constexpr auto PREV_START_BILL = eosio::name("prevstartbil");
    constexpr auto NEXT_BILL = eosio::name("nextbill");
    constexpr auto PLAN_MANAGER = eosio::name("planmanager");
    constexpr auto PRICE_OFFER = eosio::name("priceoffer");
} // namespace links

namespace items {
    constexpr auto DAO_ID = "dao_id";
    constexpr auto PLAN_ID = "plan_id";
    constexpr auto OFFER_ID = "offer_id";
    constexpr auto START_DATE = "start_date";
    constexpr auto PERIODS = "periods";
    constexpr auto END_DATE = "end_date";
    constexpr auto BILLING_DAY = "billing_day";
    constexpr auto NAME = "name";
    constexpr auto TAG = "tag";
    constexpr auto PRICE = "price";
    constexpr auto ECOSYSTEM_PRICE = "ecosystem_price";
    constexpr auto ECOSYSTEM_PRICE_STAKED = "ecosystem_price_staked";
    constexpr auto ECOSYSTEM_CHILD_PRICE = "ecosystem_child_price";
    constexpr auto ECOSYSTEM_CHILD_PRICE_STAKED = "ecosystem_child_price_staked";
    constexpr auto REACTIVATION_PERIOD = "reactivation_period_sec";
    constexpr auto DISCOUNT_PERCENTAGE = "discount_perc_x10000";
    constexpr auto MAX_MEMBER_COUNT = "max_member_count";
    constexpr auto IS_WAITING_ECOSYSTEM = "is_waiting_ecosystem";
    constexpr auto SALE_HYPHA_USD_VALUE = "sale_hypha_usd_value";
} // namespace items
    
}