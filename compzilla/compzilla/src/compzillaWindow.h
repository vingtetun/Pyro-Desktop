/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*- */


#ifndef compzillaWindow_h___
#define compzillaWindow_h___


#include <nsCOMPtr.h>
#include <nsCOMArray.h>
#include <nsIDOMDocument.h>
#include <nsIDOMKeyEvent.h>      // unstable
#include <nsIDOMKeyListener.h>   // unstable
#include <nsIDOMMouseEvent.h>
#include <nsIDOMMouseListener.h> // unstable
#include <nsIDOMUIListener.h>    // unstable

extern "C" {
#include <X11/Xlib.h>
#include <X11/extensions/Xdamage.h>
}

#include "compzillaIRenderingContextInternal.h"
#include "compzillaIWindow.h"
#include "compzillaIWindowObserver.h"


// From X.h
#define _KeyPress 2
#define _FocusIn 9
#define _FocusOut 10

// Avoid name clashes between X.h constants and Mozilla ifaces
#undef KeyPress
#undef FocusIn
#undef FocusOut


class compzillaWindow
    : public compzillaIWindow,
      public nsIDOMKeyListener,
      public nsIDOMMouseListener,
      public nsIDOMUIListener
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_COMPZILLAIWINDOW
    NS_DECL_NSIDOMEVENTLISTENER

    compzillaWindow (Display *display, 
                     Window window,
                     XWindowAttributes *attrs);
    virtual ~compzillaWindow ();

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

    void Destroyed ();
    void Mapped (bool override_redirect);
    void Unmapped ();
    void PropertyChanged (Atom prop, bool deleted);
    void Damaged (XRectangle *rect);
    void Configured (bool isNotify,
                     PRInt32 x, PRInt32 y,
                     PRInt32 width, PRInt32 height,
                     PRInt32 border,
                     compzillaWindow *aboveWin,
                     bool override_redirect);
    void ClientMessaged (Atom type, int format, long *data/*[5]*/);

    void QueueResize (PRInt32 x, PRInt32 y, PRInt32 width, PRInt32 height, PRInt32 border);

    XWindowAttributes mAttr;

 private:
    void OnMouseMove (nsIDOMEvent* aDOMEvent);
    void OnDOMMouseScroll (nsIDOMEvent* aDOMEvent);
    void SendKeyEvent (int eventType, nsIDOMKeyEvent *keyEv);
    void SendMouseEvent (int eventType, nsIDOMMouseEvent *mouseEv, bool isScroll = false);
    void ConnectListeners (bool connect, nsCOMPtr<nsISupports> aContent);
    void TranslateClientXYToWindow (int *x, int *y, nsIDOMEventTarget *target);
    Window GetSubwindowAtPoint (int *x, int *y);
    unsigned int DOMKeyCodeToKeySym (PRUint32 vkCode);

    void RedrawContentNode (nsIDOMHTMLCanvasElement *aContent, XRectangle *rect);

    void UpdateAttributes ();

    void RedirectWindow ();
    void UnredirectWindow ();
    void BindWindow ();
    void ReleaseWindow ();
    void Resized (PRInt32 x, PRInt32 y, PRInt32 width, PRInt32 height, PRInt32 border);
    void SendPendingResize ();

    nsresult GetAtomProperty (Atom prop, PRUint32* value);
    nsresult GetUTF8StringProperty (Atom prop, nsACString& utf8Value);
    nsresult GetCardinalListProperty (Atom prop, PRUint32 **values, PRUint32 expected_nitems);

    nsCOMArray<nsIDOMHTMLCanvasElement> mContentNodes;
    nsCOMArray<compzillaIWindowObserver> mObservers;
    Display *mDisplay;
    Window mWindow;

    Pixmap mPixmap;
    Damage mDamage;
    Window mLastEntered;

    bool mIsDestroyed;
    bool mIsRedirected;
    bool mIsResizePending;
    XWindowChanges mPendingChanges;

    // Used to store the keycode during a keydown event, and to reuse it during
    // the keypress to send to X. Otherwise it looks like the keycode is wrong
    // and the wrong key is transmitted to X.
    PRUint32 mKeycode;
};


nsresult CZ_NewCompzillaWindow(Display *display,
                               Window win,
                               XWindowAttributes *attrs,
                               compzillaWindow **retval);


#endif
