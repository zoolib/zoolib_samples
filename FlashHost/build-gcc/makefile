# default to a release build
CONFIGURATION?=Release

ZOOLIB_DIR ?= ../../../zoolib

include $(ZOOLIB_DIR)/tools/zmake/zmake.prefix

PROJECT_DIR = ../src

include ${PROJECT_DIR}/realmake

include $(ZOOLIB_DIR)/tools/zmake/zmake.suffix
