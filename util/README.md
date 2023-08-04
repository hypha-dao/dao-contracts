## DUNES scripts 

All these scripts are to make building on a docker image easier.

You should install DUNES for Antelope Leap 4.x:
https://github.com/AntelopeIO/DUNES

This installs the correct docker container.

## dunes_terminal

Enter the docker container

This script opens a bash shell onto the docker container

## Docker: install_ninja.sh

This script is to be run inside the docker container and installs the ninja build tool

## Docker: run_make.sh

This script runs cmake and ninja to build the conract

## Docker check_config.sh

Check current build configuration

## Docker optimize_wasm - runs wasm-opt
