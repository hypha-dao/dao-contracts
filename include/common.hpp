#pragma once

#include <eosio/symbol.hpp>
#include <eosio/asset.hpp>
#include <eosio/name.hpp>

using eosio::asset;
using eosio::name;
using eosio::symbol;

//TODO: Refacto into a Lazy load system to avoid allocating many std::strings that are not actually used
namespace hypha::common
{
    inline constexpr auto PEG_TOKEN = "peg_token";
    inline constexpr auto VOICE_TOKEN = "voice_token";
    inline constexpr auto REWARD_TOKEN = "reward_token";
    inline constexpr auto REWARD_TO_PEG_RATIO = "reward_to_peg_ratio";

    inline constexpr symbol S_REWARD("BM", 2);
    inline constexpr symbol S_VOICE("BMV", 2);
    inline constexpr symbol S_PEG("BUSD", 2);

    inline constexpr symbol S_SEEDS("SEEDS", 4);
    inline constexpr symbol S_USD("USD", 2);
    inline constexpr symbol S_HUSD("HUSD", 2);
    inline constexpr symbol S_HYPHA("HYPHA", 2);

    //Dao settings
    inline constexpr auto DAO_DESCRIPTION = "dao_description";
    inline constexpr auto DAO_TITLE = "dao_title";
    inline constexpr auto DAO_URL = "dao_url";

    inline constexpr auto DAO_DOCUMENTATION_URL = "documentation_url";
    inline constexpr auto DAO_DISCORD_URL = "discord_url";
    inline constexpr auto DAO_LOGO = "logo";
    inline constexpr auto DAO_EXTENDED_LOGO = "extended_logo";
    inline constexpr auto DAO_PATTERN = "pattern";
    inline constexpr auto DAO_PRIMARY_COLOR = "primary_color";
    inline constexpr auto DAO_SECONDARY_COLOR = "secondary_color";
    inline constexpr auto DAO_TEXT_COLOR = "text_color";
    inline constexpr auto DAO_PATTERN_COLOR = "pattern_color";
    inline constexpr auto DAO_PATTERN_OPACITY = "pattern_opacity";
    inline constexpr auto DAO_SPLASH_BACKGROUND_IMAGE = "splash_background_image";
    
    inline constexpr auto DAO_DASHBOARD_BACKGROUND_IMAGE = "dashboard_background_image";
    inline constexpr auto DAO_DASHBOARD_TITLE = "dashboard_title";
    inline constexpr auto DAO_DASHBOARD_PARAGRAPH = "dashboard_paragraph";
    inline constexpr auto DAO_PROPOSALS_BACKGROUND_IMAGE = "proposals_background_image";
    inline constexpr auto DAO_PROPOSALS_TITLE = "proposals_title";
    inline constexpr auto DAO_PROPOSALS_PARAGRAPH = "proposals_paragraph";
    inline constexpr auto DAO_MEMBERS_BACKGROUND_IMAGE = "members_background_image";
    inline constexpr auto DAO_MEMBERS_TITLE = "members_title";
    inline constexpr auto DAO_MEMBERS_PARAGRAPH = "members_paragraph";
    inline constexpr auto DAO_ORGANISATION_BACKGROUND_IMAGE = "organisation_background_image";
    inline constexpr auto DAO_ORGANISATION_TITLE = "organisation_title";
    inline constexpr auto DAO_ORGANISATION_PARAGRAPH = "organisation_paragraph";

    inline constexpr auto PERIOD_DURATION = "period_duration_sec";
    inline constexpr auto ONBOARDER_ACCOUNT = "onboarder_account";
    inline constexpr auto DAO_USES_SEEDS = "uses_seeds";
    inline constexpr auto VOICE_TOKEN_DECAY_PERIOD = "voice_token_decay_period";
    inline constexpr auto VOICE_TOKEN_DECAY_PER_PERIOD = "voice_token_decay_per_period_x10M";
    inline constexpr auto SKIP_PEG_TOKEN_CREATION = "skip_peg_token_create";
    inline constexpr auto SKIP_REWARD_TOKEN_CREATION = "skip_reward_token_create";
    //const asset RAM_ALLOWANCE = asset(20000, symbol("TLOS", 4));
    inline constexpr auto URLS_GROUP = "urls";
    inline constexpr auto URL = "url";
    inline constexpr auto CLAIM_ENABLED = "claim_enabled";
    // 365.25 / 7.4
    //const float PHASES_PER_YEAR = 49.3581081081;

