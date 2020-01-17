#ATHENA=~/Athena rpmbuild --target x86_64 -bb athena-server.spec
Name:           athena-server
Version:        0.86.0_K1_20200112_7584429
Release:        4%{?dist}
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
install -d $RPM_BUILD_ROOT/etc/athena
install -m 0755 -t $RPM_BUILD_ROOT/etc/athena $ATHENA/source/rpm/new-server
install -d $RPM_BUILD_ROOT/usr/share/athena
install -m 0755 $ATHENA/build/assignment-client/assignment-client $RPM_BUILD_ROOT/usr/share/athena
install -m 0755 $ATHENA/build/domain-server/domain-server $RPM_BUILD_ROOT/usr/share/athena
#install -m 0755 $ATHENA/build/ice-server/ice-server $RPM_BUILD_ROOT/usr/share/athena
chrpath -d $RPM_BUILD_ROOT/usr/share/athena/*
install -d $RPM_BUILD_ROOT/usr/share/athena/lib
install -m 0644 -t $RPM_BUILD_ROOT/usr/share/athena/lib $ATHENA/qt5-install/lib/*.so.*.*.*
install -m 0644 -t $RPM_BUILD_ROOT/usr/share/athena/lib $ATHENA/build/ext/makefiles/quazip/project/lib/*.so.*.*.*
install -d $RPM_BUILD_ROOT/usr/lib/systemd/system
install -m 0644 -t $RPM_BUILD_ROOT/usr/lib/systemd/system $ATHENA/source/rpm/athena-assignment-client.service
install -m 0644 -t $RPM_BUILD_ROOT/usr/lib/systemd/system $ATHENA/source/rpm/athena-assignment-client@.service
install -m 0644 -t $RPM_BUILD_ROOT/usr/lib/systemd/system $ATHENA/source/rpm/athena-domain-server.service
install -m 0644 -t $RPM_BUILD_ROOT/usr/lib/systemd/system $ATHENA/source/rpm/athena-domain-server@.service
#install -m 0644 -t $RPM_BUILD_ROOT/usr/lib/systemd/system $ATHENA/source/rpm/athena-ice-server.service
#install -m 0644 -t $RPM_BUILD_ROOT/usr/lib/systemd/system $ATHENA/source/rpm/athena-ice-server@.service
install -m 0644 -t $RPM_BUILD_ROOT/usr/lib/systemd/system $ATHENA/source/rpm/athena-server.target
install -m 0644 -t $RPM_BUILD_ROOT/usr/lib/systemd/system $ATHENA/source/rpm/athena-server@.target
cp -a $ATHENA/source/domain-server/resources $RPM_BUILD_ROOT/usr/share/athena


%files
%license $ATHENA/source/LICENSE
/etc/athena
/usr/share/athena
/usr/lib/systemd/system


%changelog


%post
# create users
getent passwd athena >/dev/numm 2>&1 || useradd -r -c "Project Athena" -d /var/lib/athena -U -M athena
#getent group athena >/dev/null 2>&1 || groupadd -r athena

# create data folder
#mkdir -p /.local && chown root:athena /.local && chmod 775 /.local
#mkdir -p /.config && chown root:athena /.config && chmod 775 /.config
mkdir -p /var/lib/athena && chown athena:athena /var/lib/athena && chmod 775 /var/lib/athena

ldconfig -n /usr/share/athena/lib
if [ ! -d "/var/lib/athena/default" ]
then
	/etc/athena/new-server default 40100
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
