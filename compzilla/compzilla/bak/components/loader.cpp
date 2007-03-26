// Copyright (c) 2005 Benjamin Smedberg <benjamin@smedbergs.us>


#include "nscore.h"
#include "nsModule.h"
#include "prlink.h"
#include "nsILocalFile.h"
#include "nsStringAPI.h"
#include "nsCOMPtr.h"


static char const *const kDependentLibraries[] =
{
  // compzilla.dll on windows, compzilla.so on linux

  MOZ_DLL_PREFIX "Xdamage" MOZ_DLL_SUFFIX,
  nsnull

  // NOTE: if the dependent libs themselves depend on other libs, the subdependencies
  // should be listed first.
};


// compzilla.dll on windows, compzilla.dll on linux
static char kRealComponent[] = MOZ_DLL_PREFIX "compzilla" MOZ_DLL_SUFFIX;


nsresult
NSGetModule(nsIComponentManager* aCompMgr,
            nsIFile* aLocation,
            nsIModule* *aResult)
{
  nsresult rv;

  // This is not the real component. We want to load the dependent libraries
  // of the real component, then the component itself, and call NSGetModule on
  // the component.

  // Assume that we're in <extensiondir>/components, and we want to find
  // <extensiondir>/libraries

  nsCOMPtr<nsIFile> libraries;
  rv = aLocation->GetParent(getter_AddRefs(libraries));
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsILocalFile> library(do_QueryInterface(libraries));
  if (!library)
    return NS_ERROR_UNEXPECTED;

  library->SetNativeLeafName(NS_LITERAL_CSTRING("libraries"));
  library->AppendNative(NS_LITERAL_CSTRING("dummy"));

  // loop through and load dependent libraries
  for (char const *const *dependent = kDependentLibraries;
       *dependent;
       ++dependent) {
    library->SetNativeLeafName(nsDependentCString(*dependent));
    PRLibrary *lib;
    library->Load(&lib);
    // 1) We don't care if this failed!
    // 2) We are going to leak this library. We don't care about that either.
  }

  library->SetNativeLeafName(NS_LITERAL_CSTRING(kRealComponent));

  PRLibrary *lib;
  rv = library->Load(&lib);
  if (NS_FAILED(rv))
    return rv;

  nsGetModuleProc getmoduleproc = (nsGetModuleProc)
    PR_FindFunctionSymbol(lib, NS_GET_MODULE_SYMBOL);

  if (!getmoduleproc)
    return NS_ERROR_FAILURE;

  return getmoduleproc(aCompMgr, aLocation, aResult);
}
