#include "FlashHost.h"

#include "zoolib/ZStream_String.h"

#include <string.h> // For strstr

namespace net_em {

using std::string;

NAMESPACE_ZOOLIB_USING

using ZNetscape::NPObjectH;
using ZNetscape::NPVariantH;

// =================================================================================================
#pragma mark -
#pragma mark * sLoadGF

#if ZCONFIG_SPI_Enabled(Carbon)

static string spFindFolder(short iDomain, OSType iFolderType)
	{
	FSSpec theFSSpec;
	if (noErr != ::FindFolder(iDomain, iFolderType, false, &theFSSpec.vRefNum, &theFSSpec.parID))
		return string();

	theFSSpec.name[0] = 0;

	FSRef theFSRef;
	::FSpMakeFSRef(&theFSSpec, &theFSRef);

	char buffer[1024];
	::FSRefMakePath(&theFSRef, (UInt8*)buffer, 1024);

	return buffer;
	}

#endif // ZCONFIG_SPI_Enabled(Carbon)

static ZRef<ZNetscape::GuestFactory> spLoadGF(const string& iPath)
	{
	try
		{
		return ZNetscape::sMakeGuestFactory(iPath);
		}
	catch (...)
		{}
	return ZRef<ZNetscape::GuestFactory>();
	}

ZRef<ZNetscape::GuestFactory> sLoadGF()
	{
	
	#if ZCONFIG_SPI_Enabled(Win)

	if (ZRef<ZNetscape::GuestFactory> theGF = spLoadGF("C:\\Program Files\\Mozilla Firefox\\plugins\\NPSWF32.dll"))
		return theGF;

	if (ZRef<ZNetscape::GuestFactory> theGF = spLoadGF("C:\\Program Files (x86)\\Adobe\\Adobe Bridge CS4\\browser\\plugins\\NPSWF32.dll"))
		return theGF;

	#endif

	#if ZCONFIG_SPI_Enabled(Carbon)

	string thePath = spFindFolder(kUserDomain, kInternetPlugInFolderType) + "/Flash Player.plugin";
	if (ZRef<ZNetscape::GuestFactory> theGF = spLoadGF(thePath))
		return theGF;

	thePath = spFindFolder(kLocalDomain, kInternetPlugInFolderType) + "/Flash Player.plugin";
	if (ZRef<ZNetscape::GuestFactory> theGF = spLoadGF(thePath))
		return theGF;

	#endif

	return ZRef<ZNetscape::GuestFactory>();
	}


// =================================================================================================
#pragma mark -
#pragma mark * ObjectH_Location

class ObjectH_Location : public ZNetscape::ObjectH
	{
public:
	ObjectH_Location(const string& iPageURL);
	virtual ~ObjectH_Location();

	virtual bool Imp_GetProperty(const std::string& iName, NPVariantH& oResult);

private:
	const string fPageURL;
	};

ObjectH_Location::ObjectH_Location(const string& iPageURL)
:	fPageURL(iPageURL)
	{}

ObjectH_Location::~ObjectH_Location()
	{}

bool ObjectH_Location::Imp_GetProperty(const std::string& iName, NPVariantH& oResult)
	{
	if (iName == "location")
		{
		oResult = fPageURL;
		return true;
		}
	else
		{
		oResult = this;
		return true;
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * FlashHost_WindowRef

#if defined(XP_MAC) || defined(XP_MACOSX)

FlashHost_WindowRef::FlashHost_WindowRef(
	ZRef<ZNetscape::GuestFactory> iGF,  bool iAllowCG, WindowRef iWindowRef)
:	Host_WindowRef(iGF, iAllowCG, iWindowRef)
	{}

FlashHost_WindowRef::~FlashHost_WindowRef()
	{
	}

NPError FlashHost_WindowRef::Host_GetURLNotify(NPP npp,
	const char* iRelativeURL, const char* iTarget, void* notifyData)
	{
	if (iRelativeURL == strstr(iRelativeURL, "javascript:"))
		{
		const string theURL = fURL + "/__flashplugin_unique__";
		this->SendDataSync(notifyData, iRelativeURL, "text/html", ZStreamRPos_String(theURL));
		return NPERR_NO_ERROR;
		}

	return Host_WindowRef::Host_GetURLNotify(npp, iRelativeURL, iTarget, notifyData);
	}

ZRef<NPObjectH> FlashHost_WindowRef::Host_GetWindowObject()
	{
	string theURL = fURL.substr(0, fURL.find('?'));
	return new ObjectH_Location(theURL);
	}

#endif // defined(XP_MAC) || defined(XP_MACOSX)

// =================================================================================================
#pragma mark -
#pragma mark * FlashHost_WindowRef

#if defined(XP_MACOSX)

FlashHost_HIViewRef::FlashHost_HIViewRef(
	ZRef<ZNetscape::GuestFactory> iGF,  bool iAllowCG, HIViewRef iHIViewRef)
:	Host_HIViewRef(iGF, iAllowCG, iHIViewRef)
	{}

FlashHost_HIViewRef::~FlashHost_HIViewRef()
	{
	}

NPError FlashHost_HIViewRef::Host_GetURLNotify(NPP npp,
	const char* iRelativeURL, const char* iTarget, void* notifyData)
	{
	if (iRelativeURL == strstr(iRelativeURL, "javascript:"))
		{
		const string theURL = fURL + "/__flashplugin_unique__";
		this->SendDataSync(notifyData, iRelativeURL, "text/html", ZStreamRPos_String(theURL));
		return NPERR_NO_ERROR;
		}

	return Host_HIViewRef::Host_GetURLNotify(npp, iRelativeURL, iTarget, notifyData);
	}

ZRef<NPObjectH> FlashHost_HIViewRef::Host_GetWindowObject()
	{
	string theURL = fURL.substr(0, fURL.find('?'));
	return new ObjectH_Location(theURL);
	}

#endif // defined(XP_MACOSX)

// =================================================================================================
#pragma mark -
#pragma mark * FlashHost_Win

#if defined(XP_WIN)

FlashHost_Win::FlashHost_Win(ZRef<ZNetscape::GuestFactory> iGF, HWND iHWND)
:	Host_Win(iGF, iHWND)
	{}

FlashHost_Win::~FlashHost_Win()
	{
	}

NPError FlashHost_Win::Host_GetURLNotify(NPP npp,
	const char* iRelativeURL, const char* iTarget, void* notifyData)
	{
	if (iRelativeURL == strstr(iRelativeURL, "javascript:"))
		{
		const string theURL = fURL + "/__flashplugin_unique__";
		this->SendDataSync(notifyData, iRelativeURL, "text/html", ZStreamRPos_String(theURL));
		return NPERR_NO_ERROR;
		}

	return Host_Win::Host_GetURLNotify(npp, iRelativeURL, iTarget, notifyData);
	}

ZRef<NPObjectH> FlashHost_Win::Host_GetWindowObject()
	{
	string theURL = fURL.substr(0, fURL.find('?'));
	return new ObjectH_Location(theURL);
	}

#endif // defined(XP_WIN)

} // namespace net_em
