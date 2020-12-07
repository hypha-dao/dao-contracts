#pragma once

#include <eosio/symbol.hpp>
#include <eosio/asset.hpp>
#include <eosio/name.hpp>

using eosio::name;
using eosio::asset;
using eosio::symbol;
using std::string;

//TODO: Refacto into a Lazy load system to avoid allocating many strings that are not actually used
namespace common {

    static constexpr symbol         S_HYPHA                         ("HYPHA", 2);
    static constexpr symbol         S_HVOICE                        ("HVOICE", 2);
    static constexpr symbol         S_SEEDS                         ("SEEDS", 4);
    static constexpr symbol         S_HUSD                          ("HUSD", 2);

    static const asset          RAM_ALLOWANCE                   = asset (20000, symbol("TLOS", 4));

    // 365.25 / 7.4
    static const float          PHASES_PER_YEAR                 = 49.3581081081;

    // 49.36 phases per annum, so each phase is 2.026% of the total
    static const float          PHASE_TO_YEAR_RATIO             = 0.02026009582;

    // content keys
    // keys used with settings
    static const string         ROOT_NODE                   = string ("root_node");
    static const string         HYPHA_DEFERRAL_FACTOR       = string ("hypha_deferral_factor_x100");
    static const string         CLIENT_VERSION              = string ("client_version");
    static const string         CONTRACT_VERSION            = string ("contract_version");

    static const string         TITLE                       = string ("title");
    static const string         DESCRIPTION                 = string ("description");
    static const string         CONTENT                     = string ("content");
    static const string         ICON                        = string ("icon");
    // static const string         PROPOSER                    = string ("proposer");

    static const string         START_PERIOD                = string ("start_period");
    static const string         END_PERIOD                  = string ("end_period");

    static const string         START_TIME                  = string ("start_time");
    static const string         END_TIME                    = string ("end_time");
    static const string         LABEL                       = string ("label");

    static const string         HYPHA_COEFFICIENT           = string ("hypha_coefficient_x10000");
    static const string         HVOICE_COEFFICIENT          = string ("hvoice_coefficient_x10000");
    static const string         SEEDS_COEFFICIENT           = string ("seeds_coefficient_x10000");
    static const string         HUSD_COEFFICIENT            = string ("husd_coefficient_x10000");

    static const string         BADGE_STRING                = string ("badge");
    static const string         DETAILS                     = string ("details");
    static const string         TYPE                        = string ("type");
    static const string         ASSIGNEE                    = string ("assignee");
    static const string         MEMBER_STRING               = string ("member");
    static const string         ASSIGNMENT_STRING           = string ("assignment");
    static const string         SYSTEM                      = string ("system");
    static const string         BALLOT_ID                   = string ("ballot_id");
    static const string         BALLOT_TYPE                 = string ("ballot_type");
    
    //setting document
    //static const string         CLIENT_VERSION              = string ("client_version");
    //static const string         CONTRACT_VERSION            = string ("contract_version");
    static const string         TELOS_DECIDE_CONTRACT       = string ("telos_decide_contract");
    static const string         SEEDS_TOKEN_CONTRACT        = string ("seeds_token_contract");
    static const string         SEEDS_ESCROW_CONTRACT       = string ("seeds_escrow_contract");
    static const string         HUSD_TOKEN_CONTRACT         = string ("husd_token_contract");
    static const string         TREASURY_CONTRACT           = string ("treasury_contract");
    static const string         HYPHA_TOKEN_CONTRACT        = string ("hypha_token_contract");
    static const string         UPDATED_DATE                = string ("updated_date");
    //static const string         ROOT_NODE                   = string ("root_node");
    static const string         SEEDS_DEFERRAL_FACTOR_X100  = string ("seeds_deferral_factor_x100");
    static const string         HYPHA_DEFERRAL_FACTOR_X100  = string ("hypha_deferral_factor_x100");
    static const string         LAST_BALLOT_ID              = string ("last_ballot_id");
    static const string         LAST_SENDER_ID              = string ("last_sender_id");
    static const string         VOTING_DURATION_SEC         = string ("voting_duration_sec");
    static const string         PUBLISHER_CONTRACT          = string ("publisher_contract");
    static const string         PAUSED                      = string ("paused");
    static const string         ROLE_STRING                 = string ("role");

