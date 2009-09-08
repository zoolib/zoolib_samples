/* -------------------------------------------------------------------------------------------------
Copyright (c) 2008 Andrew Green
http://www.zoolib.org

Permission is hereby granted, free of charge, to any person obtaining a copy of this software
and associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software
is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES
OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------------------------- */

#include "zoolib/ZCommandLine.h"
#include "zoolib/ZFile.h"
#include "zoolib/ZLog.h"
#include "zoolib/ZMain.h"
#include "zoolib/ZNet_Internet.h"
#include "zoolib/ZNet_Local.h"
#include "zoolib/ZServer.h"
#include "zoolib/ZStdIO.h"
#include "zoolib/ZStream_POSIX.h"
#include "zoolib/ZStrim_Stream.h"
#include "zoolib/ZStrimmer_Streamer.h"
#include "zoolib/ZUtil_Debug.h"

#include "zoolib/blackberry/ZBlackBerry_BBDevMgr.h"
#include "zoolib/blackberry/ZBlackBerry_OSXUSB.h"
#include "zoolib/blackberry/ZBlackBerryServer.h"

#if ZCONFIG_SPI_Enabled(MacOSX)
#	include <CoreFoundation/CFRunLoop.h>
#endif

NAMESPACE_ZOOLIB_USING

using std::string;

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

	CommandLine()
	:	fHelp("--help", "Print this message and exit"),
		fLogPriority("-p", "Priority below which log messages are discarded", 5),
		fLogFile("--logfile", "File to which log messages are written", "-")
		{}
	};
} // anonymous namespace

// =================================================================================================
#pragma mark -
#pragma mark * Helper functions

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
#pragma mark * Responder_BB

class Responder_BB
:	public ZServer::Responder
	{
public:
	Responder_BB(ZRef<ZServer> iServer, ZBlackBerryServer* iBBServer);

// From ZServer::Responder
	virtual void Handle(ZRef<ZStreamerRW> iStreamerRW);

private:
	ZBlackBerryServer* fBBServer;
	};

Responder_BB::Responder_BB(ZRef<ZServer> iServer, ZBlackBerryServer* iBBServer)
:	Responder(iServer)
,	fBBServer(iBBServer)
	{}

void Responder_BB::Handle(ZRef<ZStreamerRW> iStreamerRW)
	{
	if (ZRef<ZStreamerRWCon> theSRWCon
		= ZRefDynamicCast<ZStreamerRWCon>(iStreamerRW))
		{
		fBBServer->HandleRequest(theSRWCon);
		}
	ZTask::pFinished();	
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZMain

int _CRT_MT = 1;
int ZMain(int argc, char **argv)
	{
	const ZStrimW& serr = ZStdIO::strim_err;

	#if ZCONFIG_SPI_Enabled(Win)
		// Get COM initialized ASAP.
		::CoInitializeEx(NULL, COINIT_MULTITHREADED);
	#endif

	// Install the standard debugging gear.
	ZUtil_Debug::sInstall();

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
		serr << "org.zoolib.BBDaemon, 2009-03-12\n(c) 2009 Andrew Green\n";
		cmd.WriteUsageExtended(serr);
		return 0;
		}


	ZUtil_Debug::sSetLogPriority(cmd.fLogPriority());

	if (ZRef<ZStreamerW> theStreamerW = sOpenStreamer(cmd.fLogFile()))
		{
		ZRef<ZStrimmerW> theStrimmerW = new ZStrimmerW_Streamer_T<ZStrimW_StreamUTF8>(theStreamerW);
		ZUtil_Debug::sSetStrimmer(theStrimmerW);
		}

	if (ZLOG(s, eInfo, "ZMain"))
		s.Writef("Starting");

	ZRef<ZBlackBerry::Manager> theManager;

	#if ZCONFIG_API_Enabled(BlackBerry_OSXUSB)

		// On Mac we instantiate a Manager that talks directly to USB.
		theManager = new ZBlackBerry::Manager_OSXUSB(::CFRunLoopGetCurrent(), true);

	#elif ZCONFIG_API_Enabled(BlackBerry_BBDevMgr)

		// On Windows the Manager talks to the BBDevMgr COM server.
		theManager = new ZBlackBerry::Manager_BBDevMgr;

	#endif

	if (!theManager)
		return 1;

	ZBlackBerryServer theBlackBerryServer(theManager);

	ZRef<ZNetListener> theListener_Local, theListener_TCP;

	#if ZCONFIG_SPI_Enabled(MacOSX)
		// Listen on domain socket
		theListener_Local = ZNetListener_Local::sCreate("/tmp/org.zoolib.BlackBerryDaemon", 5);
	#else
		// Start listening on TCP port 17983
		theListener_TCP = ZNetListener_TCP::sCreate(17983, 5);
	#endif

	ZRef<ZServer> server_TCP;
	if (theListener_TCP)
		{
		server_TCP = new ZServer_T<Responder_BB, ZBlackBerryServer*>(&theBlackBerryServer);
		server_TCP->StartListener(theListener_TCP);
		}

	ZRef<ZServer> server_Local;
	if (theListener_Local)
		{
		server_Local = new ZServer_T<Responder_BB, ZBlackBerryServer*>(&theBlackBerryServer);
		server_Local->StartListener(theListener_Local);
		}
		

	#if ZCONFIG_SPI_Enabled(MacOSX)

		// The USB notification mechanism is handled by
		// callbacks made from a RunLoop.
		::CFRunLoopRun();

	#elif ZCONFIG_SPI_Enabled(Win)

		// Because we're running in an MTA we don't *need* to run a message
		// loop, but that may not always be the case.
		while (true)
			{
			MSG theMSG;
			if (::GetMessageW(&theMSG, nullptr, 0, 0) == 0)
				break;

			::DispatchMessageW(&theMSG);
			}

	#else

		// On other platforms just quietly wait -- actually we don't have a
		// Manager for anything other than OSX or Windows right now, so
		// we won't get here.
		for (;;)
			ZThreadImp::sSleep(1);

	#endif
	
	return 0;
	}
