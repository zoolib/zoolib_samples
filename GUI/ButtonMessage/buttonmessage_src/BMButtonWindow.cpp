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

#include "BMButtonWindow.h"
#include "MessageButtonPane.h"
#include "ZApp.h"
#include "ZUIFactory.h"

BMButtonWindow::BMButtonWindow(ZWindowSupervisor *inSupervisor, ZOSWindow *inOSWindow,
							const ZMessenger& iMessenger )
:	ZWindow( inSupervisor, inOSWindow ),
	ZPaneLocator( nil ),
	mContentPane( new ZWindowPane( this, this ) ),
	mQuitButton( ZUIFactory::sGet()->Make_ButtonPush( mContentPane, this, this, "Quit" ) ),
	mMessagePane( new MessageButtonPane( mContentPane, this, iMessenger ) )
{
	ZPoint loc = mMessagePane->GetLocation();
	ZPoint sz = mMessagePane->GetSize();

	ZPoint oldSize = this->GetSize();

	SetSize( ZPoint( oldSize.h, loc.v + sz.v ) );
}

BMButtonWindow::~BMButtonWindow()
{
}

ZOSWindow* BMButtonWindow::sCreateOSWindow(ZApp* inApp)
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

bool BMButtonWindow::GetPaneLocation( ZSubPane *inPane, ZPoint &outLocation )
{
	if ( inPane == mQuitButton ){
		outLocation = ZPoint( 5, 5 );

		return true;
	}

	if ( inPane == mMessagePane ){

		ZPoint qtLoc( mQuitButton->GetLocation() );
		ZPoint qtSz( mQuitButton->GetSize() );

		outLocation = ZPoint( 0,
				qtLoc.v + qtSz.v + 5 );

		return true;

	}

	return ZPaneLocator::GetPaneLocation( inPane, outLocation );
}

bool BMButtonWindow::HandleUIButtonEvent(ZUIButton* inButton, 
					 ZUIButton::Event inButtonEvent)
{
	if ( inButton == mQuitButton )
	{
		if ( inButtonEvent == ZUIButton::ButtonAboutToBeReleasedWhileDown )
		{
			ZApp::sGet()->QueueRequestQuitMessage();
		}
	}

	return false;
}

void BMButtonWindow::WindowCloseByUser()
{
	ZApp::sGet()->QueueRequestQuitMessage();
}
