#pragma once

#include <eosio/symbol.hpp>
#include <eosio/asset.hpp>
#include <eosio/name.hpp>

using eosio::name;
using eosio::asset;
using eosio::symbol;

//TODO: Refacto into a Lazy load system to avoid allocating many std::strings that are not actually used
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
    #define ROOT_NODE "root_node"
    #define HYPHA_DEFERRAL_FACTOR "hypha_deferral_factor_x100"
    #define CLIENT_VERSION "client_version"
    #define CONTRACT_VERSION "contract_version"
    #define TITLE "title"
    #define DESCRIPTION "description"
    #define CONTENT "content"
    #define ICON "icon"
    
    #define START_PERIOD                = std::string ("start_period");
    #define END_PERIOD                  = std::string ("end_period");

    #define START_TIME                  = std::string ("start_time");
    #define END_TIME                    = std::string ("end_time");
    #define LABEL                       = std::string ("label");

    #define HYPHA_COEFFICIENT           = std::string ("hypha_coefficient_x10000");
    #define HVOICE_COEFFICIENT          = std::string ("hvoice_coefficient_x10000");
    #define SEEDS_COEFFICIENT           = std::string ("seeds_coefficient_x10000");
    #define HUSD_COEFFICIENT            = std::string ("husd_coefficient_x10000");

    #define BADGE_STRING                = std::string ("badge");
    #define DETAILS                     = std::string ("details");
    #define TYPE                        = std::string ("type");
    #define ASSIGNEE                    = std::string ("assignee");
    #define MEMBER_STRING               = std::string ("member");
    #define ASSIGNMENT_STRING           = std::string ("assignment");
    #define SYSTEM                      = std::string ("system");
    #define BALLOT_ID                   = std::string ("ballot_id");
    #define BALLOT_TYPE                 = std::string ("ballot_type");
    
    //setting document
    //#define CLIENT_VERSION              = std::string ("client_version");
    //#define CONTRACT_VERSION            = std::string ("contract_version");
    #define TELOS_DECIDE_CONTRACT       = std::string ("telos_decide_contract");
    #define SEEDS_TOKEN_CONTRACT        = std::string ("seeds_token_contract");
    #define SEEDS_ESCROW_CONTRACT       = std::string ("seeds_escrow_contract");
    #define HUSD_TOKEN_CONTRACT         = std::string ("husd_token_contract");
    #define TREASURY_CONTRACT           = std::string ("treasury_contract");
    #define HYPHA_TOKEN_CONTRACT        = std::string ("hypha_token_contract");
    #define UPDATED_DATE                = std::string ("updated_date");
    //#define ROOT_NODE                   = std::string ("root_node");
    #define SEEDS_DEFERRAL_FACTOR_X100  = std::string ("seeds_deferral_factor_x100");
    #define HYPHA_DEFERRAL_FACTOR_X100  = std::string ("hypha_deferral_factor_x100");
    #define LAST_BALLOT_ID              = std::string ("last_ballot_id");
    #define LAST_SENDER_ID              = std::string ("last_sender_id");
    #define VOTING_DURATION_SEC         = std::string ("voting_duration_sec");
    #define PUBLISHER_CONTRACT          = std::string ("publisher_contract");
    #define PAUSED                      = std::string ("paused");
    #define ROLE_STRING                = std::string ("role");

    // payment related
    #define PAYMENT_PERIOD              = std::string ("payment_period");
    #define ASSETS_PAID                 = std::string ("assets_paid");
    #define HYPHA_AMOUNT                = std::string ("hypha_amount");
    #define ESCROW_SEEDS_AMOUNT         = std::string ("escrow_seeds_amount");
    #define HVOICE_AMOUNT               = std::string ("hvoice_amount");
    #define HUSD_AMOUNT                 = std::string ("husd_amount");
    #define AMOUNT                      = std::string ("amount");
    #define REFERENCE                   = std::string ("reference");
    #define RECIPIENT                   = std::string ("recipient");
    #define MEMO                        = std::string ("memo");
    #define PAYMENT_TYPE                = std::string ("payment_type");

    static constexpr name       ESCROW                      = name ("escrow");
    #define EVENT                       = std::string ("event");

    #define FULL_TIME_CAPACITY          = std::string ("fulltime_capacity_x100");
    #define ANNUAL_USD_SALARY           = std::string ("annual_usd_salary");
    #define TIME_SHARE                  = std::string ("time_share_x100");
    #define MIN_TIME_SHARE              = std::string ("min_time_share_x100");
    #define MIN_DEFERRED                = std::string ("min_deferred_x100");
    #define DEFERRED                    = std::string ("deferred_perc_x100");

    #define USD_SALARY_PER_PERIOD       = std::string ("usd_salary_value_per_phase");
    #define HYPHA_SALARY_PER_PERIOD     = std::string ("hypha_salary_per_phase");
    #define HUSD_SALARY_PER_PERIOD      = std::string ("husd_salary_per_phase");
    #define HVOICE_SALARY_PER_PERIOD    = std::string ("hvoice_salary_per_phase");
    #define SETTINGS                    = std::string ("settings");

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