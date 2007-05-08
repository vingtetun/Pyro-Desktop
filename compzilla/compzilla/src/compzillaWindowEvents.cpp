/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*- */


#include "compzillaWindowEvents.h"


NS_IMPL_ADDREF(compzillaWindowEvent)
NS_IMPL_RELEASE(compzillaWindowEvent)
NS_INTERFACE_MAP_BEGIN(compzillaWindowEvent)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, compzillaIWindowPropertyEvent)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsIDOMEvent, compzillaIWindowPropertyEvent)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(compzillaIWindowEvent, compzillaIWindowPropertyEvent)
    NS_INTERFACE_MAP_ENTRY(compzillaIWindowConfigureEvent)
    NS_INTERFACE_MAP_ENTRY(compzillaIWindowPropertyEvent)
    NS_IMPL_QUERY_CLASSINFO(compzillaWindowEvent)
NS_INTERFACE_MAP_END
NS_IMPL_CI_INTERFACE_GETTER4(compzillaWindowEvent, 
                             nsIDOMEvent,
                             compzillaIWindowEvent,
                             compzillaIWindowConfigureEvent,
                             compzillaIWindowPropertyEvent)


compzillaWindowEvent::~compzillaWindowEvent()
{
}


//
// compzillaIWindowEvent implementation...
//

compzillaWindowEvent::compzillaWindowEvent(compzillaIWindow *window)
    : mWindow(window)
{
}


NS_IMETHODIMP 
compzillaWindowEvent::GetWindow(compzillaIWindow **aWindow)
{
    *aWindow = mWindow;
    return NS_OK;
}


nsresult 
compzillaWindowEvent::Send(const nsString& type, 
                           nsIDOMEventTarget *eventTarget,
                           compzillaEventManager& eventMgr)
{
    mTarget = eventTarget;
    eventMgr.NotifyEventListeners (NS_REINTERPRET_CAST (compzillaIWindowPropertyEvent *, this));
    return NS_OK;
}


//
// compzillaIWindowConfigureEvent implementation...
//

compzillaWindowEvent::compzillaWindowEvent(compzillaIWindow *window,
                                           bool mapped,
                                           bool override,
                                           long x,
                                           long y,
                                           long width,
                                           long height,
                                           long borderWidth,
                                           compzillaIWindow *aboveWin)
    : mWindow(window),
      mMapped(mapped),
      mOverrideRedirect(override),
      mRect(x, y, width, height),
      mBorderWidth(borderWidth),
      mAboveWindow(aboveWin)
{
}


NS_IMETHODIMP
compzillaWindowEvent::GetMapped(PRBool *aMapped)
{
    *aMapped = mMapped;
    return NS_OK;
}


NS_IMETHODIMP 
compzillaWindowEvent::GetOverrideRedirect(PRBool *aOverrideRedirect)
{
    *aOverrideRedirect = mOverrideRedirect;
    return NS_OK;
}


NS_IMETHODIMP 
compzillaWindowEvent::GetX(PRInt32 *aX)
{
    *aX = mRect.x;
    return NS_OK;
}


NS_IMETHODIMP 
compzillaWindowEvent::GetY(PRInt32 *aY)
{
    *aY = mRect.y;
    return NS_OK;
}


NS_IMETHODIMP 
compzillaWindowEvent::GetWidth(PRInt32 *aWidth)
{
    *aWidth = mRect.width;
    return NS_OK;
}


NS_IMETHODIMP
compzillaWindowEvent::GetHeight(PRInt32 *aHeight)
{
    *aHeight = mRect.height;
    return NS_OK;
}


NS_IMETHODIMP
compzillaWindowEvent::GetBorderWidth(PRInt32 *aBorderWidth)
{
    *aBorderWidth = mRect.width;
    return NS_OK;
}


NS_IMETHODIMP
compzillaWindowEvent::GetAboveWindow(compzillaIWindow **aAboveWindow)
{
    *aAboveWindow = mAboveWindow;
    return NS_OK;
}


//
// compzillaIWindowPropertyEvent implementation...
//

compzillaWindowEvent::compzillaWindowEvent(compzillaIWindow *window,
                                           long atom,
                                           bool deleted,
                                           nsIPropertyBag2 *bag)
    : mWindow(window),
      mAtom(atom),
      mDeleted(deleted),
      mBag(bag)
{
}


NS_IMETHODIMP
compzillaWindowEvent::GetAtom(PRInt32 *aAtom)
{
    *aAtom = mAtom;
    return NS_OK;
}


NS_IMETHODIMP 
compzillaWindowEvent::GetDeleted(PRBool *aDeleted)
{
    *aDeleted = mDeleted;
    return NS_OK;
}


NS_IMETHODIMP 
compzillaWindowEvent::GetValue(nsIPropertyBag2 **aValue)
{
    *aValue = mBag;
    return NS_OK;
}


//
// nsIDOMEvent implementation...
//

NS_IMETHODIMP
compzillaWindowEvent::GetType (nsAString &aType) 
{
    aType = NS_LITERAL_STRING ("Events"); 
    return NS_OK; 
}


NS_IMETHODIMP
compzillaWindowEvent::GetTarget (nsIDOMEventTarget **aTarget) 
{
    *aTarget = mTarget; 
    return NS_OK;
}


NS_IMETHODIMP
compzillaWindowEvent::GetCurrentTarget (nsIDOMEventTarget **aCurrentTarget) 
{
    *aCurrentTarget = mTarget; 
    return NS_OK;
}


NS_IMETHODIMP
compzillaWindowEvent::GetEventPhase (PRUint16 *aEventPhase) 
{
    *aEventPhase = BUBBLING_PHASE;
    return NS_OK; 
}


NS_IMETHODIMP
compzillaWindowEvent::GetBubbles (PRBool *aBubbles) 
{
    *aBubbles = true;
    return NS_OK;
}


NS_IMETHODIMP
compzillaWindowEvent::GetCancelable (PRBool *aCancelable) 
{
    *aCancelable = false;
    return NS_OK;
}


NS_IMETHODIMP
compzillaWindowEvent::GetTimeStamp (DOMTimeStamp *aTimeStamp) 
{ 
    *aTimeStamp = 0;
    return NS_OK;
}


NS_IMETHODIMP
compzillaWindowEvent::StopPropagation(void)
{ 
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
compzillaWindowEvent::PreventDefault(void) 
{ 
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
compzillaWindowEvent::InitEvent(const nsAString & eventTypeArg, 
                                PRBool canBubbleArg, 
                                PRBool cancelableArg) 
{
    return NS_OK;
}
