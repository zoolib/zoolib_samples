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
#include "zoolib/ZRefSafe.h"
#include "zoolib/ZStdIO.h"
#include "zoolib/ZStreamerCopier.h"
#include "zoolib/ZStreamerOpener.h"
#include "zoolib/ZStrimmer.h"
#include "zoolib/ZServer.h"
#include "zoolib/ZVal.h"
#include "zoolib/ZYad_ZooLib.h"
#include "zoolib/ZYad_ZooLibStrim.h"

#include "zoolib/ZCompat_NSObject.h"

NAMESPACE_ZOOLIB_USING

using std::pair;
using std::string;
using std::vector;

typedef ZVal_Z ZVal;
typedef ZList_Z ZList;
typedef ZMap_Z ZMap;

const ZStrimW& serr = ZStdIO::strim_err;
const ZStrimW& sout = ZStdIO::strim_out;

// =================================================================================================
#pragma mark -
#pragma mark * Responder_PF

class Responder_PF
:	public ZServer::Responder
,	public ZTaskOwner
	{
public:
	Responder_PF(ZRef<ZServer> iServer, ZRef<ZNetName> iNN);

// From ZTask via ZServer::Responder
	virtual void Kill();

// From ZServer::Responder
	virtual void Handle(ZRef<ZStreamerRW> iStreamerRW);

// From ZTaskOwner
	virtual void Task_Finished(ZRef<ZTask> iTask);

private:
	ZRef<ZStreamerRWCon> fLocalCon;
	ZRef<ZNetName> fNN;
	ZRefSafe<ZStreamerOpener> fOpener;
	ZRefSafe<ZStreamerCopier> fLocalToRemote;
	ZRefSafe<ZStreamerCopier> fRemoteToLocal;
	};

Responder_PF::Responder_PF(ZRef<ZServer> iServer, ZRef<ZNetName> iNN)
:	Responder(iServer)
,	fNN(iNN)
	{}

void Responder_PF::Kill()
	{
	if (ZRef<ZStreamerOpener> theOpener = fOpener)
		theOpener->Kill();

	if (ZRef<ZStreamerCopier> theLocalToRemote = fLocalToRemote)
		theLocalToRemote->Kill();

	if (ZRef<ZStreamerCopier> theRemoteToLocal = fRemoteToLocal)
		theRemoteToLocal->Kill();
	}

void Responder_PF::Handle(ZRef<ZStreamerRW> iStreamerRW)
	{
	fLocalCon = ZRefDynamicCast<ZStreamerRWCon>(iStreamerRW);

	if (fLocalCon)
		{
		fOpener = new ZStreamerOpener(this, fNN);
		sStartWorkerRunner(fOpener);
		return;
		}

	ZTask::pFinished();	
	}

void Responder_PF::Task_Finished(ZRef<ZTask> iTask)
	{
	if (ZRef<ZStreamerOpener> theOpener = fOpener)
		{
		if (iTask == theOpener)
			{
			if (ZRef<ZStreamerRWCon> remoteCon
				= ZRefDynamicCast<ZStreamerRWCon>(theOpener->GetStreamerRW()))
				{
				fLocalToRemote = new ZStreamerCopier(this, fLocalCon, remoteCon);
				sStartWorkerRunner(fLocalToRemote);
				fRemoteToLocal = new ZStreamerCopier(this, remoteCon, fLocalCon);
				sStartWorkerRunner(fRemoteToLocal);
				}		
			fOpener.Clear();
			}
		}

	if (iTask == fLocalToRemote)
		fLocalToRemote.Clear();

	if (iTask == fRemoteToLocal)
		fRemoteToLocal.Clear();

	if (!fOpener && !fRemoteToLocal && !fLocalToRemote)
		ZTask::pFinished();
	}

static ZRef<ZServer> sStartListener(const ZMap& iSpec)
	{
	int16 localPort;
	if (!iSpec.Get("l").QGetInt16(localPort))
		return ZRef<ZServer>();

	int16 remotePort;
	if (!iSpec.Get("rp").QGetInt16(remotePort))
		return ZRef<ZServer>();

	string remoteHost;
	if (!iSpec.Get("rh").QGetString(remoteHost))
		return ZRef<ZServer>();

	ZRef<ZNetListener> theListener = ZNetListener_TCP::sCreateListener(localPort, 5);
	if (!theListener)
		return ZRef<ZServer>();
	
	ZRef<ZNetName> remoteName = new ZNetName_Internet(remoteHost, remotePort);
	
	ZRef<ZServer> theServer = new ZServer_T<Responder_PF, ZRef<ZNetName> >(remoteName);
	theServer->StartListener(theListener);

	serr.Writef("Listening on port %d, forwarding to %s:%d\n",
		localPort, remoteHost.c_str(), remotePort);

	return theServer;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZMain

int ZMain(int argc, char **argv)
	{
	if (argc < 2)
		{
		serr
			<< "Usage: " << argv[0]
			<< " [{ l = int16(localPort); rh = \"remoteHost\"; rp = int16(yy); }]\n";

		return 1;
		}
		

	ZVal theVal;
	try
		{
		ZRef<ZStrimmerU> theStrimmerU = new ZStrimmerU_T<ZStrimU_String8>(argv[1]);
		if (ZRef<ZYadR> theYadR = ZYad_ZooLibStrim::sMakeYadR(theStrimmerU))
			theVal = sFromYadR(ZVal_ZooLib(), theYadR);
		else
			serr << "No param\n";
		}
	catch (...)
		{
		serr << "Bad tuple syntax\n";
		return 1;
		}

	vector<ZRef<ZServer> > theServers;
	if (const ZList theList = theVal.GetList())
		{
		for (size_t x = 0; x < theList.Count(); ++x)
			{
			if (const ZMap theMap = theList.Get(x).GetMap())
				{
				if (ZRef<ZServer> theServer = sStartListener(theMap))
					theServers.push_back(theServer);
				}
			}
		}
	else if (const ZMap theMap = theVal.GetMap())
		{
		if (ZRef<ZServer> theServer = sStartListener(theMap))
			theServers.push_back(theServer);
		}

	if (theServers.empty())
		{
		serr << "No valid forwarding specification found";
		return 1;
		}

	for (;;)
		ZThread::sSleep(10);
	
	return 0;
	}
