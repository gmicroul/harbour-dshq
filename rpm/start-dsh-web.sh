#!/bin/bash
# start-dsh-web.sh — extract embedded runtime and start dsh web.
# Called by systemd harbour-dshq-web.service.
set -e

CACHE_DIR="$HOME/.cache/harbour-dshq/node"
DSH_HOME="$HOME/.local/share/harbour-dshq/dsh-home"
NODE_MODULES_TAR="/usr/share/harbour-dshq/node-modules.tar.xz"
DSH_TEMPLATE_TAR="/usr/share/harbour-dshq/dsh-template.tar.xz"

# Extract node_modules if not already done
if [ ! -d "$CACHE_DIR/lib/node_modules/@deepseek-ai/dsh" ]; then
    echo "extracting embedded Node.js runtime …" >&2
    mkdir -p "$CACHE_DIR"
    tar -xJf "$NODE_MODULES_TAR" -C "$CACHE_DIR" --no-same-owner
fi

# Always extract dsh template fresh (overwrite old files)
echo "extracting dsh template …" >&2
rm -rf "$DSH_HOME"
mkdir -p "$DSH_HOME"
tar -xJf "$DSH_TEMPLATE_TAR" -C "$DSH_HOME" --no-same-owner

# Start dsh web
export DSH_HOME
export NODE_ENV=production
cd "$DSH_HOME"
exec "$CACHE_DIR/bin/node" "$CACHE_DIR/bin/dsh" web --host 127.0.0.1 --port 3080 --no-open
