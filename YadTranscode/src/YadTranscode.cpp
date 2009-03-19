/* -------------------------------------------------------------------------------------------------
Copyright (c) 2009 Andrew Green
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
#include "zoolib/ZStdIO.h"
#include "zoolib/ZStream_Buffered.h"
#include "zoolib/ZStream_POSIX.h"
#include "zoolib/ZStrim_Stream.h"
#include "zoolib/ZStrimW_ML.h"
#include "zoolib/ZYad_Bencode.h"
#include "zoolib/ZYad_JSON.h"
#include "zoolib/ZYad_ML.h"
#include "zoolib/ZYad_XMLPList.h"
#include "zoolib/ZYad_ZooLibStream.h"
#include "zoolib/ZYad_ZooLibStrim.h"

NAMESPACE_ZOOLIB_USING

using std::string;

const ZStrimW& serr = ZStdIO::strim_err;
const ZStrimW& sout = ZStdIO::strim_out;
const ZStreamR& stream_in = ZStdIO::stream_in;

// =================================================================================================
#pragma mark -
#pragma mark * CommandLine

namespace ZANONYMOUS {
class CommandLine : public ZCommandLine
	{
public:
	Boolean fHelp;
	String fIF;
	String fIT;
	String fOF;
	String fOT;

	CommandLine()
	:	fHelp("--help", "Print this message and exit"),
		fIF("--if", "Input file", "-"),
		fIT("--it", "Input type (bencode|json|ml|xmlplist|zstream|zstrim|zstreamtuple)", "xmlplist"),
		fOF("--of", "Output file", "-"),
		fOT("--ot", "Output type (json|xmlplist|zstrim|zstream)", "xmlplist")
		{}
	};
} // anonymous namespace

// =================================================================================================

static ZRef<ZStreamerW> sOpenStreamerW(const string& iPath)
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
			theStreamWPos.SetSize(0);
			return theStreamerWPos;
			}
		}
	return ZRef<ZStreamerW>();
	}

static ZRef<ZStreamerR> sOpenStreamerR(const string& iPath)
	{
	if (iPath == "-")
		{
		return new ZStreamerR_T<ZStreamR_FILE>(stdin);
		}
	else
		{
		if (ZRef<ZStreamerR> theStreamerR = ZFileSpec(iPath).OpenR())
			return theStreamerR;
		}
	return ZRef<ZStreamerR>();
	}

typedef void (*Writer_t)(const ZStreamW& iStreamW, ZRef<ZYadR> iYadR, const ZYadOptions& iOptions);

static void sWriteZooLibStrim(const ZStreamW& iStreamW, ZRef<ZYadR> iYadR, const ZYadOptions& iOptions)
	{
	ZYad_ZooLibStrim::sToStrim(ZStrimW_StreamUTF8(iStreamW), iYadR, 0, iOptions);
	}

static void sWriteZooLibStream(const ZStreamW& iStreamW, ZRef<ZYadR> iYadR, const ZYadOptions& iOptions)
	{
	ZYad_ZooLibStream::sToStream(iStreamW, iYadR);
	}

static void sWriteJSON(const ZStreamW& iStreamW, ZRef<ZYadR> iYadR, const ZYadOptions& iOptions)
	{
	ZYad_JSON::sToStrim(ZStrimW_StreamUTF8(iStreamW), iYadR, 0, iOptions);
	}

static void sWriteXMLPList(const ZStreamW& iStreamW, ZRef<ZYadR> iYadR, const ZYadOptions& iOptions)
	{
	ZStrimW_StreamUTF8 theStrimW(iStreamW);
	ZStrimW_ML theStrimW_ML(theStrimW);

	theStrimW_ML.PI("xml");
		theStrimW_ML.Attr("version", "1.0");
		theStrimW_ML.Attr("encoding", "UTF-8");

	theStrimW_ML.Tag("!DOCTYPE");
		theStrimW_ML.Attr("plist");
		theStrimW_ML.Attr("PUBLIC");
		theStrimW_ML.Attr("\"-//Apple Computer//DTD PLIST 1.0//EN\"");
		theStrimW_ML.Attr("\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\"");

	theStrimW_ML.Tag("plist");
		theStrimW_ML.Attr("version", "1.0");

	ZYad_XMLPList::sToStrimW_ML(theStrimW_ML, iYadR);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZMain

int ZMain(int argc, char **argv)
	{
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

	ZRef<ZStreamerR> theStreamerR = sOpenStreamerR(cmd.fIF());
	if (!theStreamerR)
		{
		serr << "Couldn't open input file\n";
		return 1;
		}

	ZRef<ZStreamerW> theStreamerW = sOpenStreamerW(cmd.fOF());
	if (!theStreamerW)
		{
		serr << "Couldn't open output file\n";
		return 1;
		}

	Writer_t theWriter;

	ZYadOptions theOptions(true);
	if (false)
		{}
	else if (cmd.fOT() == "zstrim")
		{
		theWriter = sWriteZooLibStrim;
		}
	else if (cmd.fOT() == "zstream")
		{
		theWriter = sWriteZooLibStream;
		}
	else if (cmd.fOT() == "json")
		{
		theWriter = sWriteJSON;
		}
	else if (cmd.fOT() == "xmlplist")
		{
		theWriter = sWriteXMLPList;
		}
	else
		{
		serr << "Couldn't handle output format\n";
		return 1;
		}

	try
		{
		const ZStreamR& theStreamR = theStreamerR->GetStreamR();
		ZStreamR_Buffered theStreamR_Buffered(1*1024, theStreamR);
		ZStreamU_Unreader theStreamU(theStreamR_Buffered);
		ZStrimR_StreamUTF8 theStrimR(theStreamR_Buffered);
		ZStrimU_Unreader theStrimU(theStrimR);
		ZStreamW_Buffered theStreamW(1*1024, theStreamerW->GetStreamW());
		ZYadOptions theYadOptions(true);
		if (false)
			{}
		else if (cmd.fIT() == "zstrim")
			{
			theWriter(theStreamW, ZYad_ZooLibStrim::sMakeYadR(theStrimU), theYadOptions);
			}
		else if (cmd.fIT() == "zstream")
			{
			theWriter(theStreamW, ZYad_ZooLibStream::sMakeYadR(theStreamR_Buffered), theYadOptions);
			}
		else if (cmd.fIT() == "zstreamtuple")
			{
			theWriter(theStreamW, new ZYadMapR_ZooLibStreamOld(theStreamR_Buffered), theYadOptions);
			}
		else if (cmd.fIT() == "bencode")
			{
			theWriter(theStreamW, ZYad_Bencode::sMakeYadR(theStreamU), theYadOptions);
			}
		else if (cmd.fIT() == "json")
			{
			theWriter(theStreamW, ZYad_JSON::sMakeYadR(theStrimU), theYadOptions);
			}
		else if (cmd.fIT() == "xmlplist")
			{
			ZML::Reader theReader(theStrimU);
			theWriter(theStreamW, ZYad_XMLPList::sMakeYadR(theReader), theYadOptions);
			}
		else if (cmd.fIT() == "ml")
			{
			ZML::Reader theReader(theStrimU);
			theWriter(theStreamW, new ZYadMapR_ML(theReader), theYadOptions);
			}
		else
			{
			serr << "Couldn't handle input format\n";
			return 1;
			}
		}
	catch (std::exception& ex)
		{
		serr << "\nCaught exception: " << ex.what() << "\n";
		return 1;
		}
	sout << "\n";
	return 0;
	}
