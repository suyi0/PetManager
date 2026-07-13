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

# Homebrew 升级带版本号的库（如 openssl@3 3.6.2 -> 3.6.3）后，旧的 Cellar 目录会被删除，
# 而 CMakeCache.txt 里钉死的绝对路径不会自动刷新，导致链接期报 "No rule to make target"。
# 这里在 configure 前检查缓存引用的 Cellar 库文件是否仍存在，失效则清缓存重建。
CACHE_FILE="${BUILD_DIR}/CMakeCache.txt"
if [ -f "${CACHE_FILE}" ]; then
    stale_path=""
    while IFS= read -r lib_path; do
        if [ -n "${lib_path}" ] && [ ! -e "${lib_path}" ]; then
            stale_path="${lib_path}"
            break
        fi
    done < <(grep -oE '/opt/homebrew/Cellar/[^;":'"'"' ]+\.(dylib|a|so)' "${CACHE_FILE}" 2>/dev/null | sort -u || true)
    if [ -n "${stale_path}" ]; then
        echo "Stale cached library path detected (removed by a Homebrew upgrade): ${stale_path}"
        echo "Clearing build cache and reconfiguring from scratch..."
        rm -rf "${BUILD_DIR}"
    fi
fi

echo "Configuring backend with CMake..."
"${CMAKE_BIN}" -S "${BACKEND_DIR}" -B "${BUILD_DIR}" -DBUILD_TESTING=OFF

echo "Building backend..."
"${CMAKE_BIN}" --build "${BUILD_DIR}" -j4

echo "Build successful: ${BUILD_DIR}/main"
