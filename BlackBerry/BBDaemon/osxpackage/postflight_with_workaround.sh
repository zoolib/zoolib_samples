#!/bin/sh

# Work around (my) dumb mistake by setting appropriate values in
# org_zoolib_BlackBerryDaemon_tcp and org_zoolib_BlackBerryDaemon_local env vars.
if [ ! -e /etc/launchd.conf ] || ! grep org_zoolib_BlackBerryDaemon_tcp /etc/launchd.conf; then

	echo "setenv org_zoolib_BlackBerryDaemon_tcp _see_/etc/launchd.conf_for_source_of_this_env_var" >> /etc/launchd.conf
	echo "setenv org_zoolib_BlackBerryDaemon_local /tmp/org.zoolib.BlackBerryDaemon" >> /etc/launchd.conf

fi

launchctl load /Library/LaunchDaemons/org.zoolib.BBDaemon.plist
launchctl start org.zoolib.BBDaemon

exit 0
