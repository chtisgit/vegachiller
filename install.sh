#!/bin/sh

install --mode=755 vegachiller /usr/local/bin

mkdir -p /etc/systemd/system
install --mode=755 dist/vegachiller.service /etc/systemd/system

mkdir -p /etc/apparmor.d
install --mode=755 dist/usr.local.bin.vegachiller /etc/apparmor.d

which aa-enforce && aa-enforce /etc/apparmor.d/usr.local.bin.vegachiller
which systemctl && systemctl daemon-reload
