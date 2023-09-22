#!/bin/bash

YELLOW="\e[33m"
DEFAULT="\e[0m"
RED="\e[31m"

echoerr() { 
    echo -e "${RED}Error${DEFAULT} - Invalid arguments: ${RED}$1${DEFAULT}"
    echo "Expected one of the following modes of operation:"
    echo "  Mode 1: setup.sh (dev|eosdev|prod|eosprod|develop|production)        (e.g., 'setup.sh dev')"
    echo "  Mode 2: setup.sh compiler (docker|local) (e.g., 'setup.sh compiler docker')"
}

if [[ $# -eq 1 ]];
then
    if [[ "$1" == "dev" || "$1" == "develop" ]];
    then
        ENV="development"
    elif [[ "$1" == "prod" || "$1" == "production" ]];
    then
        ENV="production"
    elif [[ "$1" == "eosdev" || "$1" == "eosdevelop" ]];
    then
        ENV="eos_development"
    elif [[ "$1" == "eosprod" || "$1" == "eosproduction" ]];
    then
        ENV="eos_production"
    else
        echoerr "Expected one of the following (dev|eosdev|prod|eosprod|develop|production) i.e 'setup.sh dev'"
        exit
    fi

    # Get setup.sh directory since we could be running this script from a different location
    CWD="$(dirname "$0")"
    echo -e "\nConfiguring build for ${YELLOW}"$ENV"${DEFAULT}\n"
    cp "$CWD/templates/config/$ENV.config.hpp" "$CWD/include/config/config.hpp"
    mkdir -p build

elif [[ $# -eq 2 ]];
then
    if [[ "$1" == "compiler" ]];
    then
        if [[ "$2" == "local" ]];
        then
            COMPILER="local"
        elif [[ "$2" == "docker" ]];
        then
            COMPILER="docker"
        else
            echoerr "Expected 'local' or 'docker' as the second argument for mode 2."
            exit
        fi

        # Get setup.sh directory since we could be running this script from a different location
        CWD="$(dirname "$0")"
        echo "\nConfiguring compiler for ${YELLOW}"$COMPILER"${DEFAULT}\n"

        if [[ "$COMPILER" == "local" ]];
        then
            cp "$CWD/templates/cmake/CMakeLists.local.txt" "$CWD/CMakeLists.txt"
            cp "$CWD/templates/cmake/CMakeLists_src.local.txt" "$CWD/src/CMakeLists.txt"
        elif [[ "$COMPILER" == "docker" ]];
        then
            cp "$CWD/templates/cmake/CMakeLists.dune.txt" "$CWD/CMakeLists.txt"
            cp "$CWD/templates/cmake/CMakeLists_src.dune.txt" "$CWD/src/CMakeLists.txt"
        fi
    else
        echoerr "Expected 'compiler' as the first argument for mode 2."
    fi
else
    echoerr "Incorrect number of arguments."
fi
