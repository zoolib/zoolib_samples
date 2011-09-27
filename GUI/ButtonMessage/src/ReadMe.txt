                     ButtonMessage GUI Sample
        The ZooLib C++ Cross-Platform Application Framework
                       http://www.zoolib.org

This is a demo program built using ZooLib, a multithreaded 
cross-platform C++ application framework.

Besides GUI, ZooLib also includes a lightweight database file format 
(in which the databases are entirely contained in single files, so 
they can serve as user documents) and TCP networking.  There is also 
quite a bit of debugging support (assertions and a debugging memory
allocator).

While ZooLib's home page is http://www.zoolib.org , you can find full 
details including sample code and the source distribution at 
http://sourceforge.net/projects/zoolib

ZooLib was written over the past several years by Andrew Green of the 
Electric Magic Company, http://www.em.net and Learning in Motion, 
http://www.learn.motion.com .

The ButtonMessage demo shows how to handle button highlighting -
in ZooLib, buttons do not maintain their own state, instead the state
is kept for them by their ZPaneLocator and supplied whenever necessary,
as during a screen update.

Also it shows how to send a ZMessage from one window to another.  It
illustrates an architectural problem which Andrew Green is working to
address - if the recipient window is closed, then sending a ZMessage
to it will result in calling a virtual method on a stale pointer, and
will crash.  If you close the display window then click a radio button,
you get this crash.

Ordinarily when a window is closed it should report that fact to some
other part of the program to deal with it.  In the button window we
handle a window close by quitting.

This program is also meant to be a nicer demo on Posix than is currently
possible with ZHelloWorld.  That's because the UI is built entirely 
with buttons - with ZHelloWorld, the menu bar doesn't show up on
Posix so there's no way to quit except to kill the process.

Also shown is how to lock a window before refreshing its pane, and an
example of how to refresh a pane whose size is changing - call 
ZSubPane::Refresh() on it both before and after doing the size changing
operation.  In our case the text in the display window originally
reads "http://www.zoolib.org" and when you click a button it
changes to a digit.  If we only refreshed the pane after changing its
text, we wouldn't erase most of the old string.

Enjoy!

Michael D. Crawford
Dulcinea Technologies Corporation
Software of Elegance and Beauty
http://www.dulcineatech.com
mike@dulcineatech.com
