#!/usr/bin/env bash

set -euo pipefail

BIN_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "${BIN_DIR}")"
BACKEND_BIN="${PROJECT_ROOT}/pethospital/backend/build/main"
ENV_FILE="${PROJECT_ROOT}/.env"

if [ -f "${ENV_FILE}" ]; then
    while IFS= read -r line; do
        case "${line}" in
            ''|'#'*)
                continue
                ;;
            *=*)
                export "${line}"
                ;;
        esac
    done < "${ENV_FILE}"
fi

"${BIN_DIR}/build.sh"

if [ ! -x "${BACKEND_BIN}" ]; then
    echo "backend executable not found: ${BACKEND_BIN}"
    exit 1
fi

echo "Starting backend..."
exec "${BACKEND_BIN}"
