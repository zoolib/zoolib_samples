#ifndef __FlashHost__
#define __FlashHost__ 1
#include "zconfig.h"

#include "zoolib/netscape/ZNetscape_GuestFactory.h"
#include "zoolib/netscape/ZNetscape_Host_Cocoa.h"
#include "zoolib/netscape/ZNetscape_Host_Mac.h"
#include "zoolib/netscape/ZNetscape_Host_Win.h"

namespace net_em {

using namespace ZooLib;

ZRef<ZNetscape::GuestFactory> sLoadGF(uint64& oVersion, const std::string* iNativePaths, size_t iCount);

using ZNetscape::NPObjectH;

// =================================================================================================
#pragma mark -
#pragma mark * FlashHost_WindowRef

#if defined(XP_MAC) || defined(XP_MACOSX) && !ZCONFIG_Is64Bit

class FlashHost_WindowRef : public ZNetscape::Host_WindowRef
	{
public:
	FlashHost_WindowRef(
		ZRef<ZNetscape::GuestFactory> iGF, bool iAllowCG, WindowRef iWindowRef);
	virtual ~FlashHost_WindowRef();

// From Host_Std
	virtual NPError Host_GetURLNotify(NPP npp,
		const char* URL, const char* window, void* notifyData);

	virtual ZRef<NPObjectH> Host_GetWindowObject();
	};

#endif // defined(XP_MAC) || defined(XP_MACOSX) && !ZCONFIG_Is64Bit

// =================================================================================================
#pragma mark -
#pragma mark * FlashHost_HIViewRef

#if defined(XP_MACOSX) && !ZCONFIG_Is64Bit

class FlashHost_HIViewRef : public ZNetscape::Host_HIViewRef
	{
public:
	FlashHost_HIViewRef(
		ZRef<ZNetscape::GuestFactory> iGF,  bool iAllowCG, HIViewRef iHIViewRef);
	virtual ~FlashHost_HIViewRef();

// From Host_Std
	virtual NPError Host_GetURLNotify(NPP npp,
		const char* URL, const char* window, void* notifyData);

	virtual ZRef<NPObjectH> Host_GetWindowObject();
	};

#endif // defined(XP_MACOSX) && !ZCONFIG_Is64Bit

// =================================================================================================
#pragma mark -
#pragma mark * FlashHost_Win

#if defined(XP_WIN)

class FlashHost_Win : public ZNetscape::Host_Win
	{
public:
	FlashHost_Win(ZRef<ZNetscape::GuestFactory> iGF, HWND iHWND);
	virtual ~FlashHost_Win();

// From Host_Std
	virtual NPError Host_GetURLNotify(NPP npp,
		const char* URL, const char* window, void* notifyData);

	virtual ZRef<NPObjectH> Host_GetWindowObject();
	};

#endif // defined(XP_WIN)

} // namespace net_em

#endif // __FlashHost__
