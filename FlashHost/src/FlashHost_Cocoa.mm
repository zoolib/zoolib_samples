#include "FlashHost_Cocoa.h"

#if ZCONFIG_SPI_Enabled(Cocoa)

#include "zoolib/ZStream_String.h"

namespace net_em {

using ZNetscape::NPObjectH;
using ZNetscape::NPVariantH;
using std::string;

// =================================================================================================
#pragma mark -
#pragma mark * ObjectH_Location (anonymous)

namespace { // anonymous

class ObjectH_Location : public ZNetscape::ObjectH
	{
public:
	ObjectH_Location(const string& iPageURL);
	virtual ~ObjectH_Location();

	virtual bool Imp_Invoke(
		const std::string& iName, const NPVariantH* iArgs, size_t iCount, NPVariantH& oResult);
	virtual bool Imp_GetProperty(const std::string& iName, NPVariantH& oResult);

private:
	const string fPageURL;
	};

ObjectH_Location::ObjectH_Location(const string& iPageURL)
:	fPageURL(iPageURL)
	{}

ObjectH_Location::~ObjectH_Location()
	{}

bool ObjectH_Location::Imp_Invoke(
	const std::string& iName, const NPVariantH* iArgs, size_t iCount, NPVariantH& oResult)
	{
	if (iName == "__flash_getWindowLocation" || iName == "__flash_getTopLocation")
		{
		oResult = fPageURL;
		return true;
		}
	return false;
	}

bool ObjectH_Location::Imp_GetProperty(const std::string& iName, NPVariantH& oResult)
	{
	if (iName == "location")
		{
		oResult = fPageURL;
		return true;
		}
	else
		{
		oResult = this;
		return true;
		}
	}

} // anonymous namespace

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
