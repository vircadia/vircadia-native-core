#ATHENA=~/Athena rpmbuild --target x86_64 -bb athena-server.spec
Name:           athena-server
Version:        0.86.0_K1_20200128_486c7bde5bedf152e70fc63281f14da26ecec738
Release:        2%{?dist}
Summary:        Project Athena metaverse platform, based on the High Fidelity Engine.

License:        ASL 2.0
URL:            https://projectathena.io
Source0:        https://github.com/daleglass/athena-builder/blob/master/athena_builder

#BuildRequires:  systemd-rpm-macros
BuildRequires:  chrpath
Requires:       alsa-lib, cups, glib2, gtk3, libdrm, libinput, libjpeg-turbo, libtiff, libxkbcommon, libxkbcommon-x11, mesa-libEGL, mesa-libGL, mesa-libgbm, pcre2, pcre2-utf16, sqlite, xkeyboard-config, zlib, at-spi2-core, dbus, fontconfig, harfbuzz, libICE, libSM, libicu, libmng, libpng, libproxy, libxcb, openssl, pcre, pulseaudio-libs, unixODBC, xcb-util-image, xcb-util-keysyms, xcb-util-renderutil, xcb-util-wm, libxcb, nss, jsoncpp, libxslt, libvpx, libicu, libxml2, opus, libicu, libwebp, libXtst
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
#install -m 0755 -t $RPM_BUILD_ROOT/opt/athena $ATHENA/build/ice-server/ice-server
chrpath -d $RPM_BUILD_ROOT/opt/athena/*
install -m 0755 -t $RPM_BUILD_ROOT/opt/athena $ATHENA/source/pkg-scripts/new-server
install -d $RPM_BUILD_ROOT/opt/athena/lib
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
install -d $RPM_BUILD_ROOT/opt/athena/systemd
install -m 0644 -t $RPM_BUILD_ROOT/opt/athena/systemd $ATHENA/source/pkg-scripts/athena-assignment-client.service
install -m 0644 -t $RPM_BUILD_ROOT/opt/athena/systemd $ATHENA/source/pkg-scripts/athena-assignment-client@.service
install -m 0644 -t $RPM_BUILD_ROOT/opt/athena/systemd $ATHENA/source/pkg-scripts/athena-domain-server.service
install -m 0644 -t $RPM_BUILD_ROOT/opt/athena/systemd $ATHENA/source/pkg-scripts/athena-domain-server@.service
#install -m 0644 -t $RPM_BUILD_ROOT/opt/athena/systemd $ATHENA/source/pkg-scripts/athena-ice-server.service
#install -m 0644 -t $RPM_BUILD_ROOT/opt/athena/systemd $ATHENA/source/pkg-scripts/athena-ice-server@.service
install -m 0644 -t $RPM_BUILD_ROOT/opt/athena/systemd $ATHENA/source/pkg-scripts/athena-server.target
install -m 0644 -t $RPM_BUILD_ROOT/opt/athena/systemd $ATHENA/source/pkg-scripts/athena-server@.target
cp -a $ATHENA/source/domain-server/resources $RPM_BUILD_ROOT/opt/athena


%files
%license $ATHENA/source/LICENSE
/opt/athena


%changelog


%post
# create users
getent passwd athena >/dev/numm 2>&1 || useradd -r -c "Project Athena" -d /var/lib/athena -U -M athena
#getent group athena >/dev/null 2>&1 || groupadd -r athena

# create system scripts
if [ ! -e "/usr/lib/systemd/system/athena-assignment-client.service" ]; then
	ln -s /opt/athena/systemd/athena-assignment-client.service /usr/lib/systemd/system/athena-assignment-client.service
fi
if [ ! -e "/usr/lib/systemd/system/athena-assignment-client@.service" ]; then
	ln -s /opt/athena/systemd/athena-assignment-client@.service /usr/lib/systemd/system/athena-assignment-client@.service
fi
if [ ! -e "/usr/lib/systemd/system/athena-domain-server.service" ]; then
	ln -s /opt/athena/systemd/athena-domain-server.service /usr/lib/systemd/system/athena-domain-server.service
fi
if [ ! -e "/usr/lib/systemd/system/athena-domain-server@.service" ]; then
	ln -s /opt/athena/systemd/athena-domain-server@.service /usr/lib/systemd/system/athena-domain-server@.service
fi
#if [ ! -e "/usr/lib/systemd/system/athena-ice-server.service" ]; then
#	ln -s /opt/athena/systemd/athena-ice-server.service /usr/lib/systemd/system/athena-ice-server.service
#fi
#if [ ! -e "/usr/lib/systemd/system/athena-ice-server@.service" ]; then
#	ln -s /opt/athena/systemd/athena-ice-server@.service /usr/lib/systemd/system/athena-ice-server@.service
#fi
if [ ! -e "/usr/lib/systemd/system/athena-server.target" ]; then
	ln -s /opt/athena/systemd/athena-server.target /usr/lib/systemd/system/athena-server.target
fi
if [ ! -e "/usr/lib/systemd/system/athena-server@.target" ]; then
	ln -s /opt/athena/systemd/athena-server@.target /usr/lib/systemd/system/athena-server@.target
fi

# create data folder
mkdir -p /etc/opt/athena
mkdir -p /var/lib/athena && chown athena:athena /var/lib/athena && chmod 775 /var/lib/athena

ldconfig -n /opt/athena/lib
if [ ! -d "/var/lib/athena/default" ]; then
	/opt/athena/new-server default 40100
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
