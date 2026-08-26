#!/bin/bash
# One-click package & install for harbour-dshq.
# Usage:
#   ./build-rpm.sh            -> rebuild tarball + rpm, install with rpm -Uvh --force
#   ./build-rpm.sh 0.4.0      -> same, but bump Version first (edits the spec)
set -e
cd "$(dirname "$0")"

SPEC=rpm/harbour-dshq.spec

if [ -n "$1" ]; then
    sed -i "s/^Version:.*/Version:    $1/" "$SPEC"
    echo "version bumped to $1"
fi

VERSION=$(sed -n 's/^Version:\s*//p' "$SPEC")

# fresh source tarball straight from the working tree
rm -rf "/tmp/harbour-dshq-$VERSION"
mkdir -p "/tmp/harbour-dshq-$VERSION"
cp -r src qml harbour-dshq.pro harbour-dshq.desktop icons "/tmp/harbour-dshq-$VERSION/"
rm -f /tmp/harbour-dshq-$VERSION/src/*.o
mkdir -p "$HOME/rpmbuild/SOURCES"
tar -cJf "$HOME/rpmbuild/SOURCES/harbour-dshq-$VERSION.tar.xz" -C /tmp "harbour-dshq-$VERSION"

rpmbuild -bb "$SPEC"

RPMS=$(ls -t "$HOME"/rpmbuild/RPMS/aarch64/harbour-dshq-"$VERSION"-*.aarch64.rpm | head -1)
echo "built: $RPMS"

if sudo -n true 2>/dev/null; then
    sudo -n rpm -Uvh --force "$RPMS"
    # refresh launcher icon cache hint
    touch /usr/share/applications/harbour-dshq.desktop 2>/dev/null || \
        sudo -n touch /usr/share/applications/harbour-dshq.desktop
    echo "installed. if the launcher icon looks stale, restart the app (or lipstick)."
else
    echo "sudo not available; install manually:"
    echo "  sudo rpm -Uvh --force $RPMS"
fi
