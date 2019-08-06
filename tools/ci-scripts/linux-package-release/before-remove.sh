#!/bin/bash
if systemctl is-active <%= service %>
then
    # Stop service that is running
    systemctl stop <%= service %>
fi
