# Vircadia Server Packaging Scripts

Collection of scripts to create server distribution packages. Most of these scripts assume
use of the build script at https://github.com/vircadia/vircadia-builder, specifically that
the following directory structure exists:

```
base folder/
	source/		git checkout
	build/		result of cmake build
	qt5-install/	installed or built Qt5 installation
```

These scripts assume that the current directory is the pkg-scripts folder inside of the source directory
and that the base folder can be reached by going to `../..`. This may not work if pkg-scripts is a symlink; adding an VIRCADIA=~/Vircadia to the beginning of the commandline will override where it looks for the base folder.

## Ubuntu
```
DEBVERSION="Semver e.g. 2021.1.3" DEBEMAIL="your-email@somewhere.com" DEBFULLNAME="Your Full Name" ./make-deb-server
```

This script will retrieve the current git commit date and hash and append it to your specified version.
It will attempt construct a .deb file in the pkg-scripts folder

## Amazon Linux 2
    
You will need to install `rpm-build` if you have not already.
```
sudo yum install rpm-build
```
Then, run the build script.
```
RPMVERSION="Semver e.g. 2021.1.3" ./make-rpm-server
```

This script will retrieve the current git commit date and hash and append it to your specified version.
It will attempt construct an .rpm file in the pkg-scripts folder

## Docker
```
./make-docker-server
```

This script will attempt to create a docker container.

## Results

### Binaries

The following directory structure is created for binaries:
```
/opt/vircadia - executables
/opt/vircadia/lib - private shared libraries required for executables
/opt/vircadia/resources - files required by domain-server administrative website
/opt/vircadia/plugins - files required by assignment-client, mainly for audio codecs
```

### Services

The following systemd services are installed in `/usr/lib/systemd/system`:
```
vircadia-assignment-client.service
vircadia-domain-server.service
vircadia-server.target - used to launch/shutdown the two prior services
vircadia-assignment-client@.service
vircadia-domain-server@.service
vircadia-server@.target - used to launch/shutdown the two prior services
```

The top three services in this list are the "normal" services that launch Vircadia
in the typical fashion. The bottom three services are "template" services designed
to permit multiple services to be installed and running on a single machine.

The script `/opt/vircadia/new-server serverName basePort` will do the necessary
setup for a new domain with the specified server name and port. Upon installation
the package will create and launch a domain named "default" at base port 40100.
The domain name here has nothing to do with the name people will use to find your
domain and has nothing to do with "place names", it is only used to locate the files
used to configure and run the domain on your server.

### Stored Files

The server stores its files in the following locations:
```
/var/lib/vircadia/.local - "unnamed" services (the default location for Vircadia servers)
/var/lib/vircadia/serverName - "named" (template) domains
/etc/opt/vircadia - environment variables when launching named domains
```
