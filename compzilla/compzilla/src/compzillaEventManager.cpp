/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#define MOZILLA_INTERNAL_API
#include <nsIPrivateDOMEvent.h>
#define MOZILLA_INTERNAL_API
#include <jsapi.h>
#include <nsIJSContextStack.h>
#include <nsServiceManagerUtils.h>
#include <nsDOMJSUtils.h>

#include "compzillaEventManager.h"


/*
 * This is entirely ripped off from nsXMLHTTPRequest.cpp.  I'm surprised 
 * there's not a mozilla utility for this already...
 */

compzillaEventManager::compzillaEventManager (const char *eventName)
    : mEventName(NS_ConvertASCIItoUTF16 (eventName))
{
}


compzillaEventManager::~compzillaEventManager ()
{
    // Needed to free the listener array.
    ClearEventListeners();
}


nsresult 
compzillaEventManager::CreateEvent (const nsAString& aType, 
                                    nsIDOMEventTarget *aEventTarget,
				    nsIDOMEvent** aDOMEvent)
{
    nsresult rv = NS_NewDOMEvent (aDOMEvent, NULL, NULL);
    if (NS_FAILED(rv)) {
        return rv;
    }

    nsCOMPtr<nsIPrivateDOMEvent> privevent(do_QueryInterface(*aDOMEvent));
    if (!privevent) {
        NS_IF_RELEASE(*aDOMEvent);
        return NS_ERROR_FAILURE;
    }

    if (!aType.IsEmpty()) {
        (*aDOMEvent)->InitEvent(aType, PR_FALSE, PR_FALSE);
    }

    privevent->SetTarget(aEventTarget);
    privevent->SetCurrentTarget(aEventTarget);
    privevent->SetOriginalTarget(aEventTarget);

    // We assume anyone who managed to call CreateEvent is trusted
    privevent->SetTrusted(PR_TRUE);

    return NS_OK;
}


void
compzillaEventManager::NotifyEventListeners (nsIDOMEvent* aEvent)
{
    if (!aEvent)
        return;

    nsCOMPtr<nsIJSContextStack> stack;
    JSContext *cx = nsnull;

    if (mScriptContext) {
        stack = do_GetService("@mozilla.org/js/xpc/ContextStack;1");

        if (stack) {
            cx = (JSContext *)mScriptContext->GetNativeContext();

            if (cx) {
                stack->Push(cx);
            }
        }
    }

    // Make a copy in case a handler alter the listener list
    nsCOMArray<nsIDOMEventListener> listeners;
    listeners.AppendObjects(mListeners);

    PRInt32 count = listeners.Count();
    for (PRInt32 index = 0; index < count; ++index) {
        nsIDOMEventListener* listener = listeners[index];
        if (listener) {
            listener->HandleEvent(aEvent);
        }
    }

    if (cx) {
        stack->Pop(&cx);
    }
}


void
compzillaEventManager::ClearEventListeners ()
{
    mListeners.Clear();
}


NS_IMETHODIMP
compzillaEventManager::AddEventListener(const nsAString& type,
                                        nsIDOMEventListener *listener)
{
    NS_ENSURE_ARG(listener);

    if (type.Equals(mEventName)) {
        mListeners.AppendObject(listener);
        mScriptContext = GetCurrentContext();
  
        return NS_OK;
    }

    return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
compzillaEventManager::RemoveEventListener(const nsAString & type,
                                           nsIDOMEventListener *listener)
{
    NS_ENSURE_ARG(listener);

    // Allow a caller to remove O(N^2) behavior by removing end-to-start.
    for (PRUint32 i = mListeners.Count() - 1; i != PRUint32(-1); --i) {
        if (mListeners.ObjectAt(i) == listener) {
            mListeners.RemoveObjectAt(i);
            return NS_OK;
        }
    }

    return NS_ERROR_FAILURE;
}


nsIScriptContext *
compzillaEventManager::GetCurrentContext ()
{
    // Get JSContext from stack.
    nsCOMPtr<nsIJSContextStack> stack =
        do_GetService("@mozilla.org/js/xpc/ContextStack;1");

    if (!stack) {
        return nsnull;
    }

    JSContext *cx;

    if (NS_FAILED(stack->Peek(&cx)) || !cx) {
        return nsnull;
    }

    return GetScriptContextFromJSContext(cx);
}
