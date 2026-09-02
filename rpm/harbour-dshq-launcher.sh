#!/bin/bash
# harbour-dshq-launcher — launcher for harbour-dshq Qt client (full edition).
#
# Extracts the embedded Node.js runtime + dsh template on first run, starts
# dsh web, waits for health, then exec's the Qt UI. No system Node.js needed.
set -e

HOST="127.0.0.1"
PORT="3080"
BASE_URL="http://${HOST}:${PORT}"
HEALTH_URL="${BASE_URL}/health"
CACHE_DIR="$HOME/.cache/harbour-dshq/node"
DSH_HOME="$HOME/.local/share/harbour-dshq/dsh-home"

# ── Extract embedded node_modules on first run ───────────────────────────
if [ ! -d "$CACHE_DIR/lib/node_modules/@deepseek-ai/dsh" ]; then
    echo "first run: extracting embedded Node.js runtime …" >&2
    mkdir -p "$CACHE_DIR"
    tar -xJf "/usr/share/harbour-dshq/node-modules.tar.xz" -C "$CACHE_DIR" --no-same-owner
    echo "extracted to $CACHE_DIR" >&2
fi

# ── Extract dsh template on first run ────────────────────────────────────
if [ ! -d "$DSH_HOME/.dsh" ]; then
    echo "first run: extracting dsh template …" >&2
    mkdir -p "$DSH_HOME"
    tar -xJf "/usr/share/harbour-dshq/dsh-template.tar.xz" -C "$DSH_HOME" --no-same-owner
    echo "extracted to $DSH_HOME" >&2
fi

# ── Helper: is the web server already up? (bypass proxy) ─────────────────
web_is_up() {
    curl -sf --noproxy '*' -o /dev/null --max-time 2 "$HEALTH_URL" 2>/dev/null
}

# ── Try to reuse an already-running instance ────────────────────────────
if web_is_up; then
    echo "dsh web already running (port $PORT up)" >&2
else
    # Resolve the dsh CLI script
    DSH_CLI=""
    if [ -x "$CACHE_DIR/bin/dsh" ]; then
        DSH_CLI="$CACHE_DIR/bin/dsh"
        NODE_BIN="$CACHE_DIR/bin/node"
    elif [ -x "$CACHE_DIR/lib/node_modules/@deepseek-ai/dsh/lib/bin.js" ]; then
        DSH_CLI="$CACHE_DIR/lib/node_modules/@deepseek-ai/dsh/lib/bin.js"
        NODE_BIN="$CACHE_DIR/bin/node"
    else
        echo "ERROR: dsh CLI not found under $CACHE_DIR" >&2
        exit 1
    fi

    export DSH_HOME
    export NODE_ENV=production

    echo "starting dsh web via $NODE_BIN …" >&2
    cd "$DSH_HOME"
    nohup "$NODE_BIN" "$DSH_CLI" web \
        --host "$HOST" \
        --port "$PORT" \
        --no-open \
        >> /tmp/harbour-dshq-web.log 2>&1 &

    # Wait up to 30 s for health
    tries=0
    while [ "$tries" -lt 30 ]; do
        if web_is_up; then
            echo "dsh web is up" >&2
            break
        fi
        sleep 1
        tries=$((tries + 1))
    done
    if ! web_is_up; then
        echo "dsh web did not start within 30s — see /tmp/harbour-dshq-web.log" >&2
        exit 1
    fi
fi

# ── Launch the Qt UI ────────────────────────────────────────────────────
exec /usr/libexec/harbour-dshq/harbour-dshq "$@"
