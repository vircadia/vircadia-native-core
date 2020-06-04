#VIRCADIA=~/Vircadia rpmbuild --target x86_64 -bb vircadia-server.spec
%define version %{lua:print(os.getenv("VERSION"))}
%define depends %{lua:print(os.getenv("DEPENDS"))}

Name:           vircadia-server
Version:        %{version}
Release:        1%{?dist}
Summary:        Vircadia metaverse platform, based on the High Fidelity Engine.

License:        ASL 2.0
URL:            https://vircadia.com
Source0:        https://github.com/kasenvr/vircadia-builder/blob/master/vircadia-builder

#BuildRequires:  systemd-rpm-macros
BuildRequires:  chrpath
Requires:       %{depends}
BuildArch:      x86_64
AutoReq:        no
AutoProv:       no

%description
Vircadia allows creation and sharing of VR experiences.
  The Vircadia metaverse provides built-in social features, including avatar interactions, spatialized audio and interactive physics. Additionally, you have the ability to import any 3D object into your virtual environment.


%prep


%build


%install
rm -rf $RPM_BUILD_ROOT
install -d $RPM_BUILD_ROOT/opt/vircadia
install -m 0755 -t $RPM_BUILD_ROOT/opt/vircadia $VIRCADIA/build/assignment-client/assignment-client
install -m 0755 -t $RPM_BUILD_ROOT/opt/vircadia $VIRCADIA/build/domain-server/domain-server
install -m 0755 -t $RPM_BUILD_ROOT/opt/vircadia $VIRCADIA/build/tools/oven/oven
#install -m 0755 -t $RPM_BUILD_ROOT/opt/vircadia $VIRCADIA/build/ice-server/ice-server
strip --strip-all $RPM_BUILD_ROOT/opt/vircadia/*
chrpath -d $RPM_BUILD_ROOT/opt/vircadia/*
install -m 0755 -t $RPM_BUILD_ROOT/opt/vircadia $VIRCADIA/source/pkg-scripts/new-server
install -d $RPM_BUILD_ROOT/opt/vircadia/lib
install -m 0644 -t $RPM_BUILD_ROOT/opt/vircadia/lib $VIRCADIA/build/libraries/*/*.so
strip --strip-all $RPM_BUILD_ROOT/opt/vircadia/lib/*
chrpath -d $RPM_BUILD_ROOT/opt/vircadia/lib/*
install -m 0644 -t $RPM_BUILD_ROOT/opt/vircadia/lib $VIRCADIA/qt5-install/lib/libQt5Network.so.*.*.*
install -m 0644 -t $RPM_BUILD_ROOT/opt/vircadia/lib $VIRCADIA/qt5-install/lib/libQt5Core.so.*.*.*
install -m 0644 -t $RPM_BUILD_ROOT/opt/vircadia/lib $VIRCADIA/qt5-install/lib/libQt5Widgets.so.*.*.*
install -m 0644 -t $RPM_BUILD_ROOT/opt/vircadia/lib $VIRCADIA/qt5-install/lib/libQt5Gui.so.*.*.*
install -m 0644 -t $RPM_BUILD_ROOT/opt/vircadia/lib $VIRCADIA/qt5-install/lib/libQt5Script.so.*.*.*
install -m 0644 -t $RPM_BUILD_ROOT/opt/vircadia/lib $VIRCADIA/qt5-install/lib/libQt5WebSockets.so.*.*.*
install -m 0644 -t $RPM_BUILD_ROOT/opt/vircadia/lib $VIRCADIA/qt5-install/lib/libQt5Qml.so.*.*.*
install -m 0644 -t $RPM_BUILD_ROOT/opt/vircadia/lib $VIRCADIA/qt5-install/lib/libQt5ScriptTools.so.*.*.*
install -d $RPM_BUILD_ROOT/usr/lib/systemd/system
install -m 0644 -t $RPM_BUILD_ROOT/usr/lib/systemd/system $VIRCADIA/source/pkg-scripts/vircadia-assignment-client.service
install -m 0644 -t $RPM_BUILD_ROOT/usr/lib/systemd/system $VIRCADIA/source/pkg-scripts/vircadia-assignment-client@.service
install -m 0644 -t $RPM_BUILD_ROOT/usr/lib/systemd/system $VIRCADIA/source/pkg-scripts/vircadia-domain-server.service
install -m 0644 -t $RPM_BUILD_ROOT/usr/lib/systemd/system $VIRCADIA/source/pkg-scripts/vircadia-domain-server@.service
#install -m 0644 -t $RPM_BUILD_ROOT/usr/lib/systemd/system $VIRCADIA/source/pkg-scripts/vircadia-ice-server.service
#install -m 0644 -t $RPM_BUILD_ROOT/usr/lib/systemd/system $VIRCADIA/source/pkg-scripts/vircadia-ice-server@.service
install -m 0644 -t $RPM_BUILD_ROOT/usr/lib/systemd/system $VIRCADIA/source/pkg-scripts/vircadia-server.target
install -m 0644 -t $RPM_BUILD_ROOT/usr/lib/systemd/system $VIRCADIA/source/pkg-scripts/vircadia-server@.target
cp -a $VIRCADIA/source/domain-server/resources $RPM_BUILD_ROOT/opt/vircadia
cp -a $VIRCADIA/build/assignment-client/plugins $RPM_BUILD_ROOT/opt/vircadia
chrpath -d $RPM_BUILD_ROOT/opt/vircadia/plugins/*.so
chrpath -d $RPM_BUILD_ROOT/opt/vircadia/plugins/*/*.so
strip --strip-all $RPM_BUILD_ROOT/opt/vircadia/plugins/*.so
strip --strip-all $RPM_BUILD_ROOT/opt/vircadia/plugins/*/*.so
find $RPM_BUILD_ROOT/opt/vircadia/resources -name ".gitignore" -delete


