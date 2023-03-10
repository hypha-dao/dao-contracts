#!/bin/sh

echoerr() { 
    echo "\033[0;31m Error - Invalid arguments: \033[0m Expected one of the following (dev|eosdev|prod|eosprod|develop|production) i.e 'setup.sh dev'" 
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
        echoerr
        exit
    fi
    #Get setup.sh directory since we could be running this script from a different location
    CWD="$(dirname "$0")"
    echo "\nConfiguring build for \033[1;33m"$ENV"\033[0m\n"
    cp "$CWD/templates/config/$ENV.config.hpp" $CWD/include/config/config.hpp
    mkdir -p build
else
    echoerr
fi