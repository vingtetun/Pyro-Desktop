/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#define MOZILLA_INTERNAL_API
#include <nsMemory.h>
#include "compzillaWindowEvents.h"

#if WITH_SPEW
#define SPEW(format...) printf("   - " format)
#else
#define SPEW(format...)
#endif

#define INFO(format...) printf(" *** " format)
#define WARNING(format...) printf(" !!! " format)
#define ERROR(format...) fprintf(stderr, format)

nsresult 
CZ_NewCompzillaWindowEvent (compzillaIWindow *win, compzillaWindowEvent **retval)
{
    compzillaWindowEvent *ev = new compzillaWindowEvent (win);
    if (!ev)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(ev);

    SPEW ("CZ_NewCompzillaWindowEvent = %p\n, win=%p\n", ev, win);

    *retval = ev;
    return NS_OK;
}


nsresult 
CZ_NewCompzillaPropertyChangeEvent (compzillaIWindow *win, 
                                    long atom, 
                                    bool deleted,
                                    compzillaWindowEvent **retval)
{
    compzillaWindowEvent *ev = new compzillaWindowPropertyEvent (win, atom, deleted);
    if (!ev)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(ev);

    SPEW ("CZ_NewCompzillaPropertyChangeEvent = %p, win=%p\n", ev, win);

    *retval = ev;
    return NS_OK;
}


nsresult 
CZ_NewCompzillaConfigureEvent (compzillaIWindow *win,
                               bool mapped,
                               bool override,
                               long x,
                               long y,
                               long width,
                               long height,
                               long borderWidth,
                               compzillaIWindow *aboveWin,
                               compzillaWindowEvent **retval)
{
    compzillaWindowEvent *ev = new compzillaWindowConfigureEvent (win, mapped,
                                                                  override,
                                                                  x, y,
                                                                  width, height,
                                                                  borderWidth,
                                                                  aboveWin);
    if (!ev)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(ev);

    SPEW ("CZ_NewCompzillaConfigureEvent = %p, win=%p\n", ev, win);

    *retval = ev;
    return NS_OK;
}

nsresult
CZ_NewCompzillaClientMessageEvent (compzillaIWindow *win,
                                   long type,
                                   long format,
                                   long d1,
                                   long d2,
                                   long d3,
                                   long d4,
                                   long d5,
                                   compzillaWindowEvent **retval)
{
    compzillaWindowEvent *ev = new compzillaWindowClientMessageEvent (win,
                                                                      type, format,
                                                                      d1, d2, d3, d4, d5);

    if (!ev)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(ev);

    SPEW ("CZ_NewCompzillaClientMessageEvent = %p, win=%p\n", ev, win);

    *retval = ev;
    return NS_OK;
}


//
// compzillaIWindowEvent implementation...
//

NS_IMPL_ADDREF(compzillaWindowEvent)
NS_IMPL_RELEASE(compzillaWindowEvent)
NS_INTERFACE_MAP_BEGIN(compzillaWindowEvent)
    NS_INTERFACE_MAP_ENTRY(nsISupports)
    NS_INTERFACE_MAP_ENTRY(nsIDOMEvent)
    NS_INTERFACE_MAP_ENTRY(compzillaIWindowEvent)
    NS_IMPL_QUERY_CLASSINFO(compzillaWindowEvent)
NS_INTERFACE_MAP_END
NS_IMPL_CI_INTERFACE_GETTER2(compzillaWindowEvent, 
                             nsIDOMEvent,
                             compzillaIWindowEvent)


compzillaWindowEvent::compzillaWindowEvent(compzillaIWindow *window)
    : mWindow(window)
{
}


compzillaWindowEvent::~compzillaWindowEvent()
{
    SPEW ("compzillaWindowEvent::~compzillaWindowEvent %p\n", this);
    SPEW ("compzillaWindowEvent::~compzillaWindowEvent RELEASING WINDOW\n", this);
    mWindow = NULL;
    SPEW ("compzillaWindowEvent::~compzillaWindowEvent RELEASING TARGET\n", this);
    mTarget = NULL;
}


//
// compzillaIWindowEvent implementation...
//

NS_IMETHODIMP 
compzillaWindowEvent::GetWindow(nsISupports **aWindow)
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



//
// compzillaIWindowConfigureEvent implementation...
//

NS_IMPL_ISUPPORTS_INHERITED1 (compzillaWindowConfigureEvent,
                              compzillaWindowEvent,
                              compzillaIWindowConfigureEvent)

compzillaWindowConfigureEvent::compzillaWindowConfigureEvent (compzillaIWindow *window,
                                                              bool mapped,
                                                              bool override,
                                                              long x,
                                                              long y,
                                                              long width,
                                                              long height,
                                                              long borderWidth,
                                                              compzillaIWindow *aboveWin)
    : compzillaWindowEvent(window),
      mMapped(mapped),
      mOverrideRedirect(override),
      mRect(x, y, width, height),
      mBorderWidth(borderWidth),
      mAboveWindow(aboveWin)
{
}

