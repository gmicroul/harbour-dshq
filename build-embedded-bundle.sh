#!/bin/bash
# build-embedded-bundle.sh — prepare the Node.js + dsh web runtime that gets
# shipped inside the harbour-dshq RPM so the device needs nothing else.
#
# Produces two compressed tarballs:
#   - node-modules.tar.xz : Node.js v24 runtime + @deepseek-ai/dsh CLI + deps
#   - dsh-template.tar.xz : dsh config template (settings, web profile, credentials)
#
# Both are shipped as single files in the RPM (fast rpmbuild, no 29k file scan).
# The launcher extracts them to user dirs on first run.
set -e
cd "$(dirname "$0")"

SRC_NODE="$HOME/.local/lib/nodejs/node-v24.19.0-linux-arm64"
SRC_DSH="$HOME/.dsh"
SRC_PROFILE="$SRC_DSH/profiles/web"

OUT="/tmp/embedded-bundle"
rm -rf "$OUT"

echo "copying Node.js runtime …"
mkdir -p "$OUT/node"
cp -a "$SRC_NODE/bin"      "$OUT/node/"
cp -a "$SRC_NODE/lib"      "$OUT/node/"
rm -f  "$OUT/node/bin/corepack" "$OUT/node/bin/npm" "$OUT/node/bin/npx"
rm -rf "$OUT/node/lib/node_modules/corepack" "$OUT/node/lib/node_modules/npm"

echo "copying dsh template …"
DEST_TEMPLATE="$OUT/dsh-template/.dsh"
mkdir -p "$DEST_TEMPLATE"
# Use original settings + cordis patch (proven to work on port 3081)
cp "$SRC_DSH/settings.yaml"        "$DEST_TEMPLATE/settings.yaml"
cp "$SRC_DSH/.credentials.yaml"    "$DEST_TEMPLATE/" 2>/dev/null || true
# Copy web profile (node_modules, package.json, etc)
mkdir -p "$DEST_TEMPLATE/profiles/web"
cp -a "$SRC_PROFILE/."              "$DEST_TEMPLATE/profiles/web/"
# Use original cordis.patch.yml (only opencode, settings.yaml has both)
rm -f "$DEST_TEMPLATE/profiles/web/cordis.patch.yml"
cp "$SRC_PROFILE/cordis.patch.yml" "$DEST_TEMPLATE/profiles/web/cordis.patch.yml"
# Also create top-level settings.yaml and .credentials.yaml (dsh web expects both)
cp "$SRC_DSH/settings.yaml"        "$DEST_TEMPLATE/../settings.yaml"
cp "$SRC_DSH/.credentials.yaml"    "$DEST_TEMPLATE/../" 2>/dev/null || true
mkdir -p "$DEST_TEMPLATE/sessions" "$DEST_TEMPLATE/storages" "$DEST_TEMPLATE/.agent-presets"

echo "packing tarballs …"
cd "$OUT"
tar -cJf node-modules.tar.xz -C node bin lib
tar -cJf dsh-template.tar.xz -C dsh-template .
cd -
rm -rf "$OUT/node" "$OUT/dsh-template"

echo "embedded bundle ready at $OUT"
echo "sizes:"
du -sh "$OUT/node-modules.tar.xz" "$OUT/dsh-template.tar.xz" 2>/dev/null
