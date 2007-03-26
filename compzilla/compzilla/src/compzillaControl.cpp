/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#include "nspr.h"
#include "nsMemory.h"
#include "nsNetCID.h"
#include "nsISupportsUtils.h"
#include "nsIIOService.h"
#include "nsIObserverService.h"
#include "nsIURI.h"
#include "nsIScriptSecurityManager.h"
#include "nsServiceManagerUtils.h"
#include "nsStringAPI.h"
#include "compzillaControl.h"

// These headers are used for finding the GdkWindow for a DOM window
#include "nsIBaseWindow.h"
#include "nsIDocShell.h"
#include "nsIScriptGlobalObject.h"
#include "nsIWidget.h"


compzillaControl::compzillaControl()
{
}


compzillaControl::~compzillaControl()
{
}


NS_IMPL_ISUPPORTS1_CI(compzillaControl, compzillaIControl);