compzillaWindowConfigureEvent::~compzillaWindowConfigureEvent ()
{
    SPEW ("compzillaWindowConfigureEvent::~compzillaWindowConfigureEvent %p\n", this);
    SPEW ("compzillaWindowConfigureEvent::~compzillaWindowConfigureEvent RELEASING ABOVEWIN\n", this);
    mAboveWindow = NULL;
}

NS_IMETHODIMP
compzillaWindowConfigureEvent::GetMapped (PRBool *aMapped)
{
    *aMapped = mMapped;
    return NS_OK;
}


NS_IMETHODIMP
compzillaWindowConfigureEvent::GetOverrideRedirect (PRBool *aOverrideRedirect)
{
    *aOverrideRedirect = mOverrideRedirect;
    return NS_OK;
}


NS_IMETHODIMP
compzillaWindowConfigureEvent::GetX (PRInt32 *aX)
{
    *aX = mRect.x;
    return NS_OK;
}


NS_IMETHODIMP
compzillaWindowConfigureEvent::GetY (PRInt32 *aY)
{
    *aY = mRect.y;
    return NS_OK;
}


NS_IMETHODIMP
compzillaWindowConfigureEvent::GetWidth (PRInt32 *aWidth)
{
    *aWidth = mRect.width;
    return NS_OK;
}


NS_IMETHODIMP
compzillaWindowConfigureEvent::GetHeight (PRInt32 *aHeight)
{
    *aHeight = mRect.height;
    return NS_OK;
}


NS_IMETHODIMP
compzillaWindowConfigureEvent::GetBorderWidth (PRInt32 *aBorderWidth)
{
    *aBorderWidth = mBorderWidth;
    return NS_OK;
}


NS_IMETHODIMP
compzillaWindowConfigureEvent::GetAboveWindow (nsISupports **aAboveWindow)
{
    *aAboveWindow = mAboveWindow;
    return NS_OK;
}


//
// compzillaIWindowPropertyEvent implementation...
//

NS_IMPL_ISUPPORTS_INHERITED1 (compzillaWindowPropertyEvent,
                              compzillaWindowEvent,
                              compzillaIWindowPropertyEvent)

compzillaWindowPropertyEvent::compzillaWindowPropertyEvent (compzillaIWindow *window,
                                                            long atom,
                                                            bool deleted)
    : compzillaWindowEvent(window),
      mAtom(atom),
      mDeleted(deleted)
{
}


compzillaWindowPropertyEvent::~compzillaWindowPropertyEvent ()
{
    SPEW ("compzillaWindowPropertyEvent::~compzillaWindowPropertyEvent %p\n", this);
}


NS_IMETHODIMP
compzillaWindowPropertyEvent::GetAtom (PRInt32 *aAtom)
{
    *aAtom = mAtom;
    return NS_OK;
}


NS_IMETHODIMP 
compzillaWindowPropertyEvent::GetDeleted (PRBool *aDeleted)
{
    *aDeleted = mDeleted;
    return NS_OK;
}

//
// compzillaIWindowClientMessageEvent implementation...
//

NS_IMPL_ISUPPORTS_INHERITED1 (compzillaWindowClientMessageEvent,
                              compzillaWindowEvent,
                              compzillaIWindowClientMessageEvent)

compzillaWindowClientMessageEvent::compzillaWindowClientMessageEvent (compzillaIWindow *window,
                                                                      long type,
                                                                      long format,
                                                                      long d1,
                                                                      long d2,
                                                                      long d3,
                                                                      long d4,
                                                                      long d5)

    : compzillaWindowEvent(window),
      mType(type),
      mFormat(format),
      mD1(d1),
      mD2(d2),
      mD3(d3),
      mD4(d4),
      mD5(d5)
{
}


compzillaWindowClientMessageEvent::~compzillaWindowClientMessageEvent ()
{
    SPEW ("compzillaWindowClientMessageEvent::~compzillaWindowClientMessageEvent %p\n", this);
}


NS_IMETHODIMP
compzillaWindowClientMessageEvent::GetMessageType (PRInt32 *aType)
{
    *aType = mType;
    return NS_OK;
}


NS_IMETHODIMP 
compzillaWindowClientMessageEvent::GetFormat (PRInt32 *aFormat)
{
    *aFormat = mFormat;
    return NS_OK;
}


NS_IMETHODIMP 
compzillaWindowClientMessageEvent::GetD1 (PRInt32 *aD1)
{
    *aD1 = mD1;
    return NS_OK;
}


NS_IMETHODIMP 
compzillaWindowClientMessageEvent::GetD2 (PRInt32 *aD2)
{
    *aD2 = mD2;
    return NS_OK;
}


NS_IMETHODIMP 
compzillaWindowClientMessageEvent::GetD3 (PRInt32 *aD3)
{
    *aD3 = mD3;
    return NS_OK;
}


NS_IMETHODIMP 
compzillaWindowClientMessageEvent::GetD4 (PRInt32 *aD4)
{
    *aD4 = mD4;
    return NS_OK;
}


NS_IMETHODIMP 
compzillaWindowClientMessageEvent::GetD5 (PRInt32 *aD5)
{
    *aD5 = mD5;
    return NS_OK;
}
