/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#include <nspr.h>
#include <nsCOMPtr.h>
#include <nsXPCOMCID.h>

#include <nsICategoryManager.h>
#include <nsIDOMClassInfo.h>           // unstable
#include "mozilla/ModuleUtils.h"
#include <nsIScriptNameSpaceManager.h> // unstable
#include <nsIServiceManager.h>
#include <nsISupportsUtils.h>
#include <nsServiceManagerUtils.h>

#include "compzillaControl.h"
#include "compzillaRenderingContext.h"


NS_GENERIC_FACTORY_CONSTRUCTOR(compzillaControl)
NS_DEFINE_NAMED_CID(COMPZILLA_CONTROL_CID);

nsresult NS_NewCompzillaRenderingContext(compzillaIRenderingContext** aResult);
#define MAKE_CTOR(ctor_, iface_, func_)                   \
static nsresult                                           \
ctor_(nsISupports* aOuter, REFNSIID aIID, void** aResult) \
{                                                         \
  *aResult = nsnull;                                      \
  if (aOuter)                                             \
    return NS_ERROR_NO_AGGREGATION;                       \
  iface_* inst;                                           \
  nsresult rv = func_(&inst);                             \
  if (NS_SUCCEEDED(rv)) {                                 \
    rv = inst->QueryInterface(aIID, aResult);             \
    NS_RELEASE(inst);                                     \
  }                                                       \
  return rv;                                              \
}
MAKE_CTOR(CreateCompzillaRenderingContext, compzillaIRenderingContext, NS_NewCompzillaRenderingContext)
NS_DEFINE_NAMED_CID(COMPZILLA_RENDERING_CONTEXT_CID);

NS_DEFINE_NAMED_CID(COMPZILLA_WINDOW_CID);

static const mozilla::Module::CIDEntry kCompzillaCIDs[] = {
    { &kCOMPZILLA_CONTROL_CID, false, NULL, compzillaControlConstructor },
    { &kCOMPZILLA_RENDERING_CONTEXT_CID, false, NULL, CreateCompzillaRenderingContext},
    { &kCOMPZILLA_WINDOW_CID, false, NULL, NULL },
    { NULL }
};

static const mozilla::Module::ContractIDEntry kCompzillaContracts[] = {
    { COMPZILLA_CONTROL_CONTRACTID, &kCOMPZILLA_CONTROL_CID },
    { COMPZILLA_RENDERING_CONTEXT_CONTRACTID, &kCOMPZILLA_RENDERING_CONTEXT_CID },
    { COMPZILLA_WINDOW_CONTRACTID, &kCOMPZILLA_WINDOW_CID },
    { NULL }
};

static const mozilla::Module::CategoryEntry kCompzillaCategories[] = {
    { NULL }
};

static const mozilla::Module kCompzillaModule = {
    mozilla::Module::kVersion,
    kCompzillaCIDs,
    kCompzillaContracts,
    kCompzillaCategories
};

NSMODULE_DEFN(nsCompzillaModule) = &kCompzillaModule;

NS_IMPL_MOZILLA192_NSGETMODULE(&kCompzillaModule)
