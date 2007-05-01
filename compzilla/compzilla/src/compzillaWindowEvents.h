/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*- */


#ifndef compzillaWindowEvents_h__
#define compzillaWindowEvents_h__


#include <nsCOMPtr.h>
#include <nsIDOMEvent.h>
#include <nsIPropertyBag2.h>
#include <nsRect.h>


#include "compzillaIWindow.h"
#include "compzillaIWindowEvents.h"


// FIXME: Should split this into multiple implementations to save space, but I'm too
//        lazy for now...

class compzillaWindowEvent 
    : // public compzillaIWindowEvent, /* Remove ambiguity warning */
      public compzillaIWindowConfigureEvent,
      public compzillaIWindowPropertyEvent
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_COMPZILLAIWINDOWEVENT
    NS_DECL_COMPZILLAIWINDOWCONFIGUREEVENT
    NS_DECL_COMPZILLAIWINDOWPROPERTYEVENT
    NS_FORWARD_NSIDOMEVENT(mInner->)

    compzillaWindowEvent(compzillaIWindow *window);
    compzillaWindowEvent(compzillaIWindow *window,
                         bool mapped,
                         bool override,
                         long x,
                         long y,
                         long width,
                         long height,
                         long borderWidth,
                         compzillaIWindow *aboveWin);
    compzillaWindowEvent(compzillaIWindow *window,
                         long atom,
                         bool deleted,
                         nsIPropertyBag2 *bag);

    void SetInner(nsIDOMEvent *inner);

private:
    ~compzillaWindowEvent();

protected:
    nsCOMPtr<compzillaIWindow> mWindow;
    nsCOMPtr<nsIDOMEvent> mInner;

    bool mMapped;
    bool mOverrideRedirect;
    nsIntRect mRect;
    int mBorderWidth;
    nsCOMPtr<compzillaIWindow> mAboveWindow;

    long mAtom;
    bool mDeleted;
    nsCOMPtr<nsIPropertyBag2> mBag;
};


#if LAZY
class compzillaWindowConfigureEvent
    : public compzillaIWindowConfigureEvent
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_COMPZILLAIWINDOWCONFIGUREEVENT

    compzillaWindowConfigureEvent();

private:
    ~compzillaWindowConfigureEvent();

protected:
    /* additional members */
};


class compzillaWindowPropertyEvent
    : public compzillaIWindowPropertyEvent
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_COMPZILLAIWINDOWPROPERTYEVENT

    compzillaWindowPropertyEvent();

private:
    ~compzillaWindowPropertyEvent();

protected:
    /* additional members */
};
#endif


#endif
