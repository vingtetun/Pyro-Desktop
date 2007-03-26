/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
#include "nspr.h"
#include "nsCOMPtr.h"
#include "nsEmbedString.h"
#include "nsXPCOMCID.h"

#include "nsICategoryManager.h"
#include "nsIGenericFactory.h"
#include "nsIScriptNameSpaceManager.h"
#include "nsIServiceManager.h"
#include "nsISupportsUtils.h"
#include "nsServiceManagerUtils.h"

#include "compzillaControl.h"
#include "compzillaExtension.h"


NS_GENERIC_FACTORY_CONSTRUCTOR(compzillaExtension);


NS_GENERIC_FACTORY_CONSTRUCTOR(compzillaControl);
NS_DECL_CLASSINFO(compzillaControl);


static NS_METHOD 
registerGlobalConstructors(nsIComponentManager *aCompMgr,
                           nsIFile *aPath,
                           const char *registryLocation,
                           const char *componentType,
                           const nsModuleComponentInfo *info)
{
    nsresult rv = NS_OK;

    nsCOMPtr<nsICategoryManager> catman = do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
    if (NS_FAILED(rv))
        return rv;

    nsCString previous;
    rv = catman->AddCategoryEntry(JAVASCRIPT_GLOBAL_CONSTRUCTOR_CATEGORY,
                                  "CompzillaControl",
                                  COMPZILLA_CONTROL_CONTRACTID,
                                  PR_TRUE, PR_TRUE, getter_Copies(previous));
    NS_ENSURE_SUCCESS(rv, rv);

    return rv;
}


static const nsModuleComponentInfo components[] = {
    { 
        "Compzilla Window Manager Service",
        COMPZILLA_CONTROL_CID,
        COMPZILLA_CONTROL_CONTRACTID,
        compzillaControlConstructor,
        registerGlobalConstructors,
        NULL, // mFactoryDestrucrtor
        NULL, // mGetInterfacesProcPtr
        NS_CI_INTERFACE_GETTER_NAME(compzillaControl),
        NULL, // mGetLanguageHelperProc
        &NS_CLASSINFO_NAME(compzillaControl),
        nsIClassInfo::DOM_OBJECT
    },
    {
        "Compzilla Extension",
        COMPZILLA_EXTENSION_CID,
        COMPZILLA_EXTENSION_CONTRACTID,
        compzillaExtensionConstructor
    }
};


NS_IMPL_NSGETMODULE(compzillaModule, components)
