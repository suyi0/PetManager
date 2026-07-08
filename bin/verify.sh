#!/usr/bin/env bash

# One-shot verification: backend build + ctest, frontend build + lint.
# Usage: bin/verify.sh [backend|frontend|all]   (default: all)

set -euo pipefail

BIN_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "${BIN_DIR}")"
BACKEND_DIR="${PROJECT_ROOT}/pethospital/backend"
# 测试构建用独立目录，绝不碰日常 build/：
# verify 若与 bin/build.sh 共用 build/，BUILD_TESTING=ON 会把测试目标灌进
# build/compile_commands.json（同一源文件多条目 + 测试宏），VSCode cpptools
# 以它为准 → 大纲/跳转/悬停解析错乱。隔离后 IDE 所见配置永远是日常配置。
BUILD_DIR="${BACKEND_DIR}/build-tests"
FRONTEND_DIR="${PROJECT_ROOT}/pethospital/frontend"
CMAKE_BIN="${CMAKE_BIN:-/opt/homebrew/bin/cmake}"

TARGET="${1:-all}"
case "${TARGET}" in
    backend|frontend|all) ;;
    *)
        echo "Usage: bin/verify.sh [backend|frontend|all]"
        exit 1
        ;;
esac

if [ ! -x "${CMAKE_BIN}" ]; then
    if command -v cmake >/dev/null 2>&1; then
        CMAKE_BIN="$(command -v cmake)"
    else
        echo "cmake not found"
        exit 1
    fi
fi

CTEST_BIN="$(dirname "${CMAKE_BIN}")/ctest"
if [ ! -x "${CTEST_BIN}" ]; then
    CTEST_BIN="ctest"
fi

verify_backend() {
    echo "==> Backend: configure (BUILD_TESTING=ON)"
    "${CMAKE_BIN}" -S "${BACKEND_DIR}" -B "${BUILD_DIR}" -DBUILD_TESTING=ON

    echo "==> Backend: build"
    "${CMAKE_BIN}" --build "${BUILD_DIR}" -j4

    echo "==> Backend: ctest"
    (cd "${BUILD_DIR}" && "${CTEST_BIN}" --output-on-failure)
}

verify_frontend() {
    echo "==> Frontend: npm run build"
    (cd "${FRONTEND_DIR}" && npm run build)

    echo "==> Frontend: npm run lint"
    (cd "${FRONTEND_DIR}" && npm run lint)
}

if [ "${TARGET}" = "backend" ] || [ "${TARGET}" = "all" ]; then
    verify_backend
fi

if [ "${TARGET}" = "frontend" ] || [ "${TARGET}" = "all" ]; then
    verify_frontend
fi

echo ""
echo "✅ verify (${TARGET}) passed"
