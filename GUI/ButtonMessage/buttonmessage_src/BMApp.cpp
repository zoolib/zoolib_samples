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

#include "BMApp.h"
#include "BMDisplayWindow.h"
#include "BMButtonWindow.h"
#include "ZUtil_UI.h"

BMApp::BMApp()
	: ZApp( "application/x-goingware-buttonmessage" )
{
}
	
BMApp::~BMApp()
{
}

void BMApp::RunStarted()
{
	ZApp::RunStarted();

	BMDisplayWindow *display = new BMDisplayWindow( this,
							BMDisplayWindow::sCreateOSWindow( this ) );

	ZMessenger theMessenger = display->GetMessenger();

	display->Center();
	display->BringFront();
	display->GetLock().Release();


	BMButtonWindow *bmWin = new BMButtonWindow( this,
												BMButtonWindow::sCreateOSWindow( this ),
												theMessenger);

	bmWin->Center();
	bmWin->BringFront();

	ZPoint location( bmWin->GetLocation() );
	ZPoint size( bmWin->GetSize() );

	bmWin->GetLock().Release();

	ZLocker locker( display->GetWindowLock() );

	ZPoint dLoc( display->GetLocation() );

	display->SetLocation( ZPoint( location.h + size.h + 10,
				dLoc.v ) );

}
