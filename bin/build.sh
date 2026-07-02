#!/usr/bin/env bash

set -euo pipefail

BIN_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "${BIN_DIR}")"
BACKEND_DIR="${PROJECT_ROOT}/pethospital/backend"
BUILD_DIR="${BACKEND_DIR}/build"
CMAKE_BIN="${CMAKE_BIN:-/opt/homebrew/bin/cmake}"

if [ ! -x "${CMAKE_BIN}" ]; then
    if command -v cmake >/dev/null 2>&1; then
        CMAKE_BIN="$(command -v cmake)"
    else
        echo "cmake not found"
        exit 1
    fi
fi

echo "Configuring backend with CMake..."
"${CMAKE_BIN}" -S "${BACKEND_DIR}" -B "${BUILD_DIR}" -DBUILD_TESTING=OFF

echo "Building backend..."
"${CMAKE_BIN}" --build "${BUILD_DIR}" -j4

echo "Build successful: ${BUILD_DIR}/main"
