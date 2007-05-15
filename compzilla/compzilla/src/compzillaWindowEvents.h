/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*- */


#ifndef compzillaWindowEvents_h__
#define compzillaWindowEvents_h__


#include <nsCOMPtr.h>
#include <nsIDOMEvent.h>
#include <nsIPropertyBag2.h>
#include <nsRect.h>


#include "compzillaEventManager.h"
#include "compzillaIWindow.h"
#include "compzillaIWindowEvents.h"


// FIXME: Should split this into multiple implementations, but I'm too
//        lazy for now...

class compzillaWindowEvent 
    : // public compzillaIWindowEvent, /* Remove ambiguity warning */
      public compzillaIWindowConfigureEvent,
      public compzillaIWindowPropertyEvent
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIDOMEVENT
    NS_DECL_COMPZILLAIWINDOWEVENT
    NS_DECL_COMPZILLAIWINDOWCONFIGUREEVENT
    NS_DECL_COMPZILLAIWINDOWPROPERTYEVENT

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

    nsresult Send(const nsString& type, 
                  nsIDOMEventTarget *eventTarget,
                  compzillaEventManager& eventMgr);

private:
    virtual ~compzillaWindowEvent();

protected:
    nsCOMPtr<compzillaIWindow> mWindow;
    nsCOMPtr<nsIDOMEventTarget> mTarget;

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
    virtual ~compzillaWindowConfigureEvent();

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
    virtual ~compzillaWindowPropertyEvent();

protected:
    /* additional members */
};
#endif

nsresult CZ_NewCompzillaWindowEvent (compzillaIWindow *win, compzillaWindowEvent **retval);
nsresult CZ_NewCompzillaPropertyChangeEvent (compzillaIWindow *win, long atom, bool deleted,
                                             nsIPropertyBag2 *bag,
                                             compzillaWindowEvent **retval);
nsresult CZ_NewCompzillaConfigureEvent (compzillaIWindow *window,
                                        bool mapped,
                                        bool override,
                                        long x,
                                        long y,
                                        long width,
                                        long height,
                                        long borderWidth,
                                        compzillaIWindow *aboveWin,
                                        compzillaWindowEvent **retval);
#endif
