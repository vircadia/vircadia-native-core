#ATHENA=~/Athena rpmbuild --target x86_64 -bb athena-server.spec
%define version %{lua:print(os.getenv("VERSION"))}
%define depends %{lua:print(os.getenv("DEPENDS"))}

Name:           athena-server
Version:        %{version}
Release:        1%{?dist}
Summary:        Project Athena metaverse platform, based on the High Fidelity Engine.

License:        ASL 2.0
URL:            https://projectathena.io
Source0:        https://github.com/daleglass/athena-builder/blob/master/athena_builder

#BuildRequires:  systemd-rpm-macros
BuildRequires:  chrpath
Requires:       %{depends}
BuildArch:      x86_64
AutoReq:        no
AutoProv:       no

%description
Project Athena allows creation and sharing of VR experiences.
  The Project Athena metaverse provides built-in social features, including avatar interactions, spatialized audio and interactive physics. Additionally, you have the ability to import any 3D object into your virtual environment.


%prep


%build


%install
rm -rf $RPM_BUILD_ROOT
install -d $RPM_BUILD_ROOT/opt/athena
install -m 0755 -t $RPM_BUILD_ROOT/opt/athena $ATHENA/build/assignment-client/assignment-client
install -m 0755 -t $RPM_BUILD_ROOT/opt/athena $ATHENA/build/domain-server/domain-server
install -m 0755 -t $RPM_BUILD_ROOT/opt/athena $ATHENA/build/tools/oven/oven
#install -m 0755 -t $RPM_BUILD_ROOT/opt/athena $ATHENA/build/ice-server/ice-server
strip --strip-all $RPM_BUILD_ROOT/opt/athena/*
chrpath -d $RPM_BUILD_ROOT/opt/athena/*
install -m 0755 -t $RPM_BUILD_ROOT/opt/athena $ATHENA/source/pkg-scripts/new-server
install -d $RPM_BUILD_ROOT/opt/athena/lib
install -m 0644 -t $RPM_BUILD_ROOT/opt/athena/lib $ATHENA/build/libraries/*/*.so
strip --strip-all $RPM_BUILD_ROOT/opt/athena/lib/*
chrpath -d $RPM_BUILD_ROOT/opt/athena/lib/*
install -m 0644 -t $RPM_BUILD_ROOT/opt/athena/lib $ATHENA/qt5-install/lib/libQt5Network.so.*.*.*
install -m 0644 -t $RPM_BUILD_ROOT/opt/athena/lib $ATHENA/qt5-install/lib/libQt5Core.so.*.*.*
install -m 0644 -t $RPM_BUILD_ROOT/opt/athena/lib $ATHENA/qt5-install/lib/libQt5Widgets.so.*.*.*
install -m 0644 -t $RPM_BUILD_ROOT/opt/athena/lib $ATHENA/qt5-install/lib/libQt5Gui.so.*.*.*
install -m 0644 -t $RPM_BUILD_ROOT/opt/athena/lib $ATHENA/qt5-install/lib/libQt5Script.so.*.*.*
install -m 0644 -t $RPM_BUILD_ROOT/opt/athena/lib $ATHENA/qt5-install/lib/libQt5Quick.so.*.*.*
install -m 0644 -t $RPM_BUILD_ROOT/opt/athena/lib $ATHENA/qt5-install/lib/libQt5WebSockets.so.*.*.*
install -m 0644 -t $RPM_BUILD_ROOT/opt/athena/lib $ATHENA/qt5-install/lib/libQt5Qml.so.*.*.*
install -m 0644 -t $RPM_BUILD_ROOT/opt/athena/lib $ATHENA/qt5-install/lib/libQt5ScriptTools.so.*.*.*
install -m 0644 -t $RPM_BUILD_ROOT/opt/athena/lib $ATHENA/build/ext/makefiles/quazip/project/lib/libquazip5.so.*.*.*
install -d $RPM_BUILD_ROOT/usr/lib/systemd/system
install -m 0644 -t $RPM_BUILD_ROOT/usr/lib/systemd/system $ATHENA/source/pkg-scripts/athena-assignment-client.service
install -m 0644 -t $RPM_BUILD_ROOT/usr/lib/systemd/system $ATHENA/source/pkg-scripts/athena-assignment-client@.service
install -m 0644 -t $RPM_BUILD_ROOT/usr/lib/systemd/system $ATHENA/source/pkg-scripts/athena-domain-server.service
install -m 0644 -t $RPM_BUILD_ROOT/usr/lib/systemd/system $ATHENA/source/pkg-scripts/athena-domain-server@.service
#install -m 0644 -t $RPM_BUILD_ROOT/usr/lib/systemd/system $ATHENA/source/pkg-scripts/athena-ice-server.service
#install -m 0644 -t $RPM_BUILD_ROOT/usr/lib/systemd/system $ATHENA/source/pkg-scripts/athena-ice-server@.service
install -m 0644 -t $RPM_BUILD_ROOT/usr/lib/systemd/system $ATHENA/source/pkg-scripts/athena-server.target
install -m 0644 -t $RPM_BUILD_ROOT/usr/lib/systemd/system $ATHENA/source/pkg-scripts/athena-server@.target
cp -a $ATHENA/source/domain-server/resources $RPM_BUILD_ROOT/opt/athena
cp -a $ATHENA/build/assignment-client/plugins $RPM_BUILD_ROOT/opt/athena
chrpath -d $RPM_BUILD_ROOT/opt/athena/plugins/*.so
chrpath -d $RPM_BUILD_ROOT/opt/athena/plugins/*/*.so
strip --strip-all $RPM_BUILD_ROOT/opt/athena/plugins/*.so
strip --strip-all $RPM_BUILD_ROOT/opt/athena/plugins/*/*.so
find $RPM_BUILD_ROOT/opt/athena/resources -name ".gitignore" -delete


