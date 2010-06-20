#include "FlashHost.h"

#if ! ZCONFIG_Is64Bit

#include "zoolib/ZLog.h"
#include "zoolib/ZStream_String.h"
#include "zoolib/ZTrail.h"
#include "zoolib/ZUnicode.h"
#include "zoolib/ZWinRegistry_Val.h"

#include <set>
#include <string.h> // For strstr

#if ZCONFIG_SPI_Enabled(Win) && !ZCONFIG(Compiler, CodeWarrior)
	#include <Shlobj.h>
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
	Val curVal = iVal;
	for (size_t x = 0; curVal && x < iTrail.Count(); ++x)
		curVal = curVal.GetKeyRef().Get(iTrail.At(x));
	return curVal;
	}

static ZTrail spGetTrailAt(const Val& iRoot, const ZTrail& iTrail)
	{
	string16 aPath;
	if (spTrail(iRoot, iTrail).QGetString16(aPath))
		return spWinAsTrail(ZUnicode::sAsUTF8(aPath));
	return ZTrail(false);
	}

static void spGenerateCandidates(set<ZTrail>& oTrails)
	{
	{
	const KeyRef theKR = spTrail(KeyRef::sHKLM(), "Software/Adobe/Adobe Bridge").GetKeyRef();
	for (KeyRef::Index_t x = theKR.Begin(); x != theKR.End(); ++x)
		{
		if (ZTrail theTrail = spGetTrailAt(theKR.Get(x), "Installer/!InstallPath"))
			oTrails.insert(theTrail + "browser/plugins/npswf32.dll");
		}
	}

	{
	const KeyRef theKR = spTrail(KeyRef::sHKLM(), "Software/Mozilla/Mozilla Firefox").GetKeyRef();
	for (KeyRef::Index_t x = theKR.Begin(); x != theKR.End(); ++x)
		{
		if (ZTrail theTrail = spGetTrailAt(theKR.Get(x), "Main/!Install Directory"))
			oTrails.insert(theTrail + "plugins/npswf32.dll");
		}
	}

	{
	const KeyRef theKR = spTrail(KeyRef::sHKLM(), "Software/Mozilla").GetKeyRef();
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
	oTrails.insert(spWinAsTrail(ZUnicode::sAsUTF8(path)) + "macromed/flash/npswf32.dll");

	#if !ZCONFIG(Compiler, CodeWarrior)
		if (SUCCEEDED(::SHGetFolderPathW(0, CSIDL_PROGRAM_FILES_COMMON, NULL, SHGFP_TYPE_CURRENT, path)))
			{
			oTrails.insert(spWinAsTrail(ZUnicode::sAsUTF8(path))
				+ "Adobe Air/Versions/1.0/Resources/npswf32.dll");
			}
	#endif
	}

static uint64 spGetVersionNumber(const string16& iPath)
	{
	DWORD dummy;
	if (DWORD theSize = ::GetFileVersionInfoSizeW(const_cast<WCHAR*>(iPath.c_str()), &dummy))
		{
		vector<char> buffer(theSize);
		if (::GetFileVersionInfoW(const_cast<WCHAR*>(iPath.c_str()), 0, theSize, &buffer[0]))
			{
			VS_FIXEDFILEINFO* info;
			UINT infoSize;
			if (::VerQueryValueW(&buffer[0], const_cast<WCHAR*>(L"\\"), (void**)&info, &infoSize)
				&& infoSize >= sizeof(VS_FIXEDFILEINFO))
				{
				return uint64(info->dwFileVersionLS) | uint64(info->dwFileVersionMS) << 32;
				}
			}
		}
	return 0;
	}

static uint64 spGetVersionNumber(const ZTrail& iTrail)
	{ return spGetVersionNumber(ZUnicode::sAsUTF16(spTrailAsWin(iTrail))); }

static ZTrail spGetBestWindowsTrail(uint64& oVersion)
	{
	set<ZTrail> candidates;
	spGenerateCandidates(candidates);

	ZTrail bestCandidate(false);
	oVersion = 0;

	for (set<ZTrail>::const_iterator i = candidates.begin(); i != candidates.end(); ++i)
		{
		if (const uint64 theVer = spGetVersionNumber(*i))
			{
			if (!bestCandidate || oVersion < theVer)
				{
				bestCandidate = *i;
				oVersion = theVer;
				}
			}
		}

	if (bestCandidate)
		{
		if (ZLOG(s, ePriority_Info, "FlashHost"))
			s << "Using file: " << bestCandidate.AsString();
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

ZRef<ZNetscape::GuestFactory> sLoadGF(uint64& oVersion, const string* iNativePaths, size_t iCount)
	{
	oVersion = 0;
	for (size_t x = 0; x < iCount; ++x)
		{
		const string& thePath = iNativePaths[x];
		if (thePath.size())
			{
			if (ZRef<ZNetscape::GuestFactory> theGF = spTryLoadGF(thePath))
				{
				#if ZCONFIG_SPI_Enabled(Win)
					oVersion = spGetVersionNumber(ZUnicode::sAsUTF16(thePath));
				#endif
				return theGF;
				}
			}
		}
	
	#if ZCONFIG_SPI_Enabled(Win)

	if (ZTrail theTrail = spGetBestWindowsTrail(oVersion))
		{
		if (ZRef<ZNetscape::GuestFactory> theGF = spTryLoadGF(spTrailAsWin(theTrail)))
			return theGF;
		}

	#endif

	#if ZCONFIG_SPI_Enabled(Carbon64)

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

#endif // ! ZCONFIG_Is64Bit
