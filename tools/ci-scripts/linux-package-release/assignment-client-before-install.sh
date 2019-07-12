#!/bin/bash
dpkg -s <%= service %> &>/dev/null
if [ $? -ne 0 ]
then
    if systemctl is-active <%= service %>
    then
        # Stop service that is running
        systemctl stop <%= service %>
    fi
fi

[ $(getent group hifi) ] || groupadd hifi
id -u hifi &>/dev/null || useradd hifi -g hifi --system --no-create-home
mkdir -p /.local && chown root:hifi /.local && chmod 775 /.local
mkdir -p /.config && chown root:hifi /.config && chmod 775 /.config
mkdir -p /var/log/hifi && chown root:hifi /var/log/hifi && chmod 775 /var/log/hifi
mkdir -p /usr/share/hifi/assignment-client && chown root:hifi /usr/share/hifi/assignment-client && chmod 775 /usr/share/hifi/assignment-client
