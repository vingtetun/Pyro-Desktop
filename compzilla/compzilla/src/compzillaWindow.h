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

#include "compzillaIWindowManager.h"
#include "compzillaIWindow.h"
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
      public nsIDOMEventTarget,
      public compzillaIWindow
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIDOMEVENTLISTENER
    NS_DECL_NSIDOMEVENTTARGET
    NS_DECL_COMPZILLAIWINDOW

    compzillaWindow (Display *display, Window window, compzillaIWindowManager* wm);
    virtual ~compzillaWindow ();

    void EnsurePixmap ();

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

    nsCOMArray<nsISupports> mContentNodes;
    Display *mDisplay;
    Window mWindow;
    XWindowAttributes mAttr;
    Pixmap mPixmap;
    Damage mDamage;
    Window mLastEntered;

    void DestroyWindow ();
    void MapWindow ();
    void UnmapWindow ();
    void PropertyChanged (Window win, Atom prop);
    void WindowDamaged (XRectangle *rect);
    void WindowConfigured (PRInt32 x, PRInt32 y,
                           PRInt32 width, PRInt32 height,
                           PRInt32 border,
                           compzillaWindow *abovewin);

    compzillaEventManager mDestroyEvMgr;
    compzillaEventManager mMoveResizeEvMgr;
    compzillaEventManager mShowEvMgr;
    compzillaEventManager mHideEvMgr;
    compzillaEventManager mPropertyChangeEvMgr;

 private:
    compzillaIWindowManager* mWM;

    void OnMouseMove (nsIDOMEvent* aDOMEvent);
    void OnDOMMouseScroll (nsIDOMEvent* aDOMEvent);
    void SendKeyEvent (int eventType, nsIDOMKeyEvent *keyEv);
    void SendMouseEvent (int eventType, nsIDOMMouseEvent *mouseEv, bool isScroll = false);
    void ConnectListeners (bool connect, nsCOMPtr<nsISupports> aContent);
    void TranslateClientXYToWindow (int *x, int *y, nsIDOMEventTarget *target);
    Window GetSubwindowAtPoint (int *x, int *y);
    unsigned int DOMKeyCodeToKeySym (PRUint32 vkCode);
};

