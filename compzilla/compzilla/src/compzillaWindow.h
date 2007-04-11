/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#include <nsCOMPtr.h>
#include <nsIDOMKeyListener.h>
#include <nsIDOMMouseEvent.h>
#include <nsIDOMMouseListener.h>
#include <nsIDOMUIListener.h>

extern "C" {
#include <X11/Xlib.h>
#include <X11/extensions/Xdamage.h>
#include <X11/extensions/Xrender.h>
}


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
      public nsIDOMUIListener
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIDOMEVENTLISTENER

    compzillaWindow (Display *display, Window window);
    virtual ~compzillaWindow ();

    void EnsurePixmap ();
    void SetContent (nsCOMPtr<nsISupports> aContent);
    void TranslateClientXYToWindow (int *x, int *y);

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
    Window mWindow;
    Display *mDisplay;
    XWindowAttributes mAttr;
    Damage mDamage;
    Pixmap mPixmap;
    compzillaWindow *mFrame;

    Window mLastEntered;

 private:
    void SendMouseEvent (int eventType, nsIDOMMouseEvent *mouseEv);
    void ConnectListeners (bool connect);
    Window GetSubwindowAtPoint (int *x, int *y);
};

