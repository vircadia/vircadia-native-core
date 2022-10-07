# Vircadia Server Packaging Scripts

Collection of scripts to create server distribution packages. Some of these scripts assume
use of the build script at https://github.com/vircadia/vircadia-builder, specifically that
the following directory structure exists:

```
base folder/
	source/		git checkout
	build/		result of cmake build
	qt5-install/	installed or built Qt5 installation
```

In this case the scripts assume that the current directory is the pkg-scripts folder inside of the source directory
and that the base folder can be reached by going to `../..`. This may not work if pkg-scripts is a symlink; adding an VIRCADIA=~/Vircadia to the beginning of the commandline will override where it looks for the base folder.

## Ubuntu
First build the project following the instruction [here](../BUILD_LINUX.md).
Then in the root of the project:
```
DEBVERSION="Semver e.g. 2021.1.3" DEBEMAIL="your-email@somewhere.com" DEBFULLNAME="Your Full Name" ./pkg-scripts/make-deb-server
```
This script assumes default configuration, with a build directory in the root of the project.
It will retrieve the current git commit date and hash and append it to your specified version. If successfully created, the .deb file will be available in the project root.

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
vircadia-ice-server.service
vircadia-server.target - used to launch/shutdown the prior services
vircadia-assignment-client@.service
vircadia-domain-server@.service
vircadia-ice-server@.service
vircadia-server@.target - used to launch/shutdown the prior services
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

### Ubuntu User Guide

Install the .deb package using `apt`:

```
apt update && apt install path/to/package.deb
```

The default services should be running after installation.

To check the status of services:
```
systemctl status vircadia-domain-server@default
systemctl status vircadia-assignment-client@default
systemctl status vircadia-ice-server@default
```
Similarly can use `systemctl restart` to restart if there is a problem.

To view logs in realtime:
```
journalctl -fu vircadia-domain-server@default
journalctl -fu vircadia-assignment-client@default
journalctl -fu vircadia-ice-server@default
```

To save all of today's logs to a file:
```
journalctl -u vircadia-domain-server@default --sicne today > domain-server.log
journalctl -u vircadia-assignment-client@default --since today > assignment-client.log
journalctl -u vircadia-ice-server@default --since today > ice-server.log
```

To download the logs (or any files) you can use `scp` in your local terminal:
```
scp root@server.com:/root/domain-server.log .
```

For convenience of working in the server, a terminal multiplexer can be used. Here are the basic usage instructions for [byobu](https://www.byobu.org).

Start byobu
```
byobu
```

General controls:
```
F2 - create a new terminal window
F3 - move to the left terminal window
F4 - move to the right terminal window
```

Controls for viewing realtime logs:
```
F7 - pause and browse terminal output
While browsing:
    - Ctrl+U - scroll up
    - Ctrl+D - scroll down
    - ? - search backwards
    - / - search forwards
    - n - go to next search match (depends on search direction)
    - N - go to previous search match (depends on search direction)
    - Enter - unpause the output
Shift+F7 - open the terminal output in a text editor
```
