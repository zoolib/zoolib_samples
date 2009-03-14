#!/bin/sh
curdir=`pwd`
nice make -f ../build-gcc/makefile CONFIGURATION=Release
sudo cp -f BBDaemon_Darwin_Release org.zoolib.BBDaemon
sudo chown root org.zoolib.BBDaemon
sudo chgrp wheel org.zoolib.BBDaemon
sudo chown root org.zoolib.BBDaemon.plist
sudo chgrp wheel org.zoolib.BBDaemon.plist
/Developer/Applications/Utilities/PackageMaker.app/Contents/MacOS/PackageMaker --doc BBDaemon.pmdoc

