#!/bin/bash

# Get the directory where the script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" &> /dev/null && pwd)"

# Set the build directory relative to the script directory
BUILD_DIR="$SCRIPT_DIR/../build_ninja"

echo "Running wasm-opt on $BUILD_DIR/dao/dao.wasm"

wasm-opt -Oz -o "$BUILD_DIR/dao/dao_O3.wasm" "$BUILD_DIR/dao/dao.wasm"

echo "Optimized wasm saved in $BUILD_DIR/dao/dao_O3.wasm"

# Get the file sizes of dao.wasm and dao_O3.wasm in kilobytes
original_size_kb=$(du -k "$BUILD_DIR/dao/dao.wasm" | cut -f1)
optimized_size_kb=$(du -k "$BUILD_DIR/dao/dao_O3.wasm" | cut -f1)

# Convert kilobytes to bytes
original_size=$((original_size_kb * 1024))
optimized_size=$((optimized_size_kb * 1024))

echo "Original file size (dao.wasm): $original_size bytes"
echo "Optimized file size (dao_O3.wasm): $optimized_size bytes"

# Calculate and display the file size reduction percentage
percentage=$(awk "BEGIN { pc=100*(${original_size}-${optimized_size})/${original_size}; i=int(pc); print (pc-i<0.5)?i:i+1 }")
echo "File size reduced by $percentage%"
