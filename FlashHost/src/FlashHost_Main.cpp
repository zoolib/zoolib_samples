#include "zoolib/ZCommandLine.h"
#include "zoolib/ZFile.h"
#include "zoolib/ZMain.h"
#include "zoolib/ZNet_Internet_Socket.h"
#include "zoolib/ZStream_POSIX.h"
#include "zoolib/ZStdIO.h"
#include "zoolib/ZStrim_Stream.h"
#include "zoolib/ZStrimmer_Streamer.h"
#include "zoolib/ZUtil_CFType.h"
#include "zoolib/ZUtil_Debug.h"
#include "zoolib/ZUtil_STL.h"
#include "zoolib/ZWND.h"

#include "zoolib/netscape/ZNetscape_Host_Std.h"
#include "zoolib/netscape/ZNetscape_GuestFactory.h"

#include "FlashHost.h"

#if ZCONFIG_SPI_Enabled(Carbon)
	#include ZMACINCLUDE3(Carbon,HIToolbox,CarbonEvents.h)
	#include ZMACINCLUDE3(Carbon,HIToolbox,MacWindows.h)
#endif

using std::string;
using std::vector;

NAMESPACE_ZOOLIB_USING

ZNetscape::HostMeister_Std sHostMeister;

// =================================================================================================
#pragma mark -
#pragma mark * Helpers

static ZRef<ZStreamerW> sOpenStreamer(const string& iPath)
	{
	if (iPath == "-")
		{
		return new ZStreamerW_T<ZStreamW_FILE>(stdout);
		}
	else
		{
		if (ZRef<ZStreamerWPos> theStreamerWPos = ZFileSpec(iPath).CreateWPos(true, false))
			{
			const ZStreamWPos& theStreamWPos = theStreamerWPos->GetStreamWPos();
			theStreamWPos.SetPosition(theStreamWPos.GetSize());
			return theStreamerWPos;
			}
		}
	return ZRef<ZStreamerW>();
	}

// =================================================================================================
#pragma mark -
#pragma mark * sBuildUI_Carbon

#if ZCONFIG_SPI_Enabled(Carbon)

static ZNetscape::Host_Std* sBuildUI_Carbon(
	ZRef<ZNetscape::GuestFactory> iGF, bool iWindowRef, bool iAllowCG, bool iCompositing)
	{
	WindowClass theWC = kDocumentWindowClass;
	WindowAttributes theWA = 0;
	if (iCompositing)
		theWA |= kWindowCompositingAttribute;
	theWA |= kWindowStandardHandlerAttribute;
	theWA |= kWindowLiveResizeAttribute;
	theWA |= kWindowResizableAttribute;
	ZGRectf theBounds(400, 400);
	if (iAllowCG)
		theBounds.origin.x += 400;
	if (iWindowRef)
		theBounds.origin.x += 800;
	if (iCompositing)
		theBounds.origin.y += 440;

	theBounds.origin.y += 44;

	Rect theQDBounds = theBounds;

	WindowRef theWindowRef;
	::CreateNewWindow(theWC, theWA, &theQDBounds, &theWindowRef);

	ZNetscape::Host_Std* theHost = nil;
	if (iWindowRef)
		{
		theHost = new net_em::FlashHost_WindowRef(iGF, iAllowCG, theWindowRef);
		}
	else
		{
		HIViewRef contentViewRef;
		::HIViewFindByID(::HIViewGetRoot(theWindowRef), kHIViewWindowContentID, &contentViewRef);
		theHost = new net_em::FlashHost_HIViewRef(iGF, iAllowCG, contentViewRef);
		}

	string title;
	if (iWindowRef)
		title += "WindowRef";
	else
		title += "HIView";

	if (iAllowCG)
		title += "/CG";

	if (iCompositing)
		title += "/Compositing";

	::SetWindowTitleWithCFString(theWindowRef, ZUtil_CFType::sString(title));

	::ShowWindow(theWindowRef);
	::BringToFront(theWindowRef);

	return theHost;
	}

#endif // ZCONFIG_SPI_Enabled(Carbon)

// =================================================================================================
#pragma mark -
#pragma mark * sBuildUI_Win

#if ZCONFIG_SPI_Enabled(Win)

static ZNetscape::Host_Std* sBuildUI_Win(ZRef<ZNetscape::GuestFactory> iGF)
	{
	ZWNDW* mainWND = new ZWNDW(DefWindowProcW);
	DWORD theStyle = WS_SYSMENU | WS_POPUP | WS_BORDER
		| WS_DLGFRAME | WS_THICKFRAME | WS_CLIPCHILDREN | WS_THICKFRAME;

	mainWND->Create(nullptr, theStyle);

	ZNetscape::Host_Std* theHost = new net_em::FlashHost_Win(iGF, mainWND->GetHWND());

	bool result = ::SetWindowPos(mainWND->GetHWND(), HWND_TOP, 20, 30, 400, 400, SWP_SHOWWINDOW);

	return theHost;	
	}

#endif // ZCONFIG_SPI_Enabled(Win)

