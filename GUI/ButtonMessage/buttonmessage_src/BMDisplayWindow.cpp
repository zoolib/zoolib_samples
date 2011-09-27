/* ------------------------------------------------------------
Copyright (c) GoingWare Inc.
http://www.goingware.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------ */

#include "BMDisplayWindow.h"
#include "ZApp.h"
#include "ZOSWindow.h"
#include "ZUIFactory.h"

BMDisplayWindow::BMDisplayWindow( ZWindowSupervisor *inSupervisor, ZOSWindow *inOSWindow )
:	ZWindow( inSupervisor, inOSWindow ),
	ZPaneLocator( nil ),
	mContentPane( new ZWindowPane( this, this ) ),
	mDisplay( ZUIFactory::sGet()->Make_CaptionPane( mContentPane, 
								this, 
								"http://www.zoolib.org/" ) )
{
	mContentPane->BecomeWindowTarget();

	return;
}

BMDisplayWindow::~BMDisplayWindow()
{
}

ZOSWindow* BMDisplayWindow::sCreateOSWindow(ZApp* inApp)
{
	
	ZOSWindow::CreationAttributes attr;
	
	attr.fFrame = ZRect(0, 0, 200, 80);
	attr.fLook = ZOSWindow::lookDocument;
	attr.fLayer = ZOSWindow::layerDocument;
	attr.fResizable = false;
	attr.fHasSizeBox = false;
	attr.fHasCloseBox = true;
	attr.fHasZoomBox = false;
	attr.fHasMenuBar = false;
	attr.fHasTitleIcon = false;
	
	return inApp->CreateOSWindow( attr );
	
}

void BMDisplayWindow::ReceivedMessage(const ZMessage& inMessage)
	{
	string theWhat;
	if ( inMessage.GetString( "what", theWhat ) )
		{
		if ( theWhat == "ButtonMessage:display" )
			{
			string param;

			if ( inMessage.GetString( "ButtonMessage:parameter", param ) )
				{
				ZRef<ZUICaption> newCaption = new ZUICaption_Text(param, ZUIAttributeFactory::sGet()->GetFont_SystemLarge(), 0);
				mDisplay->SetCaption(newCaption, true);
				}
			}
		}
	return ZWindow::ReceivedMessage( inMessage );
	}

bool BMDisplayWindow::GetPaneLocation(ZSubPane* inPane, ZPoint& outLocation)
{
	if ( mDisplay == inPane ){
		outLocation = ZPoint( 10, 10 );
		return true;
	}

	return ZPaneLocator::GetPaneLocation( inPane, outLocation );
}

