Name:       harbour-dshq
Summary:    DeepSeek Harness chat client for Sailfish OS (full, embedded Node.js)
Version:    0.4.0
Release:    1
Group:      Applications/Internet
License:    MIT
URL:        https://127.0.0.1:3080
Source0:    %{name}-%{version}.tar.xz
BuildArch:  aarch64
BuildRequires: pkgconfig(Qt5Quick)
BuildRequires: pkgconfig(Qt5Qml)
BuildRequires: pkgconfig(Qt5Network)
BuildRequires: pkgconfig(Qt5WebSockets)
BuildRequires: pkgconfig(Qt5DBus)
AutoReqProv: 0
Requires: sailfishsilica-qt5 >= 0.10.9

%description
Qt Quick (Silica) chat client for a local DeepSeek Harness web service.
This full edition bundles the Node.js runtime and dsh web profile — no
system Node.js installation required. Works out of the box with kilo-auto/free.

%prep
%setup -q

%build
qmake
make %{?_smp_mflags}

%install
rm -rf %{buildroot}

# ── Qt binary ────────────────────────────────────────────────────────────
mkdir -p %{buildroot}%{_libexecdir}/%{name}
install -m 755 harbour-dshq %{buildroot}%{_libexecdir}/%{name}/%{name}

# ── Launcher script (calls dsh web + Qt) ─────────────────────────────────
mkdir -p %{buildroot}%{_bindir}
install -m 755 rpm/harbour-dshq-launcher.sh %{buildroot}%{_bindir}/%{name}

# ── Support scripts ──────────────────────────────────────────────────────
mkdir -p %{buildroot}%{_libdir}/%{name}
install -m 755 rpm/start-dsh-web.sh %{buildroot}%{_libdir}/%{name}/start-dsh-web.sh

# ── QML ──────────────────────────────────────────────────────────────────
mkdir -p %{buildroot}%{_datadir}/%{name}/qml/pages
mkdir -p %{buildroot}%{_datadir}/%{name}/qml/cover
mkdir -p %{buildroot}%{_datadir}/%{name}/qml/images
install -m 644 qml/harbour-dshq.qml %{buildroot}%{_datadir}/%{name}/qml/
install -m 644 qml/FishBackground.qml %{buildroot}%{_datadir}/%{name}/qml/
install -m 644 qml/pages/*.qml %{buildroot}%{_datadir}/%{name}/qml/pages/
install -m 644 qml/cover/*.qml %{buildroot}%{_datadir}/%{name}/qml/cover/
install -m 644 qml/images/*.png %{buildroot}%{_datadir}/%{name}/qml/images/

# ── Embedded tarballs (single files for fast rpmbuild) ───────────────────
mkdir -p %{buildroot}%{_datadir}/%{name}
if [ -f embedded/node-modules.tar.xz ]; then
    install -m 644 embedded/node-modules.tar.xz %{buildroot}%{_datadir}/%{name}/
fi
if [ -f embedded/dsh-template.tar.xz ]; then
    install -m 644 embedded/dsh-template.tar.xz %{buildroot}%{_datadir}/%{name}/
fi

# ── systemd user service ─────────────────────────────────────────────────
mkdir -p %{buildroot}%{_prefix}/lib/systemd/user
install -m 644 rpm/harbour-dshq-web.service %{buildroot}%{_prefix}/lib/systemd/user/harbour-dshq-web.service

# ── .desktop ─────────────────────────────────────────────────────────────
mkdir -p %{buildroot}%{_datadir}/applications
sed 's|^Exec=.*$|Exec=/usr/libexec/%{name}/%{name}|' \
    %{name}.desktop > %{buildroot}%{_datadir}/applications/%{name}.desktop

mkdir -p %{buildroot}%{_datadir}/icons/hicolor/86x86/apps
install -m 644 icons/%{name}.png %{buildroot}%{_datadir}/icons/hicolor/86x86/apps/

%post
systemctl --user daemon-reload 2>/dev/null || true
systemctl --user enable harbour-dshq-web.service 2>/dev/null || true
systemctl --user start harbour-dshq-web.service 2>/dev/null || true

%preun
systemctl --user disable harbour-dshq-web.service 2>/dev/null || true
systemctl --user stop harbour-dshq-web.service 2>/dev/null || true

%postun
systemctl --user daemon-reload 2>/dev/null || true

%files
%defattr(-,root,root,-)
%{_bindir}/%{name}
%{_libexecdir}/%{name}/%{name}
%{_libdir}/%{name}/start-dsh-web.sh
%dir %{_datadir}/%{name}
%{_datadir}/%{name}/qml
%{_datadir}/%{name}/node-modules.tar.xz
%{_datadir}/%{name}/dsh-template.tar.xz
%{_prefix}/lib/systemd/user/harbour-dshq-web.service
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/86x86/apps/%{name}.png
