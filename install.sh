#!/bin/bash
# DEV quick-deploy for harbour-dshq (builds into /usr/local/bin).
# NOTE: prefer the RPM package for normal installation:
#   rpmbuild -bb rpm/harbour-dshq.spec && sudo -n rpm -Uvh \
#     ~/rpmbuild/RPMS/aarch64/harbour-dshq-*.rpm
# Running this script replaces the binary/QML out-of-band; it no longer
# touches ~/.local/share/applications so the packaged .desktop stays valid.
set -e
cd "$(dirname "$0")"

make -j4 >/dev/null

sudo -n install -m 755 harbour-dshq /usr/local/bin/harbour-dshq

sudo -n mkdir -p /usr/share/harbour-dshq/qml/pages /usr/share/harbour-dshq/qml/cover /usr/share/harbour-dshq/qml/images
sudo -n cp qml/harbour-dshq.qml /usr/share/harbour-dshq/qml/
sudo -n cp qml/FishBackground.qml /usr/share/harbour-dshq/qml/
sudo -n cp qml/pages/*.qml /usr/share/harbour-dshq/qml/pages/
sudo -n cp qml/cover/*.qml /usr/share/harbour-dshq/qml/cover/
sudo -n cp qml/images/*.png /usr/share/harbour-dshq/qml/images/
# cp inherits the caller's umask; enforce world-readable or the app can't load them
sudo -n chmod 755 /usr/share/harbour-dshq/qml /usr/share/harbour-dshq/qml/pages /usr/share/harbour-dshq/qml/cover /usr/share/harbour-dshq/qml/images
sudo -n chmod 644 /usr/share/harbour-dshq/qml/harbour-dshq.qml \
                  /usr/share/harbour-dshq/qml/FishBackground.qml \
                  /usr/share/harbour-dshq/qml/pages/*.qml \
                  /usr/share/harbour-dshq/qml/cover/*.qml \
                  /usr/share/harbour-dshq/qml/images/*.png

# launcher picks up [X-Sailjail] Sandboxing=Disabled from the packaged desktop file
sudo -n install -m 644 harbour-dshq.desktop /usr/share/applications/harbour-dshq.desktop

echo "installed (dev mode). launch: /usr/local/bin/harbour-dshq"
echo "note: the launcher icon uses the RPM-installed copy at /usr/bin/harbour-dshq"
