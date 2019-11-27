#!/bin/bash
if ! systemctl is-active <%= service %>
then
    systemctl start <%= service %>
fi
