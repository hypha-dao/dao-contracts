// Trail is an on-chain voting platform for the Telos Blockchain Network that provides
// a full suite of voting services for users and developers.
//
// @author Craig Branscom
// @contract trail
// @date October 6th, 2019
// @version v2.0.0-RC2
// @copyright see LICENSE.txt

#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>

using namespace eosio;
using namespace std;

namespace trailservice {
    CONTRACT trail : public contract {

    public:

        //scope: get_self().value
        //ram: 
        TABLE treasury {
            asset supply; //current supply
            asset max_supply; //maximum supply
            name access; //public, private, invite, membership
            name manager; //treasury manager

            string title;
            string description;
            string icon;

            uint32_t voters;
            uint32_t delegates;
            uint32_t committees;
            uint32_t open_ballots;

            bool locked; //locks all settings
            name unlock_acct; //account name to unlock
            name unlock_auth; //authorization name to unlock
            map<name, bool> settings; //setting_name -> on/off

            uint64_t primary_key() const { return supply.symbol.code().raw(); }
            EOSLIB_SERIALIZE(treasury, 
                (supply)(max_supply)(access)(manager)
                (title)(description)(icon)
                (voters)(delegates)(committees)(open_ballots)
                (locked)(unlock_acct)(unlock_auth)(settings))
        };
        typedef multi_index<name("treasuries"), treasury> treasuries_table;

        //scope: get_self().value
        //ram:
        TABLE ballot {
            name ballot_name;
            name category; //proposal, referendum, election, poll, leaderboard
            name publisher;
            name status; //setup, voting, closed, cancelled, archived

            string title; //markdown
            string description; //markdown
            string content; //IPFS link to content or markdown

            symbol treasury_symbol; //treasury used for counting votes
            name voting_method; //1acct1vote, 1tokennvote, 1token1vote, 1tsquare1v, quadratic
            uint8_t min_options; //minimum options per voter
            uint8_t max_options; //maximum options per voter
            map<name, asset> options; //option name -> total weighted votes

            uint32_t total_voters; //number of voters who voted on ballot
            uint32_t total_delegates; //number of delegates who voted on ballot
            asset total_raw_weight; //total raw weight cast on ballot
            uint32_t cleaned_count; //number of expired vote receipts cleaned
            map<name, bool> settings; //setting name -> on/off
            
            time_point_sec begin_time; //time that voting begins
            time_point_sec end_time; //time that voting closes

            uint64_t primary_key() const { return ballot_name.value; }
            EOSLIB_SERIALIZE(ballot, 
                (ballot_name)(category)(publisher)(status)
                (title)(description)(content)
                (treasury_symbol)(voting_method)(min_options)(max_options)(options)
                (total_voters)(total_delegates)(total_raw_weight)(cleaned_count)(settings)
                (begin_time)(end_time))
        };
        typedef multi_index<name("ballots"), ballot> ballots_table;

    };
}