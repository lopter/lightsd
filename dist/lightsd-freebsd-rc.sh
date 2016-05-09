#!/bin/sh
#
# $FreeBSD: head/net/lightsg/files/lightsd.in 367807 2014-09-10 09:36:24Z ehaupt $
#

# PROVIDE: lightsd
# REQUIRE: LOGIN
# BEFORE:  securelevel
# KEYWORD: shutdown

# Add the following lines to /etc/rc.conf to enable `lightsd':
#
# lightsd_enable="YES"
# lightsd_flags="<set as needed>"
#
# See lightsg(1) for lightsd_flags
#

. /etc/rc.subr

name="lightsd"
rcvar=lightsd_enable

command="/usr/sbin/daemon"
start_precmd="lightsd_precmd"
pidfile="/var/run/lightsd/$name.pid"

# read configuration and set defaults
load_rc_config "$name"
: ${lightsd_enable="NO"}
: ${lightsd_user:="lightsd"}
: ${lightsd_group:="lightsd"}
: ${lightsd_listen:="localhost:23456"}
: ${lightsd_log:="info"}
: ${lightsd_flags:=""}


command_args="-P $pidfile -p $pidfile.cl -r /usr/local/bin/$name -l ${lightsd_listen} -v $lightsd_log -S $lightsd_flags 2>&1  "
#command_args="-l ${lightsd_listen} -v debug "

lightsd_precmd()
{
  # not a lot
}

run_rc_command "$1"
