/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#include <nsCOMPtr.h>
#include <nsIDOMEventTarget.h>
#include <nsIDOMKeyEvent.h>
#include <nsIDOMKeyListener.h>
#include <nsIDOMMouseEvent.h>
#include <nsIDOMMouseListener.h>
#include <nsIDOMUIListener.h>

extern "C" {
#include <X11/Xlib.h>
#include <X11/extensions/Xdamage.h>
}


#include "compzillaEventManager.h"


// From X.h
#define _KeyPress 2
#define _FocusIn 9
#define _FocusOut 10

// Avoid name clashes between X.h constants and Mozilla ifaces
#undef KeyPress
#undef FocusIn
#undef FocusOut


class compzillaWindow
    : public nsIDOMKeyListener,
      public nsIDOMMouseListener,
      public nsIDOMUIListener,
      public nsIDOMEventTarget
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIDOMEVENTLISTENER
    NS_DECL_NSIDOMEVENTTARGET

    compzillaWindow (Display *display, Window window);
    virtual ~compzillaWindow ();

    void EnsurePixmap ();
    void SetContent (nsCOMPtr<nsISupports> aContent);

    // nsIDOMKeyListener

    NS_IMETHOD KeyDown (nsIDOMEvent* aDOMEvent);
    NS_IMETHOD KeyUp (nsIDOMEvent* aDOMEvent);
    NS_IMETHOD KeyPress (nsIDOMEvent* aDOMEvent);
    
    // nsIDOMMouseListener
    
    NS_IMETHOD MouseDown (nsIDOMEvent* aDOMEvent);
    NS_IMETHOD MouseUp (nsIDOMEvent* aDOMEvent);
    NS_IMETHOD MouseClick (nsIDOMEvent* aDOMEvent);
    NS_IMETHOD MouseDblClick (nsIDOMEvent* aDOMEvent);
    NS_IMETHOD MouseOver (nsIDOMEvent* aDOMEvent);
    NS_IMETHOD MouseOut (nsIDOMEvent* aDOMEvent);
    
    // nsIDOMUIListener
    
    NS_IMETHOD Activate (nsIDOMEvent* aDOMEvent);
    NS_IMETHOD FocusIn (nsIDOMEvent* aDOMEvent);
    NS_IMETHOD FocusOut (nsIDOMEvent* aDOMEvent);

    nsCOMPtr<nsISupports> mContent;
    Display *mDisplay;
    Window mWindow;
    XWindowAttributes mAttr;
    Pixmap mPixmap;
    Damage mDamage;
    Window mLastEntered;

    compzillaEventManager mDestroyEvMgr;
    compzillaEventManager mMoveResizeEvMgr;
    compzillaEventManager mShowEvMgr;
    compzillaEventManager mHideEvMgr;
    compzillaEventManager mPropertyChangeEvMgr;

 private:
    void OnMouseMove (nsIDOMEvent* aDOMEvent);
    void OnDOMMouseScroll (nsIDOMEvent* aDOMEvent);
    void SendKeyEvent (int eventType, nsIDOMKeyEvent *keyEv);
    void SendMouseEvent (int eventType, nsIDOMMouseEvent *mouseEv, bool isScroll = false);
    void ConnectListeners (bool connect);
    void TranslateClientXYToWindow (int *x, int *y);
    Window GetSubwindowAtPoint (int *x, int *y);
    unsigned int DOMKeyCodeToKeySym (PRUint32 vkCode);
};

