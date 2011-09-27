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

#include "MessageButtonPane.h"

#include "ZFakeWindow.h"
#include "ZString.h"
#include "ZUIFactory.h"

MessageButtonPane::MessageButtonPane( ZSuperPane *inSuperpane, ZPaneLocator *inLocator, const ZMessenger& iMessenger )
:	ZSuperPane( inSuperpane, inLocator ),
	ZPaneLocator( nil ),
	mMessenger( iMessenger ),
	mSelectedButton( 0 )
	{
	char *buf = nil;

	for ( long i = 0; i < 5; i++ )
		{
		string theText = ZString::sFromInt(i);
		mMessages.push_back(theText);
		mButtons.push_back( ZUIFactory::sGet()->Make_ButtonRadio( this,
									this,
									this,
									 theText) );
		}
	}

MessageButtonPane::~MessageButtonPane()
	{}

bool MessageButtonPane::HandleUIButtonEvent(ZUIButton* inButton, 
							ZUIButton::Event inButtonEvent)
	{
	ButtonList::iterator found( find( mButtons.begin(), mButtons.end(), inButton ) );

	if ( found != mButtons.end() )
		{
		if ( inButtonEvent == ZUIButton::ButtonAboutToBeReleasedWhileDown )
			{
			size_t hit = found - mButtons.begin();

			if ( hit != mSelectedButton )
				{
				this->Refresh();
				mSelectedButton = hit;
				this->SendTheMessage();
				this->Refresh();
				}
			}
		}
	return false;
	}

bool MessageButtonPane::GetPaneHilite(ZSubPane* inPane, EZTriState& outHilite)
	{
	ButtonList::iterator found( find( mButtons.begin(), mButtons.end(), inPane ) );

	if ( found != mButtons.end() )
		{
		if ( (found - mButtons.begin()) == mSelectedButton )
			outHilite = eZTriState_On;
		else
			outHilite = eZTriState_Off;
		return true;
		}

	return ZPaneLocator::GetPaneHilite( inPane, outHilite );
	}

bool MessageButtonPane::GetPaneLocation(ZSubPane* inPane, ZPoint& outLocation)
	{
	ButtonList::iterator found( find( mButtons.begin(), mButtons.end(), inPane ) );

	if ( found != mButtons.end() )
		{
		ZPoint firstSize = mButtons.front()->GetSize();

		outLocation.h = 0;
		outLocation.v = (found - mButtons.begin()) * firstSize.v;

		return true;
		}

	return ZPaneLocator::GetPaneLocation( inPane, outLocation );
	}

ZPoint MessageButtonPane::GetSize()
	{
	long num = mButtons.size();

	if ( num == 0 )
		return ZPoint( 0, 0 );

	return ZPoint( 100, num * mButtons.front()->GetSize().v );
	}

void MessageButtonPane::SendTheMessage()
	{
	ZMessage msg;

	msg.SetString( "what", "ButtonMessage:display" );
	msg.SetString( "ButtonMessage:parameter", mMessages[ mSelectedButton ] );

	mMessenger.PostMessage(msg);
	}
