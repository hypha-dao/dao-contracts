#!/bin/bash

# Function to check if a command is available
command_exists() {
  command -v "$1" >/dev/null 2>&1
}

# Check if Ninja is installed
if ! command_exists ninja; then
  echo "Ninja is not installed. Please run the Ninja installation script first."
  exit 1
fi

# Check if required tools (git, cmake) are installed
if ! command_exists git || ! command_exists cmake; then
  echo "Required dependencies (git and cmake) not found. Please install them first."
  exit 1
fi

# Directory where the build will take place
BUILD_DIR="build_ninja"

# Create build directory if it doesn't exist
mkdir -p "$BUILD_DIR"

# Run CMake to generate Ninja Makefiles
echo "Running CMake..."
cmake -S . -B "$BUILD_DIR" -G Ninja

# Change directory to build folder
cd "$BUILD_DIR" || exit 1

# Run Ninja to build the project
echo "Running Ninja..."
ninja

# Optionally, you can run any tests or other commands here if needed.

# Return to the original directory
cd ..

echo "Build complete."
