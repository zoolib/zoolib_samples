#!/bin/sh

launchctl load /Library/LaunchDaemons/org.zoolib.BBDaemon.plist
launchctl start org.zoolib.BBDaemon

exit 0
