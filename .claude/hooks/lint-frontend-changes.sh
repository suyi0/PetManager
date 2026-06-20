#!/usr/bin/env bash
# Stop hook: lint ONLY the frontend files changed in the working tree.
# No-op (exit 0) when nothing under pethospital/frontend changed, so the
# common case is cheap. On lint failure, block Stop and feed the report
# back to Claude so it fixes the issues before finishing.
set -u

REPO="$(git rev-parse --show-toplevel 2>/dev/null)" || exit 0
[ -n "$REPO" ] || exit 0

FRONTEND="$REPO/pethospital/frontend"
LINT_BIN="$FRONTEND/node_modules/.bin/vue-cli-service"
# Can't lint without the local binary -> don't block, just no-op.
[ -x "$LINT_BIN" ] || exit 0

# Changed (vs HEAD, staged or unstaged) + untracked frontend source files.
FILES="$(
  {
    git -C "$REPO" diff --name-only HEAD --
    git -C "$REPO" ls-files --others --exclude-standard
  } 2>/dev/null \
    | grep -E '^pethospital/frontend/.*\.(vue|ts|js)$' \
    | sort -u
)"
[ -z "$FILES" ] && exit 0

# Keep only files that still exist on disk (skip deletions/renamed-away).
ABS=""
while IFS= read -r f; do
  [ -n "$f" ] && [ -f "$REPO/$f" ] && ABS="$ABS$REPO/$f
"
done <<EOF
$FILES
EOF
[ -z "$ABS" ] && exit 0

# Report only (no auto-fix): the hook must not silently rewrite files after
# Claude has finished. Run from the frontend so the project eslint config loads.
OUT="$(cd "$FRONTEND" && printf '%s' "$ABS" | tr '\n' '\0' | xargs -0 "$LINT_BIN" lint --no-fix 2>&1)"
STATUS=$?
[ $STATUS -eq 0 ] && exit 0

# Lint failed: block Stop and hand the report to Claude. python3 escapes JSON safely.
python3 - "$OUT" <<'PY'
import json, sys
msg = "Frontend lint failed on the files you changed. Fix these issues, then finish:\n\n" + sys.argv[1]
print(json.dumps({"decision": "block", "reason": msg}))
PY
exit 0
