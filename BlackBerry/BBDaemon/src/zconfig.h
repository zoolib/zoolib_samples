#ifndef __zconfig__
#define __zconfig__ 1

#define ZCONFIG_NamespaceHack 0

#include "zoolib/zconfigd.h"

#include "zoolib/zconfigl.h"

//#define ZCONFIG_SPI_Avail__GDI 1
//#define ZCONFIG_SPI_Avail__X11 1
//#define ZCONFIG_SPI_Avail__QuickDraw 1

#ifndef MAC_OS_X_VERSION_MIN_REQUIRED
#	define MAC_OS_X_VERSION_MIN_REQUIRED MAC_OS_X_VERSION_10_2
#endif

#endif // __zconfig__