%files
%license $ATHENA/source/LICENSE
/opt/athena
/usr/lib/systemd/system


%changelog


%post
# create users
getent passwd athena >/dev/numm 2>&1 || useradd -r -c "Project Athena" -d /var/lib/athena -U -M athena
#getent group athena >/dev/null 2>&1 || groupadd -r athena

# create data folder
mkdir -p /etc/opt/athena
mkdir -p /var/lib/athena && chown athena:athena /var/lib/athena && chmod 775 /var/lib/athena

ldconfig -n /opt/athena/lib
if [ ! -d "/var/lib/athena/default" ]; then
	/opt/athena/new-server default 40100
	systemctl enable athena-server@default.target
	systemctl start athena-server@default.target
fi

%systemd_post athena-assignment-client.service
%systemd_post athena-assignment-client@.service
%systemd_post athena-domain-server.service
%systemd_post athena-domain-server@.service
#%systemd_post athena-ice-server.service
#%systemd_post athena-ice-server@.service
%systemd_post athena-server.target
%systemd_post athena-server@.target


%preun
%systemd_preun athena-server.target
%systemd_preun athena-server@.target
%systemd_preun athena-assignment-client.service
%systemd_preun athena-assignment-client@.service
%systemd_preun athena-domain-server.service
%systemd_preun athena-domain-server@.service
#%systemd_preun athena-ice-server.service
#%systemd_preun athena-ice-server@.service


%postun
%systemd_postun_with_restart athena-server.target
%systemd_postun_with_restart athena-server@.target
%systemd_postun_with_restart athena-assignment-client.service
%systemd_postun_with_restart athena-assignment-client@.service
%systemd_postun_with_restart athena-domain-server.service
%systemd_postun_with_restart athena-domain-server@.service
#%systemd_postun_with_restart athena-ice-server.service
#%systemd_postun_with_restart athena-ice-server@.service
