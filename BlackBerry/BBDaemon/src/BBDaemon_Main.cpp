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

#include "ZLog.h"
#include "ZMain.h"
#include "ZNet_Internet.h"
#include "ZServer.h"
#include "ZStdIO.h"
#include "ZStream_POSIX.h"
#include "ZStrim_Stream.h"
#include "ZStrimmer_Streamer.h"
#include "ZUtil_Debug.h"

#include "ZBlackBerry_BBDevMgr.h"
#include "ZBlackBerry_OSXUSB.h"
#include "ZBlackBerryServer.h"

#if ZCONFIG(OS, MacOSX)
#	include <CoreFoundation/CFRunLoop.h>
#endif

const ZStrimW& serr = ZStdIO::strim_err;
const ZStrimW& sout = ZStdIO::strim_out;

// =================================================================================================
#pragma mark -
#pragma mark * ZMain

static void sHandler(void* iRefcon, ZRef<ZStreamerRWCon> iSRWCon)
	{
	ZBlackBerryServer* theBlackBerryServer = static_cast<ZBlackBerryServer*>(iRefcon);
	theBlackBerryServer->HandleRequest(iSRWCon);
	}

int ZMain(int argc, char **argv)
	{
	#if ZCONFIG(OS, Win32)
		// Get COM initialized ASAP.
		::CoInitializeEx(NULL, COINIT_MULTITHREADED);
//		::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	#endif

	// Install the standard debugging gear.
	ZUtil_Debug::sInstall();

	// And set up the strimmer to which log messages will be written, in this
	// case we're just dumping to stdout.
	ZRef<ZStrimmerW> theStrimmerW
	 	= new ZStrimmerW_Streamer_T<ZStrimW_StreamUTF8>
		(new ZStreamerW_T<ZStreamW_FILE>(stdout));
	ZUtil_Debug::sSetStrimmer(theStrimmerW);

	ZUtil_Debug::sSetLogPriority(ZLog::ePriority_Debug);


	if (const ZLog::S& s = ZLog::S(ZLog::ePriority_Info, "ZMain"))
		s.Writef("Starting");

	ZRef<ZBlackBerry::Manager> theManager;

	#if ZCONFIG(OS, MacOSX)

		// On Mac we instantiate a Manager that talks directly to USB.
		theManager = new ZBlackBerry::Manager_OSXUSB;

	#elif ZCONFIG(OS, Win32)

		// On Windows the Manager talks to the BBDevMgr COM server.
		theManager = new ZBlackBerry::Manager_BBDevMgr;

	#endif


	// Start listening on TCP port 17983
	if (ZRef<ZNetListener> theListener = ZNetListener_TCP::sCreateListener(17983, 5))
		{
		ZBlackBerryServer theBlackBerryServer(theManager);
		
		ZServer_Callback theNLServer(sHandler, &theBlackBerryServer);
		theNLServer.StartWaitingForConnections(theListener);

		#if ZCONFIG(OS, MacOSX)

			// The USB notification mechanism is handled by
			// callbacks made from a RunLoop.
			::CFRunLoopRun();

		#elif ZCONFIG(OS, Win32)

			// Because we're running in an MTA we don't *need* to run a message
			// loop, but that may not always be the case.
			while (true)
				{
				MSG theMSG;
				if (::GetMessageW(&theMSG, nil, 0, 0) == 0)
					break;

				::DispatchMessageW(&theMSG);
				}

		#else

			// On other platforms just quietly wait -- actually we don't have a
			// Manager for anything other than OSX or Windows right now, so
			// we won't get here.
			for (;;)
				ZThread::sSleep(1000);


		#endif
		}
	else
		{
		serr << "Couldn't open port 17983";
		return 1;
		}
	
	return 0;
	}
