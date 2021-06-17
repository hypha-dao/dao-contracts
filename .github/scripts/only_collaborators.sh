#!/bin/env bash

TOKEN="${TOKEN:-$GITHUB_TOKEN}"
CURL="${CURL_COMMAND:-curl --silent}"
USER="${USER:-$GITHUB_ACTOR}"

echo "Using curl as \"${CURL}\""

function call_curl {
  ${CURL} -H "Authorization: Bearer ${TOKEN}" -s -o /dev/null -w %{http_code} "$@"
}

echo "Testing if ${USER} is collaborator of this repository"

IS_COLLABORATOR="https://api.github.com/repos/hypha-dao/dao-contracts/collaborators/${USER}"
RESPONSE=`call_curl "${IS_COLLABORATOR}"`

if [ "$RESPONSE" = "204" ]; then
    exit 0;
fi

echo "This workflow only runs for collaborators of this repo"
exit 1
