#!/bin/sh

if [ -e /Library/LaunchDaemons/org.zoolib.BBDaemon.plist ]; then
	launchctl unload /Library/LaunchDaemons/org.zoolib.BBDaemon.plist
fi

exit 0
