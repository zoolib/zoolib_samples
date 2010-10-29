#include "FlashHost_Cocoa.h"
#include "FlashHost.h"

#if ZCONFIG_SPI_Enabled(Cocoa)

#include "zoolib/ZStream_String.h"

namespace net_em {

using ZNetscape::NPObjectH;
using ZNetscape::NPVariantH;
using std::string;

// =================================================================================================
#pragma mark -
#pragma mark * FlashHost_Cocoa

FlashHost_Cocoa::FlashHost_Cocoa(
	ZRef<ZNetscape::GuestFactory> iGuestFactory, NSView_NetscapeHost* iView)
:	Host_Cocoa(iGuestFactory, iView)
	{}

FlashHost_Cocoa::~FlashHost_Cocoa()
	{}

NPError FlashHost_Cocoa::Host_GetURLNotify(NPP npp,
	const char* iRelativeURL, const char* iTarget, void* notifyData)
	{
	if (iRelativeURL == strstr(iRelativeURL, "javascript:"))
		{
		const string theURL = fURL + "/__flashplugin_unique__";
		this->SendDataSync(notifyData, iRelativeURL, "text/html", ZStreamRPos_String(theURL));
		return NPERR_NO_ERROR;
		}

	return Host_Cocoa::Host_GetURLNotify(npp, iRelativeURL, iTarget, notifyData);
	}

ZRef<NPObjectH> FlashHost_Cocoa::Host_GetWindowObject()
	{
	string theURL = fURL.substr(0, fURL.find('?'));
	return new ObjectH_Location(theURL);
	}

bool FlashHost_Cocoa::Host_Evaluate(NPP npp,
	NPObject* obj, NPString* script, NPVariant* result)
	{
	((NPVariantH*)(result))->SetNull();
	return true;
	}

} // namespace net_em

#endif // ZCONFIG_SPI_Enabled(Cocoa)
