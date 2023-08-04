#!/bin/sh

# Get the absolute path of the directory where the script resides
SCRIPT_DIR=$(dirname "$(readlink -f "$0")")

# Get the absolute path of the parent directory (one level up) of SCRIPT_DIR
PARENT_DIR=$(dirname "$SCRIPT_DIR")

# Add the /host prefix to PARENT_DIR
HOST_PARENT_DIR="/host$PARENT_DIR"

echo "The absolute path of the script's directory is: $SCRIPT_DIR"
echo "The parent directory path is: $PARENT_DIR"

# docker exec -it dune_container bash -c "cd '$HOST_PARENT_DIR'; bash"
# docker exec -it dune_container bash -c "PS1='$(printf "\033[0;33m\w\033[0m\n> ")'; cd '$HOST_PARENT_DIR'; bash"
# docker exec -it dune_container bash -c "PS1='\[\033[0;33m\]\w\n> \[\033[0m\]'; cd '$HOST_PARENT_DIR'; bash"
docker exec -it dune_container bash -c "cd '$HOST_PARENT_DIR'; PS1='\[\033[0;33m\]\w\n> \[\033[0m\]' bash"
