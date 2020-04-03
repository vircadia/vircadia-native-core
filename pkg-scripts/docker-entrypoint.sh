#!/bin/sh
set -x


# In Prod, this may be configured with a GID already matching the container
# allowing the container to be run directly as Jenkins. In Dev, or on unknown
# environments, run the container as root to automatically correct docker
# group in container to match the docker.sock GID mounted from the host.
if [ -f /var/lib/athena/.local -a "$(id -u)" = "0" ]; then
	# realign gid
	THIS_ATHENA_GID=`ls -ngd /var/lib/athena/.local | cut -f3 -d' '`
	CUR_ATHENA_GID=`getent group athena | cut -f3 -d: || true`
	if [ ! -z "$THIS_ATHENA_GID" -a "$THIS_ATHENA_GID" != "$CUR_ATHENA_GID" ]; then
		groupmod -g ${THIS_ATHENA_GID} -o athena
	fi

	# realign pid
	THIS_ATHENA_PID=`ls -nd /var/lib/athena/.local | cut -f3 -d' '`
	CUR_ATHENA_PID=`getent passwd athena | cut -f3 -d: || true`
	if [ ! -z "$THIS_ATHENA_PID" -a "$THIS_ATHENA_PID" != "$CUR_ATHENA_PID" ]; then
		usermod -u ${THIS_ATHENA_PID} -o athena
	fi

	if ! groups athena | grep -q athena; then
		usermod -aG athena athena
	fi
fi

chmod 777 /dev/stdout
chmod 777 /dev/stderr

# continue with CMD
exec "$@"