    //365.25*24*60*60
    inline constexpr int64_t YEAR_DURATION_SEC = 31557600;

    inline constexpr size_t MAX_PROPOSAL_TITLE_CHARS = 50;
    inline constexpr size_t MAX_PROPOSAL_DESC_CHARS = 4000;

    // 49.36 phases per annum, so each phase is 2.026% of the total
    //const float PHASE_TO_YEAR_RATIO = 0.02026009582;

    // graph edge names
    inline constexpr name ESCROW = name("escrow");
    inline constexpr name SETTINGS_EDGE = name("settings");
    inline constexpr name HOLDS_BADGE = name("holdsbadge");
    inline constexpr name HELD_BY = name("heldby");
    inline constexpr name OWNED_BY = name("ownedby");
    inline constexpr name OWNS = name("owns");
    inline constexpr name MEMBER_OF = name("memberof");
    inline constexpr name APPLICANT = name("applicant");
    inline constexpr name APPLICANT_OF = name("applicantof");
    inline constexpr name ENROLLER = name("enroller");
    inline constexpr name ADMIN = name("admin");
    inline constexpr name OWNER = name("owner");

    inline constexpr name DHO_ROOT_NAME = name("dhoroot");
    inline constexpr name ASSIGN_BADGE = name("assignbadge");
    inline constexpr name VOTE_TALLY = name("votetally");
    inline constexpr name ASSIGNMENT = name("assignment");
    inline constexpr name ROLE_NAME = name("role");
    inline constexpr name PAYOUT = name("payout");
    inline constexpr name ATTESTATION = name("attestation");
    inline constexpr name EDIT = name("edit");
    inline constexpr name ORIGINAL = name("original");
    inline constexpr name EXTENSION = name("extension");
    inline constexpr        name SUSPEND = name("suspend");
    inline constexpr        name SUSPENDED = name("suspended");
    //Must only be used by Quests
    inline constexpr        name ENTRUSTED_TO = name("entrustedto");
    inline constexpr        name ENTRUSTED = name("entrusted");
    inline constexpr name QUEST_START = name("queststart");
    inline constexpr name QUEST_COMPLETION = name("questcomplet");

    //Must only be used by Assignments
    inline constexpr name ASSIGNED = name("assigned");
    inline constexpr name ASSIGNEE_NAME = name("assignee");
    inline constexpr        auto APPROVED_DATE = "original_approved_date";
    inline constexpr        auto BALLOT_TITLE = "ballot_title";
    inline constexpr        auto BALLOT_DESCRIPTION = "ballot_description";
    inline constexpr        auto BALLOT_QUORUM = "ballot_quorum";
    inline constexpr        auto BALLOT_SUPPLY = "ballot_supply";
    inline constexpr        auto BALLOT_ALIGNMENT = "ballot_alignment";
    inline constexpr        auto STATE = "state";
    inline constexpr        auto STATE_PROPOSED = "proposed";
    inline constexpr        auto STATE_ARCHIVED = "archived";
    inline constexpr        auto STATE_DRAFTED = "drafted";
    inline constexpr        auto STATE_APPROVED = "approved";
    inline constexpr        auto STATE_REJECTED = "rejected";
    inline constexpr        auto STATE_SUSPENDED = "suspended";
    inline constexpr        auto STATE_WITHDRAWED = "withdrawed";

    inline constexpr        auto HYPHA_USD_VALUE = "hypha_usd_value";
    inline constexpr name PERIOD = name("period");
    inline constexpr name START = name ("start");
    inline constexpr name CURRENT = name ("current");
    inline constexpr name END = name ("end");
    inline constexpr name NEXT = name ("next");
    inline constexpr name PAYMENT = name("payment");
    inline constexpr name PAID = name ("paid");
    inline constexpr name CLAIMED = name ("claimed");

    inline constexpr name COMPLETED_BY = name("completedby");

    inline constexpr auto PEG_AMOUNT = "peg_amount";
    inline constexpr auto VOICE_AMOUNT = "voice_amount";
    inline constexpr auto REWARD_AMOUNT = "reward_amount";

