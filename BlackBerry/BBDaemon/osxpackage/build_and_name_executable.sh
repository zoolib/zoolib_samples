#!/bin/sh
curdir=`pwd`
nice make -f ../build-gcc/makefile CONFIGURATION=Release
cp BBDaemon_Darwin_Release org.zoolib.BBDaemon
