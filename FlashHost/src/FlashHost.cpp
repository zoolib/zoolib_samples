#include "FlashHost.h"

#include "zoolib/ZLog.h"
#include "zoolib/ZStream_String.h"
#include "zoolib/ZTrail.h"
#include "zoolib/ZUnicode.h"
#include "zoolib/ZWinRegistry_Val.h"

#include <map>
#include <set>
#include <string.h> // For strstr

#if ZCONFIG_SPI_Enabled(Win) && !ZCONFIG(Compiler, CodeWarrior)
	#include <Shlobj.h> // For SHGetFolderPathW
#endif

namespace net_em {

using std::set;
using std::string;
using std::vector;

using namespace ZooLib;

using ZNetscape::NPObjectH;
using ZNetscape::NPVariantH;

// =================================================================================================
#pragma mark -
#pragma mark * spTryLoadGF

static ZRef<ZNetscape::GuestFactory> spTryLoadGF(ZQ<int> iEarliest, ZQ<int> iLatest, const string& iPath)
	{
	try { return ZNetscape::sMakeGuestFactory(iEarliest, iLatest, iPath); }
	catch (...)
		{}

	return null;
	}

// =================================================================================================
#pragma mark -
#pragma mark * Carbon helpers

#if ZCONFIG_SPI_Enabled(Carbon64)

static string spFindFolder(short iDomain, OSType iFolderType)
	{
	FSRef theFSRef;
	if (noErr == ::FSFindFolder(iDomain, iFolderType, kDontCreateFolder, &theFSRef))
		{
		char buffer[1024];
		::FSRefMakePath(&theFSRef, (UInt8*)buffer, 1024);
		return buffer;
		}
	return string();
	}

#endif // ZCONFIG_SPI_Enabled(Carbon64)

// =================================================================================================
#pragma mark -
#pragma mark * Windows helpers

#if ZCONFIG_SPI_Enabled(Win)

static string8 spTrailAsWin(const ZTrail& iTrail)
	{
	string8 result;
	if (iTrail.Count() > 0)
		{
		result = iTrail.At(0) + ":\\";
		if (iTrail.Count() > 1)
			result += iTrail.SubTrail(1).AsString("\\", "");
		}
	return result;
	}

static ZTrail spWinAsTrail(const string8& iWin)
	{
	ZTrail result = ZTrail("\\", "", "", iWin);
	if (result.Count())
		{
		const string firstComp = result.At(0);
		if (firstComp.size() > 1)
			{
			if (firstComp[1] == ':')
				result = firstComp.substr(0, 1) + result.SubTrail(1);
			}
		}
	return result;
	}

using ZWinRegistry::KeyRef;
using ZWinRegistry::Val;

static Val spTrail(const Val& iVal, const ZTrail& iTrail)
	{
	ZQ<Val> curVal = iVal;
	for (size_t x = 0; curVal && x < iTrail.Count(); ++x)
		curVal = curVal->GetKeyRef().QGet(iTrail.At(x));
	return curVal.DGet(Val());
	}

static ZQ<ZTrail> spGetTrailAt(const Val& iRoot, const ZTrail& iTrail)
	{
	if (ZQ<string16> theQ = spTrail(iRoot, iTrail).QGetString16())
		return spWinAsTrail(ZUnicode::sAsUTF8(*theQ));
	return null;
	}

static set<ZTrail> spGenerateCandidates()
	{
	set<ZTrail> result;
	{
	const KeyRef theKR = spTrail(KeyRef::sHKLM(), "Software/Adobe/Adobe Bridge").GetKeyRef();
	for (KeyRef::Index_t x = theKR.Begin(); x != theKR.End(); ++x)
		{
		if (ZQ<ZTrail> theTrail = spGetTrailAt(theKR.Get(x), "Installer/!InstallPath"))
			{
			result.insert(theTrail.Get() + "browser/plugins/npswf32.dll");
			result.insert(theTrail.Get() + "browser/plugins/npswf64.dll");
			}
		}
	}

	{
	const KeyRef theKR = spTrail(KeyRef::sHKLM(), "Software/Mozilla/Mozilla Firefox").GetKeyRef();
	for (KeyRef::Index_t x = theKR.Begin(); x != theKR.End(); ++x)
		{
		if (ZQ<ZTrail> theTrail = spGetTrailAt(theKR.Get(x), "Main/!Install Directory"))
			{
			result.insert(theTrail.Get() + "plugins/npswf32.dll");
			result.insert(theTrail.Get() + "plugins/npswf64.dll");
			}
		}
	}

	{
	const KeyRef theKR = spTrail(KeyRef::sHKLM(), "Software/Mozilla").GetKeyRef();
	for (KeyRef::Index_t x = theKR.Begin(); x != theKR.End(); ++x)
		{
		if (ZQ<ZTrail> theTrail = spGetTrailAt(theKR.Get(x), "extensions/!plugins"))
			{
			result.insert(theTrail.Get() + "npswf32.dll");
			result.insert(theTrail.Get() + "npswf64.dll");
			}
		}
	}

	for (int x = 0; x < 2; ++x)
		{
		const KeyRef curRoot = x ? KeyRef::sHKLM() : KeyRef::sHKCU();
		if (ZQ<ZTrail> theTrail =
			spGetTrailAt(curRoot, "Software/MozillaPlugins/@adobe.com/FlashPlayer/!Path"))
			{
			result.insert(theTrail.Get() + "npswf32.dll");
			result.insert(theTrail.Get() + "npswf64.dll");
			}
		}

	WCHAR path[1024];
	::GetSystemDirectoryW(path, countof(path));
	result.insert(spWinAsTrail(ZUnicode::sAsUTF8(path)) + "macromed/flash/npswf32.dll");
	result.insert(spWinAsTrail(ZUnicode::sAsUTF8(path)) + "macromed/flash/npswf64.dll");

	#if !ZCONFIG(Compiler, CodeWarrior)
		if (SUCCEEDED(::SHGetFolderPathW(0, CSIDL_PROGRAM_FILES_COMMON, NULL, SHGFP_TYPE_CURRENT, path)))
			{
			result.insert(spWinAsTrail(ZUnicode::sAsUTF8(path))
				+ "Adobe Air/Versions/1.0/Resources/npswf32.dll");
			result.insert(spWinAsTrail(ZUnicode::sAsUTF8(path))
				+ "Adobe Air/Versions/1.0/Resources/npswf64.dll");
			}
	#endif

	return result;
	}

static ZRef<ZNetscape::GuestFactory> spLoadWindows(ZQ<int> iEarliest, ZQ<int> iLatest)
	{
	const set<ZTrail> candidates = spGenerateCandidates();
	for (set<ZTrail>::const_iterator i = candidates.begin(); i != candidates.end(); ++i)
		{
		if (ZRef<ZNetscape::GuestFactory> theGF = spTryLoadGF(iEarliest, iLatest, spTrailAsWin(*i)))
			{
			if (ZLOGF(s, ePriority_Info))
				s << "Using file: " << i->AsString();
			return theGF;
			}
		}

	return null;
	}

#endif // ZCONFIG_SPI_Enabled(Win)

// =================================================================================================
#pragma mark -
#pragma mark * sLoadGF

ZRef<ZNetscape::GuestFactory> sLoadGF
	(ZQ<int> iEarliest, ZQ<int> iLatest, const string* iNativePaths, size_t iCount)
	{
	for (size_t x = 0; x < iCount; ++x)
		{
		const string& thePath = iNativePaths[x];
		if (thePath.size())
			{
			if (ZRef<ZNetscape::GuestFactory> theGF = spTryLoadGF(iEarliest, iLatest, thePath))
				return theGF;
			}
		}
	
	ZRef<ZNetscape::GuestFactory> theGF;

	#if ZCONFIG_SPI_Enabled(Win)

	if (ZRef<ZNetscape::GuestFactory> theGF = spLoadWindows(iEarliest, iLatest))
		return theGF;

	#endif // ZCONFIG_SPI_Enabled(Win)

	#if ZCONFIG_SPI_Enabled(Carbon64)

	if (ZRef<ZNetscape::GuestFactory> theGF =
		spTryLoadGF(iEarliest, iLatest, spFindFolder(kUserDomain, kInternetPlugInFolderType) + "/Flash Player.plugin"))
		{
		return theGF;
		}

	if (ZRef<ZNetscape::GuestFactory> theGF =
		spTryLoadGF(iEarliest, iLatest, spFindFolder(kLocalDomain, kInternetPlugInFolderType) + "/Flash Player.plugin"))
		{
		return theGF;
		}

	#endif // ZCONFIG_SPI_Enabled(Carbon64)

	return null;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ObjectH_Location

ObjectH_Location::ObjectH_Location(const string& iPageURL)
:	fPageURL(iPageURL)
	{}

ObjectH_Location::~ObjectH_Location()
	{}

bool ObjectH_Location::Imp_Invoke(
	const std::string& iName, const NPVariantH* iArgs, size_t iCount, NPVariantH& oResult)
	{
	if (iName == "__flash_getWindowLocation" || iName == "__flash_getTopLocation")
		{
		oResult = fPageURL;
		return true;
		}
	return false;
	}

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

#if defined(XP_MAC) || defined(XP_MACOSX) && !ZCONFIG_Is64Bit

FlashHost_WindowRef::FlashHost_WindowRef(
	ZRef<ZNetscape::GuestFactory> iGF,  bool iAllowCG, WindowRef iWindowRef)
:	Host_WindowRef(iGF, iAllowCG, iWindowRef)
	{}

FlashHost_WindowRef::~FlashHost_WindowRef()
	{}

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

bool FlashHost_WindowRef::Host_Evaluate(NPP npp,
	NPObject* obj, NPString* script, NPVariant* result)
	{
	((NPVariantH*)(result))->SetNull();
	return true;
	}

#endif // defined(XP_MAC) || defined(XP_MACOSX) && !ZCONFIG_Is64Bit

// =================================================================================================
#pragma mark -
#pragma mark * FlashHost_HIViewRef

#if defined(XP_MACOSX) && !ZCONFIG_Is64Bit

FlashHost_HIViewRef::FlashHost_HIViewRef(
	ZRef<ZNetscape::GuestFactory> iGF,  bool iAllowCG, HIViewRef iHIViewRef)
:	Host_HIViewRef(iGF, iAllowCG, iHIViewRef)
	{}

FlashHost_HIViewRef::~FlashHost_HIViewRef()
	{}

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

bool FlashHost_HIViewRef::Host_Evaluate(NPP npp,
	NPObject* obj, NPString* script, NPVariant* result)
	{
	((NPVariantH*)(result))->SetNull();
	return true;
	}

#endif // defined(XP_MACOSX) && !ZCONFIG_Is64Bit

// =================================================================================================
#pragma mark -
#pragma mark * FlashHost_Win

#if defined(XP_WIN)

FlashHost_Win::FlashHost_Win(ZRef<ZNetscape::GuestFactory> iGF, HWND iHWND)
:	Host_Win(iGF, iHWND)
	{}

FlashHost_Win::~FlashHost_Win()
	{}

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

bool FlashHost_Win::Host_Evaluate(NPP npp,
	NPObject* obj, NPString* script, NPVariant* result)
	{
	((NPVariantH*)(result))->SetNull();
	return true;
	}

#endif // defined(XP_WIN)

} // namespace net_em
