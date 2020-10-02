## Requirements
### Windows
1. Visual Studio 2017
2. Perl - http://strawberryperl.com/


# Windows
Command Prompt
```
git clone --single-branch --branch 5.9 https://code.qt.io/qt/qt5.git
cd qt5
perl ./init-repository -force --module-subset=qtbase,qtdeclarative,qtgraphicaleffects,qtquickcontrols,qtquickcontrols2,qtmultimedia
```

Modify the following lines in `qtbase/mkspecs/common/msvc-desktop.conf`, changing `-MD` to `-MT`, and `-MDd` to `-MTd`
```
QMAKE_CFLAGS_RELEASE    = $$QMAKE_CFLAGS_OPTIMIZE -MD
QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO += $$QMAKE_CFLAGS_OPTIMIZE -Zi -MD
QMAKE_CFLAGS_DEBUG      = -Zi -MDd
```


Command Prompt
```
cp path-to-your-hifi-directory/tools/qt-builder/qt5vars.bat ./
mkdir qt-build
cd qt-build
..\qt5vars.bat
..\configure.bat
```

Download ssl-static.zip and unzip to ssl-static folder next to qt5 folder
`https://cdn-1.vircadia.com/eu-c-1/vircadia-public/dante/ssl-static-windows.zip`
remove config.opt in the build folder
copy over the config file from qt-builder
```
cp path-to-your-hifi-directory/tools/qt-builder/qt-lite-windows-config ./config.opt
```
delete config.cache

modify the config.opt file vaules `-L` and `-I` to point to the loaction of your openssl static lib

```
OPENSSL_LIBS=-lssleay32 -llibeay32
-I\path\to\openssl\include
-L\path\to\openssl\lib
```

Command Prompt
```
rem config.cache
config.status.bat
jom
jom install
```

# OSX
Must use Apple LLVM version 8.1.0 (clang-802.0.42)
Command Prompt
```
git clone --single-branch --branch 5.9 https://code.qt.io/qt/qt5.git
cd qt5
./init-repository -force --module-subset=qtbase,qtdeclarative,qtgraphicaleffects,qtquickcontrols,qtquickcontrols2,qtmultimedia,,
```

Command Prompt
```
mkdir qt-build
cd qt-build
../configure
```

Download ssl-static.zip and unzip to ssl-static folder next to qt5 folder
`https://cdn-1.vircadia.com/eu-c-1/vircadia-public/dante/openssl-static-osx.zip`
copy over the config file from qt-builder
```
cp path-to-your-hifi-directory/tools/qt-builder/qt-lite-osx-config ./config.opt
```
delete config.cache

modify the config.opt file vaules `-L` and `-I`to point to the loaction of your openssl static lib

```
OPENSSL_LIBS=-lssl -lcrypto
-L/path/to/openssl/lib
-I/path/to/openssl/include
```


Comand Prompt
```
rm config.cache
config.status
make
make install
```


#Building a static version of openssl on windows
https://wiki.openssl.org/index.php/Compilation_and_Installation#OpenSSL_1.0.2
follow the instructions in that link.

Keeping in mind that you need to use the non-dll commands (ex: 'nmake -f ms\ntdll.mak clean for the DLL target and nmake -f ms\nt.mak clean for static libraries.'
so you'd want to use 'ms\nt.mak'