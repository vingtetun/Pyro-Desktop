/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*- */


#ifndef compzillaWindowEvents_h__
#define compzillaWindowEvents_h__


#include <nsCOMPtr.h>
#include <nsWeakPtr.h>
#include <nsIDOMEvent.h>
#include <nsRect.h>


#include "compzillaEventManager.h"
#include "compzillaIWindow.h"
#include "compzillaIWindowEvents.h"


class compzillaWindowEvent 
    : public compzillaIWindowEvent
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIDOMEVENT
    NS_DECL_COMPZILLAIWINDOWEVENT

    compzillaWindowEvent(compzillaIWindow *window);

    nsresult Send(const nsString& type, 
                  nsIDOMEventTarget *eventTarget,
                  compzillaEventManager& eventMgr);

protected:
    virtual ~compzillaWindowEvent();

    nsCOMPtr<nsISupports> mWindow;
    nsCOMPtr<nsIDOMEventTarget> mTarget;
};


class compzillaWindowConfigureEvent
    : public compzillaWindowEvent,
      public compzillaIWindowConfigureEvent
{
public:
    NS_DECL_ISUPPORTS_INHERITED
    NS_FORWARD_NSIDOMEVENT(compzillaWindowEvent::)
    NS_FORWARD_COMPZILLAIWINDOWEVENT(compzillaWindowEvent::)
    NS_DECL_COMPZILLAIWINDOWCONFIGUREEVENT

    compzillaWindowConfigureEvent (compzillaIWindow *window,
                                   bool mapped,
                                   bool override,
                                   long x,
                                   long y,
                                   long width,
                                   long height,
                                   long borderWidth,
                                   compzillaIWindow *aboveWin);

protected:
    virtual ~compzillaWindowConfigureEvent ();

private:
    bool mMapped;
    bool mOverrideRedirect;
    nsIntRect mRect;
    int mBorderWidth;
    nsCOMPtr<nsISupports> mAboveWindow;

};

class compzillaWindowPropertyEvent
    : public compzillaWindowEvent,
      public compzillaIWindowPropertyEvent
{
public:
    NS_DECL_ISUPPORTS_INHERITED
    NS_FORWARD_NSIDOMEVENT(compzillaWindowEvent::)
    NS_FORWARD_COMPZILLAIWINDOWEVENT(compzillaWindowEvent::)
    NS_DECL_COMPZILLAIWINDOWPROPERTYEVENT

    compzillaWindowPropertyEvent (compzillaIWindow *window,
                                  long atom,
                                  bool deleted);

protected:
    virtual ~compzillaWindowPropertyEvent ();

private:

    long mAtom;
    bool mDeleted;

};

class compzillaWindowClientMessageEvent
    : public compzillaWindowEvent,
      public compzillaIWindowClientMessageEvent
{
public:
    NS_DECL_ISUPPORTS_INHERITED
    NS_FORWARD_NSIDOMEVENT(compzillaWindowEvent::)
    NS_FORWARD_COMPZILLAIWINDOWEVENT(compzillaWindowEvent::)
    NS_DECL_COMPZILLAIWINDOWCLIENTMESSAGEEVENT

    compzillaWindowClientMessageEvent (compzillaIWindow *window /* can be null, meaning the root window */,
                                       long type,
                                       long format,
                                       long d1,
                                       long d2,
                                       long d3,
                                       long d4,
                                       long d5);

protected:
    virtual ~compzillaWindowClientMessageEvent ();

private:
    long mType;
    long mFormat;
    long mD1;
    long mD2;
    long mD3;
    long mD4;
    long mD5;
};


nsresult CZ_NewCompzillaWindowEvent (compzillaIWindow *win, 
                                     compzillaWindowEvent **retval);

nsresult CZ_NewCompzillaPropertyChangeEvent (compzillaIWindow *win, 
                                             long atom, 
                                             bool deleted,
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

nsresult CZ_NewCompzillaClientMessageEvent (compzillaIWindow *window,
                                            long type,
                                            long format,
                                            long d1,
                                            long d2,
                                            long d3,
                                            long d4,
                                            long d5,
                                            compzillaWindowEvent **retval);
#endif
