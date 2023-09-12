#!/bin/bash

## Run this inside the docker container to install ninja

# Function to check if a command is available
command_exists() {
  command -v "$1" >/dev/null 2>&1
}

# Check if required tools (git and cmake) are installed
if ! command_exists git || ! command_exists cmake; then
  echo "Installing required dependencies: git and cmake..."
  apt-get update
  apt-get install -y git cmake
fi

# Install Ninja using apt-get
echo "Installing Ninja..."
apt-get install -y ninja-build


echo "Ninja installation complete."

echo "Installing wasm-opt"

apt-get install binaryen

echo "wasm-opt install complete."
