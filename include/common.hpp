#pragma once

#include <eosio/symbol.hpp>
#include <eosio/asset.hpp>
#include <eosio/name.hpp>

using eosio::asset;
using eosio::name;
using eosio::symbol;

//TODO: Refacto into a Lazy load system to avoid allocating many std::strings that are not actually used
namespace common
{

    constexpr auto PEG_TOKEN = "peg_token";
    constexpr auto VOICE_TOKEN = "voice_token";
    constexpr auto REWARD_TOKEN = "reward_token";
    constexpr auto REWARD_TO_PEG_RATIO = "reward_to_peg_ratio";

    constexpr symbol S_REWARD("BM", 2);
    constexpr symbol S_VOICE("BMV", 2);
    constexpr symbol S_PEG("BUSD", 2);

    constexpr symbol S_SEEDS("SEEDS", 4);
    constexpr symbol S_USD("USD", 2);

    //Dao settings
    constexpr auto DAO_DESCRIPTION = "dao_description";
    constexpr auto DAO_TITLE = "dao_title";
    constexpr auto PERIOD_DURATION = "period_duration_sec";
    constexpr auto ONBOARDER_ACCOUNT = "onboarder_account";

    const asset RAM_ALLOWANCE = asset(20000, symbol("TLOS", 4));

    // 365.25 / 7.4
    const float PHASES_PER_YEAR = 49.3581081081;

    // 49.36 phases per annum, so each phase is 2.026% of the total
    const float PHASE_TO_YEAR_RATIO = 0.02026009582;

    // graph edge names
    constexpr name ESCROW = name("escrow");
    constexpr name SETTINGS_EDGE = name("settings");
    constexpr name HOLDS_BADGE = name("holdsbadge");
    constexpr name HELD_BY = name("heldby");
    constexpr name OWNED_BY = name("ownedby");
    constexpr name OWNS = name("owns");
    constexpr name MEMBER_OF = name("memberof");
    constexpr name APPLICANT = name("applicant");
    constexpr name APPLICANT_OF = name("applicantof");

    constexpr name ASSIGN_BADGE = name("assignbadge");
    constexpr name VOTE_TALLY = name("votetally");
    constexpr name ASSIGNMENT = name("assignment");
    constexpr name ROLE_NAME = name("role");
    constexpr name PAYOUT = name("payout");
    constexpr name ATTESTATION = name("attestation");
    constexpr name EDIT = name("edit");
    constexpr name ORIGINAL = name("original");
    constexpr name EXTENSION = name("extension");
    constexpr        name SUSPEND = name("suspend");
    constexpr        name SUSPENDED = name("suspended");

    constexpr name ASSIGNED = name("assigned");
    constexpr name ASSIGNEE_NAME = name("assignee");
    constexpr        auto APPROVED_DATE = "original_approved_date";
    constexpr        auto BALLOT_TITLE = "ballot_title";
    constexpr        auto BALLOT_DESCRIPTION = "ballot_description";
    constexpr        auto STATE = "state";
    constexpr        auto STATE_PROPOSED = "proposed";
    constexpr        auto STATE_APPROVED = "approved";
    constexpr        auto STATE_REJECTED = "rejected";
    constexpr        auto STATE_SUSPENDED = "suspended";
    constexpr        auto STATE_EXPIRED = "expired";
    constexpr        auto STATE_WITHDRAWED = "withdrawed";
    constexpr        auto HYPHA_USD_VALUE = "hypha_usd_value";
    constexpr name PERIOD = name("period");
    constexpr name START = name ("start");
    constexpr name END = name ("end");
    constexpr name NEXT = name ("next");
    constexpr name PAYMENT = name("payment");
    constexpr name PAID = name ("paid");
    constexpr name CLAIMED = name ("claimed");

    constexpr name VOTE = name ("vote");
    constexpr name VOTE_ON = name ("voteon");

    // document types
    constexpr name ALERT = name ("alert");

    // graph edges hanging off of primary DHO node
    constexpr name BADGE_NAME = name("badge");
    constexpr name PROPOSAL = name("proposal");
    constexpr name FAILED_PROPS = name("failedprops");
    constexpr name PASSED_PROPS = name("passedprops");
    constexpr name MEMBER = name("member");
    constexpr name DHO = name ("dho");
    constexpr name DAO = name ("dao");

    constexpr name BALLOT_TYPE_OPTIONS = name("options");

    constexpr name GROUP_TYPE_OPTION = name("option");

    constexpr name LAST_TIME_SHARE = name("lastimeshare");
    constexpr name CURRENT_TIME_SHARE = name("curtimeshare");
    constexpr name INIT_TIME_SHARE = name("initimeshare");
    constexpr name NEXT_TIME_SHARE = name("nextimeshare");
    constexpr name TIME_SHARE_LABEL = name("timeshare");
    constexpr auto ADJUST_MODIFIER = "modifier";
    constexpr auto MOD_WITHDRAW = "withdraw";

