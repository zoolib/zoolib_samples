/* -------------------------------------------------------------------------------------------------
Copyright (c) 2007 Andrew Green
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

#include "zoolib/ZNet_Internet.h"
#include "zoolib/ZServer.h"
#include "zoolib/ZStdIO.h"
#include "zoolib/ZThreadSimple.h"
#include "zoolib/ZTuple.h"
#include "zoolib/ZYad_ZooLib.h"
#include "zoolib/ZYad_ZooLibStrim.h"

NAMESPACE_ZOOLIB_USING

using std::pair;
using std::string;
using std::vector;

const ZStrimW& serr = ZStdIO::strim_err;
const ZStrimW& sout = ZStdIO::strim_out;

// =================================================================================================
#pragma mark -
#pragma mark * ZMain

static void sCopyFromTo(const ZStreamR& r, const ZStreamWCon& w)
	{
	r.CopyAllTo(w);
	w.SendDisconnect();
	}

typedef pair<const ZStreamR*, const ZStreamWCon*> StreamPair_t;

static void sCopyFromTo(StreamPair_t iParam)
	{ sCopyFromTo(*iParam.first, *iParam.second); }

struct RemoteInfo
	{
	string fHost;
	int16 fPort;
	};

static void sHandler(void* iRefcon, ZRef<ZStreamerRWCon> iConnection)
	{
	RemoteInfo* theRI = static_cast<RemoteInfo*>(iRefcon);

	if (ZRef<ZNetEndpoint> theEP
		= ZRefDynamicCast<ZNetEndpoint>(iConnection))
		{
		if (ZRef<ZNetAddress_Internet> theNA
			= ZRefDynamicCast<ZNetAddress_Internet>(theEP->GetRemoteAddress()))
			{
			const ip_addr clientIP = theNA->GetHost();
			serr.Writef("Connection from %d.%d.%d.%d:%d, ",
				0xFF & (clientIP >> 24),
				0xFF & (clientIP >> 16),
				0xFF & (clientIP >> 8),
				0xFF & clientIP,
				theNA->GetPort());

			serr.Writef("forwarding to %s:%d\n",
				theRI->fHost.c_str(), theRI->fPort);
			}
		}

	if (ZRef<ZStreamerRWCon> remoteCon = ZNetName_Internet(theRI->fHost, theRI->fPort).Connect())
		{
		StreamPair_t localToRemote(&iConnection->GetStreamR(), &remoteCon->GetStreamWCon());

		(new ZThreadSimple<StreamPair_t>(sCopyFromTo, localToRemote))->Start();

		sCopyFromTo(remoteCon->GetStreamR(), iConnection->GetStreamWCon());
		}
	}

static bool sStartListener(const ZTuple& iSpec)
	{
	int16 localPort;
	if (!iSpec.GetInt16("l", localPort))
		return false;

	int16 remotePort;
	if (!iSpec.GetInt16("rp", remotePort))
		return false;

	string remoteHost;
	if (!iSpec.GetString("rh", remoteHost))
		return false;

	ZRef<ZNetListener> theListener = ZNetListener_TCP::sCreateListener(localPort, 5);
	if (!theListener)
		return false;

	RemoteInfo* theRI = new RemoteInfo;
	theRI->fHost = remoteHost;
	theRI->fPort = remotePort;
	
	ZServer* theServer = new ZServer_Callback(sHandler, theRI);
	theServer->StartWaitingForConnections(theListener);

	serr.Writef("Listening on port %d, forwarding to %s:%d\n",
		localPort, remoteHost.c_str(), remotePort);

	return true;
	}

int ZMain(int argc, char **argv)
	{
	if (argc < 2)
		{
		serr
			<< "Usage: " << argv[0]
			<< " [{ l = int16(localPort); rh = \"remoteHost\"; rp = int16(yy); }]\n";

		return 1;
		}
		

	ZTValue theTValue;
	try
		{
		ZStrimU_String8 theStrimU(argv[1]);
		if (ZRef<ZYadR> theYadR = ZYad_ZooLibStrim::sMakeYadR(theStrimU))
			theTValue = ZYad_ZooLib::sFromYadR(theYadR);
		else
			serr << "No param\n";
		}
	catch (...)
		{
		serr << "Bad tuple syntax\n";
		return 1;
		}

	bool anyStarted = false;
	if (theTValue.TypeOf() == eZType_Vector)
		{
		const vector<ZTValue>& theVector = theTValue.GetVector();
		for (vector<ZTValue>::const_iterator i = theVector.begin();
			i != theVector.end(); ++i)
			{
			if (const ZTuple theTuple = i->GetTuple())
				{
				if (sStartListener(theTuple))
					anyStarted = true;
				}
			}
		}
	else if (const ZTuple theTuple = theTValue.GetTuple())
		{
		if (sStartListener(theTuple))
			anyStarted = true;
		}

	if (!anyStarted)
		{
		serr << "No valid forwarding specification found";
		return 1;
		}

	for (;;)
		ZThread::sSleep(10000);
	
	return 0;
	}
