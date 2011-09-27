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

#include "ZPane.h"
#include <vector>
#include "ZUI.h"

class ZMessageLooper;


class MessageButtonPane: public ZSuperPane, 
			 public ZPaneLocator,
			 public ZUIButton::Responder
{
 public:
	MessageButtonPane( ZSuperPane *inSuperpane, 
				ZPaneLocator *inLocator, 
			 	const ZMessenger& iMessenger );
	virtual ~MessageButtonPane();

	virtual bool HandleUIButtonEvent(ZUIButton* inButton, ZUIButton::Event inButtonEvent);

	virtual bool GetPaneLocation(ZSubPane* inPane, ZPoint& outLocation);
	virtual bool GetPaneHilite(ZSubPane* inPane, EZTriState& outHilite);

	virtual ZPoint GetSize();

 protected:
	void SendTheMessage();

 private:
	typedef std::vector< ZUIButton* > ButtonList;
	typedef std::vector< string > MessageList;

	ButtonList mButtons;
	MessageList mMessages;
	size_t mSelectedButton;
	ZMessenger mMessenger;
};