    constexpr auto APPROVED_DEFERRED = "approved_deferred_perc_x100";

    constexpr name BALLOT_DEFAULT_OPTION_PASS = name("pass");
    constexpr name BALLOT_DEFAULT_OPTION_ABSTAIN = name("abstain");
    constexpr name BALLOT_DEFAULT_OPTION_FAIL = name("fail");

// content keys
// keys used with settings
#define ROOT_NODE "root_node"
#define DAO_NAME "dao_name"

#define HYPHA_DEFERRAL_FACTOR "hypha_deferral_factor_x100"
#define CLIENT_VERSION "client_version"
#define CONTRACT_VERSION "contract_version"
#define DEFAULT_VERSION "un-versioned"
#define TITLE "title"
#define DESCRIPTION "description"
#define CONTENT "content"
#define ICON "icon"
#define LEVEL "level"

#define START_PERIOD "start_period"
#define END_PERIOD "end_period"
#define PERIOD_COUNT "period_count"
#define START_TIME "start_time"
#define END_TIME "end_time"
#define LABEL "label"
#define NODE_LABEL "node_label"
#define READABLE_START_TIME "readable_start_time"
#define READABLE_START_DATE "readable_start_date"

#define ORIGINAL_DOCUMENT "original_document"
#define HYPHA_COEFFICIENT "hypha_coefficient_x10000"
#define HVOICE_COEFFICIENT "hvoice_coefficient_x10000"
#define SEEDS_COEFFICIENT "seeds_coefficient_x10000"
#define HUSD_COEFFICIENT "husd_coefficient_x10000"

#define BADGE_STRING "badge"
#define DETAILS "details"
#define TYPE "type"
#define ASSIGNEE "assignee"
#define MEMBER_STRING "member"
#define ASSIGNMENT_STRING "assignment"
#define SYSTEM "system"
#define BALLOT "ballot"
#define BALLOT_OPTIONS "ballot_options"
#define BALLOT_TYPE "ballot_type"

// setting document
#define SEEDS_TOKEN_CONTRACT "seeds_token_contract"
#define SEEDS_ESCROW_CONTRACT "seeds_escrow_contract"
#define PEG_TOKEN_CONTRACT "peg_token_contract"
#define TREASURY_CONTRACT "treasury_contract"
#define REWARD_TOKEN_CONTRACT "reward_token_contract"
#define GOVERNANCE_TOKEN_CONTRACT "governance_token_contract"

#define UPDATED_DATE "updated_date"
#define SEEDS_DEFERRAL_FACTOR_X100 "seeds_deferral_factor_x100"
#define HYPHA_DEFERRAL_FACTOR_X100 "hypha_deferral_factor_x100"
#define LAST_SENDER_ID "last_sender_id"
#define VOTING_DURATION_SEC "voting_duration_sec"
#define VOTING_QUORUM_FACTOR_X100 "voting_quorum_x100"
#define VOTING_ALIGNMENT_FACTOR_X100 "voting_alignment_x100"
#define PUBLISHER_CONTRACT "publisher_contract"
#define PAUSED "paused"
#define ROLE_STRING "role"

// payment related
#define PAYMENT_PERIOD "payment_period"
#define PERIOD_COUNT "period_count"
#define ASSETS_PAID "assets_paid"
#define HYPHA_AMOUNT "hypha_amount"
#define ESCROW_SEEDS_AMOUNT "escrow_seeds_amount"
#define HVOICE_AMOUNT "hvoice_amount"
#define HUSD_AMOUNT "husd_amount"
#define AMOUNT "amount"
#define REFERENCE "reference"
#define RECIPIENT "recipient"
#define MEMO "memo"
#define PAYMENT_TYPE "payment_type"

#define EVENT "event"

#define TIME_SHARE_START_DATE "start_date"
#define FULL_TIME_CAPACITY "fulltime_capacity_x100"
#define ANNUAL_USD_SALARY "annual_usd_salary"
#define TIME_SHARE "time_share_x100"
#define NEW_TIME_SHARE "new_time_share_x100"
#define MIN_TIME_SHARE "min_time_share_x100"
#define MIN_DEFERRED "min_deferred_x100"
#define DEFERRED "deferred_perc_x100"

#define USD_AMOUNT "usd_amount"
#define USD_SALARY_PER_PERIOD "usd_salary_value_per_phase"
#define HYPHA_SALARY_PER_PERIOD "hypha_salary_per_phase"
#define HUSD_SALARY_PER_PERIOD "husd_salary_per_phase"
#define HVOICE_SALARY_PER_PERIOD "hvoice_salary_per_phase"
#define SETTINGS "settings"

#define VOTE_POWER "vote_power"
#define VOTE_LABEL "vote"
#define VOTER_LABEL "voter"
#define EXPIRATION_LABEL "expiration"

}; // namespace common