// =================================================================================================
#pragma mark -
#pragma mark * CommandLine

namespace ZANONYMOUS {
class CommandLine : public ZCommandLine
	{
public:
	Boolean fHelp;
	Int64 fLogPriority;
	String fLogFile;
	String fURL;
	CommandLine()
	:	fHelp("--help", "Print this message and exit"),
		fLogPriority("-p", "Priority below which log messages should be discarded", ZLog::eInfo),
		fLogFile("--logfile", "Log: name of file to write log messages to", "-"),
		fURL("--url", "URL from which to load an swf",
			"http://www.adobe.com/devnet/flash/samples/game_2/2_amoebas.swf")
		{}
	};
} // anonymous namespace

// =================================================================================================
#pragma mark -
#pragma mark * ZMain

int ZMain(int argc, char** argv)
	{
	const ZStrimW& serr = ZStdIO::strim_err;

	ZUtil_Debug::sInstall();

	// Strip out the -psnXXX argument
	char** source = argv;
	char** dest = argv;
	for (size_t x = 0; x < argc; ++x)
		{
		if (strlen(*source) >= 4 && 0 == strncmp(*source, "-psn", 4))
			++source;
		else
			*dest++ = *source++;
		}

	argc = dest - argv;

	CommandLine cmd;
	if (!cmd.Parse(serr, argc, argv) && !cmd.fHelp())
		{
		serr << "Usage: " << argv[0] << " " << cmd << "\n";
		for (size_t x = 0; x < argc; ++x)
			{
			serr.Writef("%d: ", x);
			serr << argv[x];
			serr << "\n";
			}
		return 1;
		}

	if (cmd.fHelp())
		{
		serr << "Usage: " << argv[0] << " " << cmd << "\n";
		cmd.WriteUsageExtended(serr);
		return 0;
		}

	#if ZCONFIG_API_Enabled(Internet_Socket)
		ZNetListener_TCP_Socket::sEnableFastCancellation();
	#endif

	ZUtil_Debug::sSetLogPriority(cmd.fLogPriority());

	ZRef<ZStrimmerW> theStrimmerW;

	if (ZRef<ZStreamerW> theStreamerW = sOpenStreamer(cmd.fLogFile()))
		theStrimmerW = new ZStrimmerW_Streamer_T<ZStrimW_StreamUTF8>(theStreamerW);

	ZUtil_Debug::sSetStrimmer(theStrimmerW);

	if (const ZLog::S& s = ZLog::S(ZLog::ePriority_Info, "ZMain"))
		s.Writef("Starting");

	ZRef<ZNetscape::GuestFactory> theGF = net_em::sLoadGF();

	const string theMIME = "application/x-shockwave-flash";
	const string theURL = cmd.fURL();

	typedef ZNetscape::Host_Std::Param_t Param_t;
	vector<Param_t> theParams;
	theParams.push_back(Param_t("type", theMIME));
	theParams.push_back(Param_t("src", theURL));
	theParams.push_back(Param_t("quality", "high"));

	if (ZCONFIG_SPI_Enabled(Win))
		{
		// On windows we'd need to double-buffer to make transparent
		// plugins redraw without flicker.
		theParams.push_back(Param_t("wmode", "opaque"));
		}
	else
		{
		theParams.push_back(Param_t("wmode", "transparent"));
		}

	#if ZCONFIG_SPI_Enabled(Carbon)
		for (int useWindowRef = 0; useWindowRef < 2; ++useWindowRef)
			{
			for (int useAllowCG = 0; useAllowCG < 2; ++useAllowCG)
				{
				for (int useCompositing = 0; useCompositing < 2; ++useCompositing)
					{
					if (useCompositing && useWindowRef)
						continue;
					if (!useCompositing && !useWindowRef)
						continue;

					ZNetscape::Host_Std* theFlashHost
						= sBuildUI_Carbon(theGF, useWindowRef, useAllowCG, useCompositing);
					theFlashHost->CreateAndLoad(theURL, theMIME,
						ZUtil_STL::sFirstOrNil(theParams), theParams.size());
					}
				}
			}
	#endif // ZCONFIG_SPI_Enabled(Carbon)

	#if (ZCONFIG_SPI_Enabled(Win))
		ZNetscape::Host_Std* theFlashHost
			= sBuildUI_Win(theGF);
		theFlashHost->CreateAndLoad(theURL, theMIME,
			ZUtil_STL::sFirstOrNil(theParams), theParams.size());
	#endif // (ZCONFIG_SPI_Enabled(Win))


	#if ZCONFIG_SPI_Enabled(Carbon)

		::RunApplicationEventLoop();

	#elif ZCONFIG_SPI_Enabled(Win)

		while (true)
			{
			MSG theMSG;
			if (::GetMessageW(&theMSG, nullptr, 0, 0) == 0)
				break;

			::DispatchMessageW(&theMSG);
			}

	#endif

	if (const ZLog::S& s = ZLog::S(ZLog::ePriority_Info, "ZMain"))
		s.Writef("Finished");

	return 0;
	}
