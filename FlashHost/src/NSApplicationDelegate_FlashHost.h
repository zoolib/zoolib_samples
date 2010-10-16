#ifndef __NSApplication_FlashHost__
#define __NSApplication_FlashHost__ 1
#include "zconfig.h"
#include "zoolib/ZCONFIG_SPI.h"

#if ZCONFIG_SPI_Enabled(Cocoa)

#include <AppKit/NSApplication.h>
#include <AppKit/NSWindow.h>

// =================================================================================================
#pragma mark -
#pragma mark * NSApplicationDelegate_FlashHost

@interface NSApplicationDelegate_FlashHost : NSObject
	{
@public
	NSWindow* fWindow;
	}

@end

#endif // ZCONFIG_SPI_Enabled(Cocoa)

#endif // __NSApplication_FlashHost__