%files
%license $VIRCADIA/source/LICENSE
/opt/vircadia
/usr/lib/systemd/system


%changelog


%post
# create users
getent passwd vircadia >/dev/null 2>&1 || useradd -r -c "Vircadia" -d /var/lib/vircadia -U -M vircadia
#getent group vircadia >/dev/null 2>&1 || groupadd -r vircadia

# create data folder
mkdir -p /etc/opt/vircadia
mkdir -p /var/lib/vircadia && chown vircadia:vircadia /var/lib/vircadia && chmod 775 /var/lib/vircadia

ldconfig -n /opt/vircadia/lib
if [ ! -d "/var/lib/vircadia/default" ]; then
	/opt/vircadia/new-server default 40100
	systemctl enable vircadia-server@default.target
	systemctl start vircadia-server@default.target
fi

%systemd_post vircadia-assignment-client.service
%systemd_post vircadia-assignment-client@.service
%systemd_post vircadia-domain-server.service
%systemd_post vircadia-domain-server@.service
#%systemd_post vircadia-ice-server.service
#%systemd_post vircadia-ice-server@.service
%systemd_post vircadia-server.target
%systemd_post vircadia-server@.target


%preun
%systemd_preun vircadia-server.target
%systemd_preun vircadia-server@.target
%systemd_preun vircadia-assignment-client.service
%systemd_preun vircadia-assignment-client@.service
%systemd_preun vircadia-domain-server.service
%systemd_preun vircadia-domain-server@.service
#%systemd_preun vircadia-ice-server.service
#%systemd_preun vircadia-ice-server@.service


%postun
%systemd_postun_with_restart vircadia-server.target
%systemd_postun_with_restart vircadia-server@.target
%systemd_postun_with_restart vircadia-assignment-client.service
%systemd_postun_with_restart vircadia-assignment-client@.service
%systemd_postun_with_restart vircadia-domain-server.service
%systemd_postun_with_restart vircadia-domain-server@.service
#%systemd_postun_with_restart vircadia-ice-server.service
#%systemd_postun_with_restart vircadia-ice-server@.service
