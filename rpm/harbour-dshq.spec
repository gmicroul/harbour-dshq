Name:       harbour-dshq
Summary:    DeepSeek Harness chat client for Sailfish OS
Version:    0.3.0
Release:    9
Group:      Applications/Internet
License:    MIT
URL:        https://127.0.0.1:3080
Source0:    %{name}-%{version}.tar.xz
BuildArch:  aarch64
Requires:   sailfishsilica-qt5 >= 0.10.9
BuildRequires: pkgconfig(Qt5Quick)
BuildRequires: pkgconfig(Qt5Qml)
BuildRequires: pkgconfig(Qt5Network)
BuildRequires: pkgconfig(Qt5WebSockets)
BuildRequires: pkgconfig(Qt5DBus)

%description
Qt Quick (Silica) chat client for a local DeepSeek Harness web service:
session history with search, resumable conversations, paginated history,
streamed replies, and per-session model switching.

%prep
%setup -q

%build
qmake
make %{?_smp_mflags}

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}%{_bindir}
install -m 755 harbour-dshq %{buildroot}%{_bindir}/%{name}
mkdir -p %{buildroot}%{_datadir}/%{name}/qml/pages
mkdir -p %{buildroot}%{_datadir}/%{name}/qml/cover
mkdir -p %{buildroot}%{_datadir}/%{name}/qml/images
install -m 644 qml/harbour-dshq.qml %{buildroot}%{_datadir}/%{name}/qml/
install -m 644 qml/FishBackground.qml %{buildroot}%{_datadir}/%{name}/qml/
install -m 644 qml/pages/*.qml %{buildroot}%{_datadir}/%{name}/qml/pages/
install -m 644 qml/cover/*.qml %{buildroot}%{_datadir}/%{name}/qml/cover/
install -m 644 qml/images/*.png %{buildroot}%{_datadir}/%{name}/qml/images/
mkdir -p %{buildroot}%{_datadir}/applications
desktop-file-install --dir=%{buildroot}%{_datadir}/applications %{name}.desktop 2>/dev/null || \
  install -m 644 %{name}.desktop %{buildroot}%{_datadir}/applications/
mkdir -p %{buildroot}%{_datadir}/icons/hicolor/86x86/apps
install -m 644 icons/%{name}.png %{buildroot}%{_datadir}/icons/hicolor/86x86/apps/

%files
%defattr(-,root,root,-)
%{_bindir}/%{name}
%dir %{_datadir}/%{name}
%{_datadir}/%{name}/qml
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/86x86/apps/%{name}.png
