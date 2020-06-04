#!/bin/sh
set -x


# In Prod, this may be configured with a GID already matching the container
# allowing the container to be run directly as Jenkins. In Dev, or on unknown
# environments, run the container as root to automatically correct docker
# group in container to match the docker.sock GID mounted from the host.
if [ -f /var/lib/vircadia/.local -a "$(id -u)" = "0" ]; then
	# realign gid
	THIS_VIRCADIA_GID=`ls -ngd /var/lib/vircadia/.local | cut -f3 -d' '`
	CUR_VIRCADIA_GID=`getent group vircadia | cut -f3 -d: || true`
	if [ ! -z "$THIS_VIRCADIA_GID" -a "$THIS_VIRCADIA_GID" != "$CUR_VIRCADIA_GID" ]; then
		groupmod -g ${THIS_VIRCADIA_GID} -o vircadia
	fi

	# realign pid
	THIS_VIRCADIA_PID=`ls -nd /var/lib/vircadia/.local | cut -f3 -d' '`
	CUR_VIRCADIA_PID=`getent passwd vircadia | cut -f3 -d: || true`
	if [ ! -z "$THIS_VIRCADIA_PID" -a "$THIS_VIRCADIA_PID" != "$CUR_VIRCADIA_PID" ]; then
		usermod -u ${THIS_VIRCADIA_PID} -o vircadia
	fi

	if ! groups vircadia | grep -q vircadia; then
		usermod -aG vircadia vircadia
	fi
fi

chmod 777 /dev/stdout
chmod 777 /dev/stderr

# continue with CMD
exec "$@"
