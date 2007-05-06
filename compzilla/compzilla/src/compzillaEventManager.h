/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#ifndef compzillaEventManager_h__
#define compzillaEventManager_h__


#define MOZILLA_INTERNAL_API
#include <nsCOMArray.h>
#undef MOZILLA_INTERNAL_API
#include <nsCOMPtr.h>
#include <nsIDOMEvent.h>
#include <nsIDOMEventListener.h>
#include <nsIDOMEventTarget.h>
#include <nsIScriptContext.h>


class compzillaEventManager
{
public:
    compzillaEventManager (const char *eventName);
    virtual ~compzillaEventManager ();
    
    // This creates a trusted event, which is not cancelable and doesn't
    // bubble. Don't call this if we have no event listeners, since this may
    // use our script context, which is not set in that case.
    nsresult CreateEvent(const nsAString& aType, 
			 nsIDOMEventTarget *aEventTarget,
			 nsIDOMEvent** domevent);
    
    void NotifyEventListeners(nsIDOMEvent* aEvent);

    NS_IMETHOD AddEventListener(const nsAString& type,
                                nsIDOMEventListener *listener);

    NS_IMETHOD RemoveEventListener(const nsAString & type,
                                   nsIDOMEventListener *listener);

    void ClearEventListeners();

    bool HasEventListeners ();

private:
    static nsIScriptContext *GetCurrentContext ();

    nsString mEventName;
    nsCOMArray<nsIDOMEventListener> mListeners;
    nsCOMPtr<nsIScriptContext> mScriptContext;
};


#endif
