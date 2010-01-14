#include "FlashHost.h"

#include "zoolib/ZLog.h"
#include "zoolib/ZStream_String.h"
#include "zoolib/ZTrail.h"
#include "zoolib/ZUnicode.h"
#include "zoolib/ZWinRegistry_Val.h"

#include <set>
#include <string.h> // For strstr

namespace net_em {

using std::string;
using std::set;

NAMESPACE_ZOOLIB_USING

using ZNetscape::NPObjectH;
using ZNetscape::NPVariantH;

// =================================================================================================
#pragma mark -
#pragma mark * Carbon helpers

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

static ZTrail sWinAsTrail(const string8& iWin)
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

static Val sTrail(const Val& iVal, const ZTrail& iTrail)
	{
	Val curVal = iVal;
	for (size_t x = 0; curVal && x < iTrail.Count(); ++x)
		curVal = curVal.GetKeyRef().Get(iTrail.At(x));
	return curVal;
	}

static ZTrail spGetTrailAt(const Val& iRoot, const ZTrail& iTrail)
	{
	string16 aPath;
	if (sTrail(iRoot, iTrail).QGetString16(aPath))
		return sWinAsTrail(ZUnicode::sAsUTF8(aPath));
	return ZTrail(false);
	}

static void spGenerateCandidates(set<ZTrail>& oTrails)
	{
	{
	const KeyRef theKR = sTrail(KeyRef::sHKLM(), "Software/Adobe/Adobe Bridge").GetKeyRef();
	for (KeyRef::Index_t x = theKR.Begin(); x != theKR.End(); ++x)
		{
		if (ZTrail theTrail = spGetTrailAt(theKR.Get(x), "Installer/!InstallPath"))
			oTrails.insert(theTrail + "browser/plugins/npswf32.dll");
		}
	}

	{
	const KeyRef theKR = sTrail(KeyRef::sHKLM(), "Software/Mozilla/Mozilla Firefox").GetKeyRef();
	for (KeyRef::Index_t x = theKR.Begin(); x != theKR.End(); ++x)
		{
		if (ZTrail theTrail = spGetTrailAt(theKR.Get(x), "Main/!Install Directory"))
			oTrails.insert(theTrail + "plugins/npswf32.dll");
		}
	}

	{
	const KeyRef theKR = sTrail(KeyRef::sHKLM(), "Software/Mozilla").GetKeyRef();
	for (KeyRef::Index_t x = theKR.Begin(); x != theKR.End(); ++x)
		{
		if (ZTrail theTrail = spGetTrailAt(theKR.Get(x), "extensions/!plugins"))
			oTrails.insert(theTrail + "npswf32.dll");
		}
	}

	for (int x = 0; x < 2; ++x)
		{
		const KeyRef curRoot = x ? KeyRef::sHKLM() : KeyRef::sHKCU();
		if (ZTrail theTrail =
			spGetTrailAt(curRoot, "Software/MozillaPlugins/@adobe.com/FlashPlayer/!Path"))
			{
			oTrails.insert(theTrail + "npswf32.dll");
			}
		}

	WCHAR path[1024];
	::GetSystemDirectoryW(path, countof(path));
	oTrails.insert(sWinAsTrail(ZUnicode::sAsUTF8(path)) + "macromed/flash/npswf32.dll");
	}

static uint64 spGetVersionNumber(const ZTrail& iTrail)
	{
	const string16 thePath = ZUnicode::sAsUTF16(spTrailAsWin(iTrail));
	DWORD dummy;
	if (DWORD theSize = ::GetFileVersionInfoSizeW(const_cast<WCHAR*>(thePath.c_str()), &dummy))
		{
		vector<char> buffer(theSize);
		if (::GetFileVersionInfoW(const_cast<WCHAR*>(thePath.c_str()), 0, theSize, &buffer[0]))
			{
			VS_FIXEDFILEINFO* info;
			UINT infoSize;
			if (::VerQueryValueW(&buffer[0], L"\\", (void**)&info, &infoSize)
				&& infoSize >= sizeof(VS_FIXEDFILEINFO))
				{
				return uint64(info->dwFileVersionLS) | uint64(info->dwFileVersionMS) << 32;
				}
			}
		}
	return 0;
	}

static ZTrail spGetBestWindowsTrail()
	{
	set<ZTrail> candidates;
	spGenerateCandidates(candidates);

	ZTrail bestCandidate(false);
	uint64  bestCandidateVer = 0;

	for (set<ZTrail>::const_iterator i = candidates.begin(); i != candidates.end(); ++i)
		{
		if (const uint64 theVer = spGetVersionNumber(*i))
			{
			if (!bestCandidate || bestCandidateVer < theVer)
				{
				bestCandidate = *i;
				bestCandidateVer = theVer;
				}
			}
		}

	if (bestCandidate)
		{
		if (const ZLog::S& s = ZLog::S(ZLog::ePriority_Info, "ZMain"))
			s << "Using " << bestCandidate.AsString();
		}
	return bestCandidate;
	}

#endif // ZCONFIG_SPI_Enabled(Win)

// =================================================================================================
#pragma mark -
#pragma mark * sLoadGF

static ZRef<ZNetscape::GuestFactory> spTryLoadGF(const string& iPath)
	{
	try
		{
		return ZNetscape::sMakeGuestFactory(iPath);
		}
	catch (...)
		{}
	return ZRef<ZNetscape::GuestFactory>();
	}

ZRef<ZNetscape::GuestFactory> sLoadGF(const std::string& iFlashLib)
	{
	if (iFlashLib.size())
		{
		if (ZRef<ZNetscape::GuestFactory> theGF = spTryLoadGF(iFlashLib))
			return theGF;
		}
	
	#if ZCONFIG_SPI_Enabled(Win)

	if (ZTrail theTrail = spGetBestWindowsTrail())
		{
		if (ZRef<ZNetscape::GuestFactory> theGF = spTryLoadGF(spTrailAsWin(theTrail)))
			return theGF;
		}

	#endif

	#if ZCONFIG_SPI_Enabled(Carbon)

	string thePath = spFindFolder(kUserDomain, kInternetPlugInFolderType) + "/Flash Player.plugin";
	if (ZRef<ZNetscape::GuestFactory> theGF = spTryLoadGF(thePath))
		return theGF;

	thePath = spFindFolder(kLocalDomain, kInternetPlugInFolderType) + "/Flash Player.plugin";
	if (ZRef<ZNetscape::GuestFactory> theGF = spTryLoadGF(thePath))
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
