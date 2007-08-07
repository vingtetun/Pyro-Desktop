/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#include <nspr.h>
#include <nsCOMPtr.h>
#include <nsXPCOMCID.h>

#include <nsICategoryManager.h>
#include <nsIDOMClassInfo.h>           // unstable
#include <nsIGenericFactory.h>
#include <nsIScriptNameSpaceManager.h> // unstable
#include <nsIServiceManager.h>
#include <nsISupportsUtils.h>
#include <nsServiceManagerUtils.h>

#include "compzillaControl.h"
#include "compzillaRenderingContext.h"


NS_GENERIC_FACTORY_CONSTRUCTOR(compzillaControl);
NS_DECL_CLASSINFO(compzillaControl);

NS_GENERIC_FACTORY_CONSTRUCTOR(compzillaRenderingContext);
NS_DECL_CLASSINFO(compzillaRenderingContext)

NS_DECL_CLASSINFO(compzillaWindow)


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

    rv = catman->AddCategoryEntry(JAVASCRIPT_GLOBAL_CONSTRUCTOR_CATEGORY,
                                  "CompzillaControl",
                                  COMPZILLA_CONTROL_CONTRACTID,
                                  PR_TRUE, PR_TRUE, NULL);
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
        nsIClassInfo::SINGLETON
    },
    {
        "Compzilla Canvas Rendering Context",
        COMPZILLA_RENDERING_CONTEXT_CID,
        COMPZILLA_RENDERING_CONTEXT_CONTRACTID,
        compzillaRenderingContextConstructor,
        nsnull, nsnull, nsnull,
        NS_CI_INTERFACE_GETTER_NAME(compzillaRenderingContext),
        nsnull,
        &NS_CLASSINFO_NAME(compzillaRenderingContext),
        nsIClassInfo::DOM_OBJECT
    },
    {
        "Compzilla Native Window",
        COMPZILLA_WINDOW_CID,
        COMPZILLA_WINDOW_CONTRACTID,
        nsnull, nsnull, nsnull, nsnull,
        NS_CI_INTERFACE_GETTER_NAME(compzillaWindow),
        nsnull,
        &NS_CLASSINFO_NAME(compzillaWindow),
        nsIClassInfo::PLUGIN_OBJECT
    },
};


NS_IMPL_NSGETMODULE(compzillaModule, components)
