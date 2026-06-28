#!/usr/bin/env bash

set -euo pipefail

BIN_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "${BIN_DIR}")"
BACKEND_BIN="${PROJECT_ROOT}/pethospital/backend/build/main"
ENV_FILE="${PROJECT_ROOT}/.env"
ORIGINAL_PATH="${PATH}"

if [ -f "${ENV_FILE}" ]; then
    while IFS= read -r line; do
        case "${line}" in
            ''|'#'*)
                continue
                ;;
            *=*)
                key="${line%%=*}"
                if [ "${key}" != "PATH" ]; then
                    export "${line}"
                fi
                ;;
        esac
    done < "${ENV_FILE}"
fi
export PATH="${ORIGINAL_PATH}"

"${BIN_DIR}/build.sh"

if [ ! -x "${BACKEND_BIN}" ]; then
    echo "backend executable not found: ${BACKEND_BIN}"
    exit 1
fi

echo "Starting backend..."
export PETMANAGER_FRONTEND_DIST="${PROJECT_ROOT}/pethospital/frontend/dist"
exec "${BACKEND_BIN}"
