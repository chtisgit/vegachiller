#!/bin/sh

if [ -z "$PREFIX" ]; then
	PREFIX="/usr"
fi

rm -f /usr/local/bin/vegachiller
rm -f /etc/systemd/system/vegachiller.service
rm -f /etc/apparmor.d/usr.local.bin.vegachiller
