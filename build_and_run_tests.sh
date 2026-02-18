#!/bin/bash

# Build and Run Tests Script
# Simple script to do a clean build and run all tests

set -e  # Exit on error

# Display help
show_help() {
    cat << EOF
Usage: ./build_and_run_tests.sh [MODE]

Builds the Rtrv project and runs all tests.

MODES:
    quick       Incremental build (faster, default)
                Reuses existing build directory and configuration
    
    full        Clean build from scratch
                Removes build directory and reconfigures from scratch
    
    --help      Display this help message

EXAMPLES:
    ./build_and_run_tests.sh            # Quick build (default)
    ./build_and_run_tests.sh quick      # Quick build (explicit)
    ./build_and_run_tests.sh full       # Full clean build

EOF
    exit 0
}

# Check for help flag
if [ "$1" = "--help" ] || [ "$1" = "-h" ]; then
    show_help
fi

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${PROJECT_DIR}/build"
BUILD_TYPE="${1:-quick}"

# Validate build type
if [ "$BUILD_TYPE" != "quick" ] && [ "$BUILD_TYPE" != "full" ]; then
    echo "Error: Invalid build mode '$BUILD_TYPE'"
    echo "Use --help for usage information"
    exit 1
fi

echo "=========================================="
echo "  Rtrv - Build and Test Script"
echo "  Mode: ${BUILD_TYPE}"
echo "=========================================="
echo ""

# Clean build directory (only for full build)
if [ "$BUILD_TYPE" = "full" ]; then
    echo "[1/4] Cleaning build directory..."
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
        echo "  ✓ Build directory cleaned"
    else
        echo "  ✓ No existing build directory"
    fi
    echo ""
else
    echo "[1/4] Skipping clean (quick build)..."
    echo "  ✓ Using existing build directory"
    echo ""
fi

# Create build directory and configure
echo "[2/4] Configuring project with CMake..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
if [ "$BUILD_TYPE" = "full" ] || [ ! -f "CMakeCache.txt" ]; then
    cmake .. -DCMAKE_BUILD_TYPE=Release
    echo "  ✓ Configuration complete"
else
    echo "  ✓ Using existing configuration"
fi
echo ""

# Build the project
echo "[3/4] Building project..."
make -j$(sysctl -n hw.ncpu)
echo "  ✓ Build complete"
echo ""

# Run tests
echo "[4/4] Running tests..."
cd "$BUILD_DIR"
./tests/search_engine_tests
echo ""

echo "=========================================="
echo "  All tests completed!"
echo "=========================================="
