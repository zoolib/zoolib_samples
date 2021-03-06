#include "NSApplicationDelegate_FlashHost.h"

#if ZCONFIG_SPI_Enabled(Cocoa)

#import <AppKit/NSScreen.h>

#include "FlashHost_Cocoa.h"

#include "zoolib/ZUtil_STL_vector.h"

#include "zoolib/ZCartesian_NS.h"
#include "zoolib/netscape/ZNetscape_Host_Cocoa.h"
#include "zoolib/netscape/ZNetscape_GuestFactory.h"

using namespace ZooLib;
using std::string;
using std::vector;
using net_em::FlashHost_Cocoa;

ZRef<ZNetscape::GuestFactory> sharedGF;

@implementation NSApplicationDelegate_FlashHost

-(id)init
	{
	[super init];
	return self;
	}

- (void) awakeFromNib
	{
	NSRect iBounds = sRect<NSRect>(0, 44, 400, 344);
	
	fWindow = [[NSWindow alloc]
		initWithContentRect:sFlippedY([[[NSScreen screens] objectAtIndex:0] frame].size.height, iBounds)
		styleMask:NSTitledWindowMask | NSClosableWindowMask | NSResizableWindowMask
		backing:NSBackingStoreBuffered
		defer:NO];

	[fWindow setAcceptsMouseMovedEvents:YES];

	NSView_NetscapeHost* theView = [[NSView_NetscapeHost alloc]init];
	[fWindow setContentView:theView];
	[fWindow makeKeyAndOrderFront:self];
	
	FlashHost_Cocoa* theHost = new FlashHost_Cocoa(sharedGF, theView);

	const string theMIME = "application/x-shockwave-flash";
	const string theURL = "http://www.adobe.com/content/dotcom/en/devnet/actionscript/samples/game_2/_jcr_content/articlecontentAdobe/generic/file.res/2_amoebas%5b1%5d.swf";
//	const string theURL = "http://www.em.net/fl_64/form1easy.swf";

	typedef ZNetscape::Host_Std::Param_t Param_t;
	vector<Param_t> theParams;
	theParams.push_back(Param_t("type", theMIME));
	theParams.push_back(Param_t("src", theURL));
	theParams.push_back(Param_t("quality", "high"));
	theParams.push_back(Param_t("wmode", "transparent"));
	theHost->CreateAndLoad(theURL, theMIME,
		ZUtil_STL::sFirstOrNil(theParams), theParams.size());
	}

@end
#endif // ZCONFIG_SPI_Enabled(Cocoa)
