#ifndef __FlashHost_Cocoa__
#define __FlashHost_Cocoa__ 1
#include "zconfig.h"

#include "zoolib/netscape/ZNetscape_GuestFactory.h"
#include "zoolib/netscape/ZNetscape_Host_Cocoa.h"

#if ZCONFIG_SPI_Enabled(Cocoa) && defined(__OBJC__)

namespace net_em {

using namespace ZooLib;

using ZNetscape::NPObjectH;

// =================================================================================================
#pragma mark -
#pragma mark * FlashHost_Cocoa

class FlashHost_Cocoa : public ZNetscape::Host_Cocoa
	{
public:
	FlashHost_Cocoa(ZRef<ZNetscape::GuestFactory> iGuestFactory, NSView_NetscapeHost* iView);
	virtual ~FlashHost_Cocoa();

// From Host_Std
	virtual NPError Host_GetURLNotify(NPP npp,
		const char* URL, const char* window, void* notifyData);

	virtual ZRef<NPObjectH> Host_GetWindowObject();

	virtual bool Host_Evaluate(NPP npp,
		NPObject* obj, NPString* script, NPVariant* result);
	};

} // namespace net_em

#endif // ZCONFIG_SPI_Enabled(Cocoa) && defined(__OBJC__)

#endif // __FlashHost_Cocoa__
