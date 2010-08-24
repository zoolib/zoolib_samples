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
#include "zoolib/ZLog.h"
#include "zoolib/ZML.h"
#include "zoolib/ZStdIO.h"
#include "zoolib/ZStream_Buffered.h"
#include "zoolib/ZStream_POSIX.h"
#include "zoolib/ZStrim_Stream.h"
#include "zoolib/ZStrimmer_Streamer.h"
#include "zoolib/ZStrimU_Unreader.h"
#include "zoolib/ZYad_Bencode.h"
#include "zoolib/ZYad_JSON.h"
#include "zoolib/ZYad_ML.h"
#include "zoolib/ZYad_XMLAttr.h"
#include "zoolib/ZYad_XMLPList.h"
#include "zoolib/ZYad_ZooLibStream.h"
#include "zoolib/ZYad_ZooLibStrim.h"

using namespace ZooLib;

using std::string;

const ZStrimW& serr = ZStdIO::strim_err;
const ZStrimW& sout = ZStdIO::strim_out;
const ZStreamR& stream_in = ZStdIO::stream_in;

// =================================================================================================
#pragma mark -
#pragma mark * CommandLine

namespace {
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
		fIT("--it",
			"Input type (bencode|json|ml|xmlattr|xmlplist|zstream|zstream|zstreamtuple|zstrim)",
			"xmlplist"),
		fOF("--of", "Output file", "-"),
		fOT("--ot", "Output type (json|xmlplist|zstream|zstreamjava|zstrim)", "xmlplist")
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
			{
			return theStreamerR;
			}
		}
	return ZRef<ZStreamerR>();
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

	ZRef<ZYadR> theYadR;

	try
		{
		ZRef<ZStreamerR> theStreamerR_Buffered = new ZStreamerR_Buffered(4096, theStreamerR);

		ZRef<ZStreamerU> theStreamerU_Unreader =
			new ZStreamerU_FT<ZStreamU_Unreader>(theStreamerR_Buffered);
		
		ZRef<ZStrimmerR> theStrimmerR_StreamUTF8 =
			new ZStrimmerR_Streamer_T<ZStrimR_StreamUTF8>(theStreamerR_Buffered);

		ZRef<ZStrimmerU> theStrimmerU_Unreader =
			new ZStrimmerU_FT<ZStrimU_Unreader>(theStrimmerR_StreamUTF8);

		ZRef<ZML::StrimmerU> theStrimmerU_ML = new ZML::StrimmerU(theStrimmerU_Unreader);

		if (false)
			{}
		else if (cmd.fIT() == "zstream")
			{
			theYadR = ZYad_ZooLibStream::sMakeYadR(theStreamerR_Buffered);
			}
		else if (cmd.fIT() == "zstreamtuple")
			{
			theYadR = new ZYadMapR_ZooLibStreamOld(theStreamerR_Buffered);
			}
		else if (cmd.fIT() == "bencode")
			{
			theYadR = ZYad_Bencode::sMakeYadR(theStreamerU_Unreader);
			}
		else if (cmd.fIT() == "json")
			{
			theYadR = ZYad_JSON::sMakeYadR(theStrimmerU_Unreader);
			}
		else if (cmd.fIT() == "zstrim")
			{
			theYadR = ZYad_ZooLibStrim::sMakeYadR(theStrimmerU_Unreader);
			}
		else if (cmd.fIT() == "ml")
			{
			theYadR = new ZYadMapR_ML(theStrimmerU_ML);
			}
		else if (cmd.fIT() == "xmlattr")
			{
			theYadR = ZYad_XMLAttr::sMakeYadR(theStrimmerU_ML);
			}
		else if (cmd.fIT() == "xmlplist")
			{
			theYadR = ZYad_XMLPList::sMakeYadR(theStrimmerU_ML);
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

	ZYadOptions theYadOptions(true);

//	ZStreamW_Buffered theStreamW(1*1024, theStreamerW->GetStreamW());
	const ZStreamW& theStreamW(theStreamerW->GetStreamW());

	if (false)
		{}
	else if (cmd.fOT() == "zstream")
		{
		ZYad_ZooLibStream::sToStream(theStreamW, theYadR);
		}
	else if (cmd.fOT() == "zstreamjava")
		{
		ZYad_ZooLibStream::sToStream(theStreamW, theYadR);
		}
	else if (cmd.fOT() == "json")
		{
		ZYad_JSON::sToStrim(0, theYadOptions,
			theYadR, ZStrimW_StreamUTF8(theStreamW));
		}
	else if (cmd.fOT() == "zstrim")
		{
		ZYad_ZooLibStrim::sToStrim(0, theYadOptions,
			theYadR, ZStrimW_StreamUTF8(theStreamW));
		}
	else if (cmd.fOT() == "xmlplist")
		{
		ZStrimW_StreamUTF8 theStrimW(theStreamW);
		ZML::StrimW theStrimW_ML(theStrimW);

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

		ZYad_XMLPList::sToStrim(theYadR, theStrimW_ML);
		}
	else
		{
		serr << "Couldn't handle output format\n";
		return 1;
		}

	sout << "\n";

	return 0;
	}
