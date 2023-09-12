## DUNES scripts 

All these scripts are to make building on a docker image easier.

You should install DUNES for Antelope Leap 4.x:
https://github.com/AntelopeIO/DUNES

This installs the correct docker container.

## There is 2 ways to run DUNES
1 - Use the "dune" command to control things inside the docker container
2 - Open a terminal into the docker container, and run things in an environment running the latest Antelope tools. 

dune is intended for the first usecase, but I found it was too restricive, and I get more done just opening a bash inside the docker container and doing things there.

## IMPORTANT!!
The docker container mounts the local file system but it does not reliably update - if we change files
in the local file system, the docker container file system is often not updated. 

The workaround I've found so far is the "touch" command - calling touch on any file inside the docker container updates it to the latest. 

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