    // payment related
    static const string         PAYMENT_PERIOD              = string ("payment_period");
    static const string         ASSETS_PAID                 = string ("assets_paid");
    static const string         HYPHA_AMOUNT                = string ("hypha_amount");
    static const string         ESCROW_SEEDS_AMOUNT         = string ("escrow_seeds_amount");
    static const string         HVOICE_AMOUNT               = string ("hvoice_amount");
    static const string         HUSD_AMOUNT                 = string ("husd_amount");
    static const string         AMOUNT                      = string ("amount");
    static const string         REFERENCE                   = string ("reference");
    static const string         RECIPIENT                   = string ("recipient");
    static const string         MEMO                        = string ("memo");
    static const string         PAYMENT_TYPE                = string ("payment_type");

    static constexpr name       ESCROW                      = name ("escrow");
    static const string         EVENT                       = string ("event");

    static const string         FULL_TIME_CAPACITY          = string ("fulltime_capacity_x100");
    static const string         ANNUAL_USD_SALARY           = string ("annual_usd_salary");
    static const string         TIME_SHARE                  = string ("time_share_x100");
    static const string         MIN_TIME_SHARE              = string ("min_time_share_x100");
    static const string         MIN_DEFERRED                = string ("min_deferred_x100");
    static const string         DEFERRED                    = string ("deferred_perc_x100");

    static const string         USD_SALARY_PER_PERIOD       = string ("usd_salary_value_per_phase");
    static const string         HYPHA_SALARY_PER_PERIOD     = string ("hypha_salary_per_phase");
    static const string         HUSD_SALARY_PER_PERIOD      = string ("husd_salary_per_phase");
    static const string         HVOICE_SALARY_PER_PERIOD    = string ("hvoice_salary_per_phase");
    static const string         SETTINGS                    = string ("settings");

    // graph edge names
    static constexpr name       SETTINGS_EDGE               = name ("settings");
    static constexpr name       HOLDS_BADGE                 = name ("holdsbadge");
    static constexpr name       HELD_BY                     = name ("heldby");
    static constexpr name       OWNED_BY                    = name ("ownedby");
    static constexpr name       OWNS                        = name ("owns");
    static constexpr name       MEMBER_OF                   = name ("memberof");
    static constexpr name       APPLICANT                   = name ("applicant");
    static constexpr name       APPLICANT_OF                = name ("applicantof");

    static constexpr name       ASSIGN_BADGE                = name ("assignbadge");
    static constexpr name       VOTE_TALLY                  = name ("votetally");
    static constexpr name       ASSIGNMENT                  = name ("assignment");
    static constexpr name       ROLE_NAME                   = name ("role");

    static constexpr name       ASSIGNED                    = name ("assigned");
    static constexpr name       ASSIGNEE_NAME               = name ("assignee");
    static constexpr name       PERIOD                      = name ("period");
    static constexpr name       PAYMENT                     = name ("payment");

    
    // graph edges hanging off of primary DHO node
    static constexpr name       BADGE_NAME                  = name ("badge");
    static constexpr name       PROPOSAL                    = name ("proposal");
    static constexpr name       FAILED_PROPS                = name ("failedprops");
    static constexpr name       PASSED_PROPS                = name ("passedprops");
    static constexpr name       MEMBER                      = name ("member");

    static constexpr name       BALLOT_TYPE_OPTIONS         = name ("options");
    static constexpr name       BALLOT_TYPE_TELOS_DECIDE    = name ("telosdecide");

    static constexpr name       GROUP_TYPE_OPTION           = name ("option");


    static const uint64_t       NO_ASSIGNMENT                   = -1;         
};