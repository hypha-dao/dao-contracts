#!/bin/sh
nodeos --genesis-json ./genesis.json -e -p eosio \
  --plugin eosio::producer_plugin  \
  --max-transaction-time 1000 \
  --plugin eosio::producer_api_plugin \
  --plugin eosio::chain_api_plugin \
  --plugin eosio::http_plugin \
  --plugin eosio::history_plugin \
  --plugin eosio::history_api_plugin \
  --filter-on='*' \
  --access-control-allow-origin='*' \
  --contracts-console \
  --http-validate-host=false \
  --verbose-http-errors \
  --delete-all-blocks \
  --http-max-response-time-ms=5000 \
  --max-body-size=2097152 \
  &> nodeos.log