    inline constexpr name VOTE = name ("vote");
    inline constexpr name VOTE_ON = name ("voteon");

    inline constexpr name COMMENT = name("comment");
    inline constexpr name COMMENT_OF = name("commentof");
    inline constexpr name COMMENT_SECTION = name("cmntsect");
    inline constexpr name COMMENT_SECTION_OF = name("cmntsectof");

    inline constexpr name REACTED_TO = name("reactedto");
    inline constexpr name REACTED_BY = name("reactedby");
    inline constexpr name REACTION = name("reaction");
    inline constexpr name REACTION_OF = name("reactionof");
    inline constexpr name REACTION_LINK = name("reactionlnk");
    inline constexpr name REACTION_LINK_REVERSE = name("reactionlnkr");


    // document types
    inline constexpr name ALERT = name ("alert");
    inline constexpr name SALARY_BAND = name ("salaryband");
    inline constexpr name NOTIFIES = name ("notifies");

    // graph edges hanging off of primary DHO node
    inline constexpr name BADGE_NAME = name("badge");
    inline constexpr name PROPOSAL = name("proposal");
    inline constexpr name VOTABLE = name("votable");
    inline constexpr name FAILED_PROPS = name("failedprops");
    inline constexpr name PASSED_PROPS = name("passedprops");
    inline constexpr name MEMBER = name("member");
    inline constexpr name DHO = name ("dho");
    inline constexpr name DAO = name ("dao");

    inline constexpr name BALLOT_TYPE_OPTIONS = name("options");

    inline constexpr name GROUP_TYPE_OPTION = name("option");

    //SALARY BANDS
    inline constexpr auto SALARY_BAND_NAME = "name";

    //ALERTS
    inline constexpr auto ADD_GROUP = "add";
    inline constexpr auto EDIT_GROUP = "edit";
    inline constexpr auto DELETE_GROUP = "del";

    namespace AlertInfo {
      enum  {
            kDESCRIPTION,
            kLEVEL,
            kENABLED,
            kID
      };
    }

    inline constexpr name LAST_TIME_SHARE = name("lastimeshare");
    inline constexpr name CURRENT_TIME_SHARE = name("curtimeshare");
    inline constexpr name INIT_TIME_SHARE = name("initimeshare");
    inline constexpr name NEXT_TIME_SHARE = name("nextimeshare");
    inline constexpr name TIME_SHARE_LABEL = name("timeshare");
    inline constexpr auto ADJUST_MODIFIER = "modifier";
    inline constexpr auto MOD_WITHDRAW = "withdraw";

    inline constexpr auto APPROVED_DEFERRED = "approved_deferred_perc_x100";

    inline constexpr name BALLOT_DEFAULT_OPTION_PASS = name("pass");
    inline constexpr name BALLOT_DEFAULT_OPTION_ABSTAIN = name("abstain");
    inline constexpr name BALLOT_DEFAULT_OPTION_FAIL = name("fail");

    inline constexpr auto PEG_SALARY_PER_PERIOD = "peg_salary_per_period";
    inline constexpr auto VOICE_SALARY_PER_PERIOD = "voice_salary_per_period";
    inline constexpr auto REWARD_SALARY_PER_PERIOD = "reward_salary_per_period";

    inline constexpr auto REWARD_COEFFICIENT = "reward_coefficient_x10000";
    inline constexpr auto VOICE_COEFFICIENT = "voice_coefficient_x10000";
    inline constexpr auto PEG_COEFFICIENT = "peg_coefficient_x10000";

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
#define ENABLED "enabled"

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
#define HYPHA_TOKEN_CONTRACT "hypha_token_contract"
#define HYPHA_COSALE_CONTRACT "hypha_cosale_contract"
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
// #define HYPHA_SALARY_PER_PERIOD "hypha_salary_per_phase"
// #define HUSD_SALARY_PER_PERIOD "husd_salary_per_phase"
// #define HVOICE_SALARY_PER_PERIOD "hvoice_salary_per_phase"
#define SETTINGS "settings"

#define VOTE_POWER "vote_power"
#define VOTE_LABEL "vote"
#define VOTER_LABEL "voter"
#define EXPIRATION_LABEL "expiration"

}; // namespace common
