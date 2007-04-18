/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#define MOZILLA_INTERNAL_API

#include "compzillaWindow.h"

#include <nsIDOMEventTarget.h>
#include <nsICanvasElement.h>
#include <nsISupportsUtils.h>
#if MOZILLA_1_8_BRANCH
#include <nsStringAPI.h>
#else
#include <nsXPCOMStrings.h>
#endif

extern "C" {
#include <stdio.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xcomposite.h>
#if HAVE_XEVIE
#include <X11/extensions/Xevie.h>
#endif

extern uint32 gtk_get_current_event_time (void);
}


#define DEBUG(format...) printf("   - " format)
#define INFO(format...) printf(" *** " format)
#define WARNING(format...) printf(" !!! " format)
#define ERROR(format...) fprintf(stderr, format)


NS_IMPL_ADDREF(compzillaWindow)
NS_IMPL_RELEASE(compzillaWindow)
NS_INTERFACE_MAP_BEGIN(compzillaWindow)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMKeyListener)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsIDOMEventListener, nsIDOMKeyListener)
    NS_INTERFACE_MAP_ENTRY(nsIDOMKeyListener)
    NS_INTERFACE_MAP_ENTRY(nsIDOMMouseListener)
    NS_INTERFACE_MAP_ENTRY(nsIDOMUIListener)
NS_INTERFACE_MAP_END


compzillaWindow::compzillaWindow(Display *display, Window win)
    : mContent(),
      mDisplay(display),
      mWindow(win),
      mPixmap(nsnull)
{
    NS_INIT_ISUPPORTS();

    /* 
     * Set up damage notification.  RawRectangles gives us smaller grain
     * changes, versus NonEmpty which seems to always include the entire
     * contents.
     */
    mDamage = XDamageCreate (display, win, XDamageReportRawRectangles);

    // Redirect output entirely
    XCompositeRedirectWindow (display, win, CompositeRedirectManual);

    // Compiz selects only these.  Need more?
    XSelectInput (display, win, (PropertyChangeMask | EnterWindowMask | FocusChangeMask));
    XShapeSelectInput (display, win, ShapeNotifyMask);

    EnsurePixmap ();
}


compzillaWindow::~compzillaWindow()
{
    WARNING ("compzillaWindow::~compzillaWindow %p, xid=%p\n", this, mWindow);

    // Don't send events
    // FIXME: This crashes for some reason
    //ConnectListeners (false);

    // Let the window render itself
    XCompositeUnredirectWindow (mDisplay, mWindow, CompositeRedirectManual);

    if (mDamage) {
        XDamageDestroy(mDisplay, mDamage);
    }
    if (mPixmap) {
        XFreePixmap(mDisplay, mPixmap);
    }

    /* 
     * This is the only case where a window is removed but not
     * destroyed. We must remove our event mask and all passive
     * grabs.  -- compiz
     */
    XSelectInput (mDisplay, mWindow, NoEventMask);
    XShapeSelectInput (mDisplay, mWindow, NoEventMask);
    XUngrabButton (mDisplay, AnyButton, AnyModifier, mWindow);
}


void
compzillaWindow::EnsurePixmap()
{
    if (!mPixmap) {
        XGrabServer (mDisplay);

        XGetWindowAttributes(mDisplay, mWindow, &mAttr);
        if (mAttr.map_state == IsViewable) {
            mPixmap = XCompositeNameWindowPixmap (mDisplay, mWindow);
        }

        XUngrabServer (mDisplay);
    }
}


void
compzillaWindow::ConnectListeners (bool connect)
{
    const char *events[] = {
	// nsIDOMKeyListener events
	// NOTE: These aren't delivered due to WM focus issues currently
	"keydown",
	"keyup",
	//"keypress",

	// nsIDOMMouseListener events
	"mousedown",
	"mouseup",
	"mouseover",
	"mouseout",
	"mousein",
	//"click",
	//"dblclick",

	// nsIDOMUIListener events
	//"activate",
	"focusin",
	"focusout",
        "DOMFocusIn",
        "DOMFocusOut",
        "resize",

	// HandleEvent events
	"focus",
	"blur",
	"mousemove",
	"DOMMouseScroll",

	NULL
    };

    if (!mContent) {
	return;
    }

    nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface (mContent);
    nsCOMPtr<nsIDOMEventListener> listener = 
	do_QueryInterface (NS_ISUPPORTS_CAST (nsIDOMKeyListener *, this));

    if (!target || !listener) {
	return;
    }

    for (int i = 0; events [i]; i++) {
	nsString evname = NS_ConvertASCIItoUTF16 (events [i]);
	if (connect) {
	    target->AddEventListener (evname, listener, PR_TRUE);
	} else {
	    target->RemoveEventListener (evname, listener, PR_TRUE);
	}
    }
}


void 
compzillaWindow::SetContent (nsCOMPtr<nsISupports> aContent)
{
    // FIXME: This crashes for some reason
    //ConnectListeners (false);

    mContent = aContent;
    ConnectListeners (true);
}


// All of the event listeners below return NS_OK to indicate that the
// event should not be consumed in the default case.

NS_IMETHODIMP
compzillaWindow::HandleEvent (nsIDOMEvent* aDOMEvent)
{
    nsString type;
    aDOMEvent->GetType (type);
    NS_LossyConvertUTF16toASCII ctype(type);
    const char *cdata;
    NS_CStringGetData (ctype, &cdata);

    if (strcmp (cdata, "mousemove") == 0) {
        OnMouseMove (aDOMEvent);
    } else if (strcmp (cdata, "DOMMouseScroll") == 0) {
        OnDOMMouseScroll (aDOMEvent);
    } else if (strcmp (cdata, "focus") == 0) {
        FocusIn (aDOMEvent);
    } else if (strcmp (cdata, "blur") == 0) {
        FocusOut (aDOMEvent);
    } else {
        nsCOMPtr<nsIDOMEventTarget> target;
        aDOMEvent->GetTarget (getter_AddRefs (target));

        WARNING ("compzillaWindow::HandleEvent: unhandled type=%s, target=%p!!!\n", 
                 cdata, target.get ());
    }

    return NS_OK;
}


void
compzillaWindow::SendKeyEvent (int eventType, nsIDOMKeyEvent *keyEv)
{
    DOMTimeStamp timestamp;
    PRBool ctrl, shift, alt, meta;
    int state = 0;
    PRUint32 keycode;

    keyEv->GetTimeStamp (&timestamp);
    if (!timestamp) {
        timestamp = gtk_get_current_event_time (); // CurrentTime;
    }

    keyEv->GetAltKey (&alt);
    keyEv->GetCtrlKey (&ctrl);
    keyEv->GetShiftKey (&shift);
    keyEv->GetMetaKey (&meta);

    if (ctrl) {
        state |= ControlMask;
    }
    if (shift) {
        state |= ShiftMask;
    }
    if (alt) {
        state |= Mod1Mask;
    }
    if (meta) {
        state |= Mod2Mask;
    }

    keyEv->GetKeyCode (&keycode);

    // Build up the XEvent we will send
    XEvent xev = { 0 };
    xev.xkey.type = eventType;
    xev.xkey.serial = 0;
    xev.xkey.display = mDisplay;
    xev.xkey.window = mWindow;
    xev.xkey.root = mAttr.root;
    xev.xkey.time = timestamp;
    xev.xkey.state = state;
    //xev.xkey.keycode = keycode;
    xev.xkey.keycode = 42;
    xev.xkey.same_screen = True;

    // Figure out who to send to
    long xevMask;

    switch (eventType) {
    case _KeyPress:
	xevMask = KeyPressMask;
	break;
    case KeyRelease:
	xevMask = KeyReleaseMask;
	break;
    default:
        NS_NOTREACHED ("Unknown eventType");
        return;
    }

    WARNING ("compzillaWindow::SendKeyEvent: XSendEvent win=%p, child=%p, state=%p, "
             "keycode=%u, timestamp=%d\n", 
             mWindow, mWindow, state, keycode, timestamp);

    XSendEvent (mDisplay, mWindow, True, xevMask, &xev);

    keyEv->StopPropagation ();
    keyEv->PreventDefault ();
}


NS_IMETHODIMP
compzillaWindow::KeyDown (nsIDOMEvent* aDOMEvent)
{
    nsCOMPtr<nsIDOMKeyEvent> keyEv = do_QueryInterface (aDOMEvent);
    NS_ASSERTION (keyEv, "Invalid key event");

    SendKeyEvent (_KeyPress, keyEv);

    WARNING ("compzillaWindow::KeyDown: Handled\n");
    return NS_OK;
}


NS_IMETHODIMP
compzillaWindow::KeyUp (nsIDOMEvent* aDOMEvent)
{
    nsCOMPtr<nsIDOMKeyEvent> keyEv = do_QueryInterface (aDOMEvent);
    NS_ASSERTION (keyEv, "Invalid key event");

    SendKeyEvent (KeyRelease, keyEv);

    WARNING ("compzillaWindow::KeyUp: Handled\n");
    return NS_OK;
}


NS_IMETHODIMP
compzillaWindow::KeyPress(nsIDOMEvent* aDOMEvent)
{
    WARNING ("compzillaWindow::KeyPress: Ignored!!!\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}


void
compzillaWindow::TranslateClientXYToWindow (int *x, int *y)
{
    nsCOMPtr<nsICanvasElement> canvasElement = do_QueryInterface (mContent);
    if (!canvasElement) {
	return;
    }

    // FIXME: This is probably broken and not robust

    nsIFrame *frame;
    canvasElement->GetPrimaryCanvasFrame (&frame);
    if (!frame) {
	return;
    }

    nsIntRect screenRect = frame->GetScreenRectExternal ();
    *x = *x - screenRect.x;
    *y = *y - screenRect.y;
}


Window
compzillaWindow::GetSubwindowAtPoint (int *x, int *y)
{
    Window last_child, child, new_child;
    last_child = child = mWindow;

    while (child) {
        XTranslateCoordinates (mDisplay, 
                               last_child, 
                               child, 
                               *x, *y, x, y, 
                               &new_child);

        last_child = child;
        child = new_child;
    }

    return last_child;
}


void
compzillaWindow::SendMouseEvent (int eventType, nsIDOMMouseEvent *mouseEv, bool isScroll)
{
    DOMTimeStamp timestamp;
    PRUint16 button;
    int x, y;
    int x_root, y_root;
    PRBool ctrl, shift, alt, meta;
    int state = 0;

    mouseEv->GetTimeStamp (&timestamp);
    if (!timestamp) {
        timestamp = gtk_get_current_event_time (); // CurrentTime;
    }

    mouseEv->GetScreenX (&x_root);
    mouseEv->GetScreenY (&y_root);
    mouseEv->GetClientX (&x);
    mouseEv->GetClientY (&y);
    TranslateClientXYToWindow (&x, &y);

    mouseEv->GetButton (&button);

    if (isScroll) {
        /* 
         * For button down/up GetDetail is the number of clicks.  For
         * DOMMouseScroll, it's the scroll amount as +3/-3.
         */
        PRInt32 evDetail = 0;
        mouseEv->GetDetail (&evDetail);

        if (evDetail < 0) {
            button = 3; // Turns into button 4
        } else if (evDetail > 0) {
            button = 4; // Turns into button 5
        }
    }

    mouseEv->GetCtrlKey (&ctrl);
    mouseEv->GetShiftKey (&shift);
    mouseEv->GetAltKey (&alt);
    mouseEv->GetMetaKey (&meta);

    if (ctrl) {
        state |= ControlMask;
    }
    if (shift) {
        state |= ShiftMask;
    }
    if (alt) {
        state |= Mod1Mask;
    }
    if (meta) {
        state |= Mod2Mask;
    }

    Window destChild = GetSubwindowAtPoint (&x, &y);

    if (destChild != mLastEntered && (eventType != EnterNotify)) {
        if (mLastEntered) {
            XEvent xev = { 0 };
            xev.xcrossing.type = LeaveNotify;
            xev.xcrossing.serial = 0;
            xev.xcrossing.display = mDisplay;
            xev.xcrossing.window = mLastEntered;
            xev.xcrossing.root = mAttr.root;
            xev.xcrossing.time = timestamp;
            xev.xcrossing.state = state;
            xev.xcrossing.x = x;
            xev.xcrossing.y = y;
            xev.xcrossing.x_root = x_root;
            xev.xcrossing.y_root = y_root;
            xev.xcrossing.same_screen = True;

            XSendEvent (mDisplay, mLastEntered, True, LeaveWindowMask, &xev);
            //XevieSendEvent(mDisplay, &xev, XEVIE_MODIFIED);
        }
        
        mLastEntered = destChild;
        SendMouseEvent (EnterNotify, mouseEv);
    }

    // Build up the XEvent we will send
    XEvent xev = { 0 };
    xev.xany.type = eventType;

    switch (eventType) {
    case ButtonPress:
    case ButtonRelease:
        xev.xbutton.display = mDisplay;
        xev.xbutton.window = destChild;
	//xev.xbutton.window = mWindow;
        //xev.xbutton.subwindow = destChild;
	xev.xbutton.root = mAttr.root;
	xev.xbutton.time = timestamp;
	xev.xbutton.state = eventType == ButtonRelease ? Button1Mask << button : 0;
	xev.xbutton.button = button + 1; // DOM buttons start at 0
	xev.xbutton.x = x;
	xev.xbutton.y = y;
	xev.xbutton.x_root = x_root;
	xev.xbutton.y_root = y_root;
	xev.xbutton.same_screen = True;
	break;
    case MotionNotify:
        xev.xmotion.display = mDisplay;
        xev.xmotion.window = destChild;
	//xev.xmotion.window = mWindow;
        //xev.xmotion.subwindow = destChild;
	xev.xmotion.root = mAttr.root;
	xev.xmotion.time = timestamp;
	xev.xmotion.state = state;
	xev.xmotion.x = x;
	xev.xmotion.y = y;
	xev.xmotion.x_root = x_root;
	xev.xmotion.y_root = y_root;
	xev.xmotion.same_screen = True;
	break;
    case EnterNotify:
        //XSetInputFocus (mDisplay, mWindow, RevertToParent, timestamp);
        // nobreak
    case LeaveNotify:
        xev.xcrossing.display = mDisplay;
        xev.xcrossing.window = destChild;
        //xev.xcrossing.window = mWindow;
        //xev.xcrossing.subwindow = destChild;
	xev.xcrossing.root = mAttr.root;
	xev.xcrossing.time = timestamp;
	xev.xcrossing.state = state;
	xev.xcrossing.x = x;
	xev.xcrossing.y = y;
	xev.xcrossing.x_root = x_root;
	xev.xcrossing.y_root = y_root;
	xev.xcrossing.same_screen = True;
	break;
    default:
        NS_NOTREACHED ("Unknown eventType");
	return;
    }

    // Figure out who to send to
    long xevMask;

    switch (eventType) {
    case ButtonPress:
	xevMask = ButtonPressMask;
	break;
    case ButtonRelease:
	xevMask = ButtonReleaseMask;
	break;
    case MotionNotify:
	xevMask = (PointerMotionMask | PointerMotionHintMask);
	if (button) {
            xevMask |= ButtonMotionMask;
            xevMask |= Button1MotionMask << button;
	}
	break;
    case EnterNotify:
	xevMask = EnterWindowMask;
	break;
    case LeaveNotify:
	xevMask = LeaveWindowMask;
	break;
    default:
        NS_NOTREACHED ("Unknown eventType");
	return;
    }

    DEBUG ("compzillaWindow::SendMouseEvent: XSendEvent win=%p, child=%p, x=%d, y=%d, state=%p, "
           "button=%u, timestamp=%d\n", 
	   mWindow, destChild, x, y, state, button + 1, timestamp);

    XSendEvent (mDisplay, destChild, True, xevMask, &xev);
    //XevieSendEvent(mDisplay, &xev, XEVIE_MODIFIED);

    // Stop processing event
    if (eventType != MotionNotify) {
        mouseEv->StopPropagation ();
        mouseEv->PreventDefault ();
    }
}


NS_IMETHODIMP
compzillaWindow::MouseDown (nsIDOMEvent* aDOMEvent)
{
    nsCOMPtr<nsIDOMMouseEvent> mouseEv = do_QueryInterface (aDOMEvent);
    SendMouseEvent (ButtonPress, mouseEv);

    WARNING ("compzillaWindow::MouseDown: Handled\n");

    return NS_OK;
}


NS_IMETHODIMP
compzillaWindow::MouseUp (nsIDOMEvent* aDOMEvent)
{
    nsCOMPtr<nsIDOMMouseEvent> mouseEv = do_QueryInterface (aDOMEvent);
    SendMouseEvent (ButtonRelease, mouseEv);
    WARNING ("compzillaWindow::MouseUp: Handled\n");
    return NS_OK;
}


NS_IMETHODIMP
compzillaWindow::MouseClick (nsIDOMEvent* aDOMEvent)
{
    WARNING ("compzillaWindow::MouseClick: Ignored\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
compzillaWindow::MouseDblClick (nsIDOMEvent* aDOMEvent)
{
    WARNING ("compzillaWindow::MouseDblClick: Ignored\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
compzillaWindow::MouseOver (nsIDOMEvent* aDOMEvent)
{
    nsCOMPtr<nsIDOMMouseEvent> mouseEv = do_QueryInterface (aDOMEvent);
    SendMouseEvent (EnterNotify, mouseEv);
    WARNING ("compzillaWindow::MouseOver: Handled\n");
    return NS_OK;
}


NS_IMETHODIMP
compzillaWindow::MouseOut (nsIDOMEvent* aDOMEvent)
{
    nsCOMPtr<nsIDOMMouseEvent> mouseEv = do_QueryInterface (aDOMEvent);
    SendMouseEvent (LeaveNotify, mouseEv);
    WARNING ("compzillaWindow::MouseOut: Handled\n");
    return NS_OK;
}


NS_IMETHODIMP
compzillaWindow::Activate (nsIDOMEvent* aDOMEvent)
{
    WARNING ("compzillaWindow::Activate: W00T!!!\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
compzillaWindow::FocusIn (nsIDOMEvent* aDOMEvent)
{
    WARNING ("compzillaWindow::FocusIn: win=%p\n", mWindow);

    XEvent xev = { 0 };
    xev.xfocus.type = _FocusIn;
    xev.xfocus.serial = 0;
    xev.xfocus.display = mDisplay;
    xev.xfocus.window = mWindow;
    xev.xfocus.mode = NotifyNormal;
    xev.xfocus.detail = NotifyAncestor;

    // Send FocusIn (to toplevel, or child?)
    XSendEvent (mDisplay, mWindow, True, FocusChangeMask, &xev);

    return NS_OK;
}


NS_IMETHODIMP
compzillaWindow::FocusOut (nsIDOMEvent* aDOMEvent)
{
    WARNING ("compzillaWindow::FocusOut: win=%p\n", mWindow);

    XEvent xev = { 0 };
    xev.xfocus.type = _FocusOut;
    xev.xfocus.serial = 0;
    xev.xfocus.display = mDisplay;
    xev.xfocus.window = mWindow;
    xev.xfocus.mode = NotifyNormal;
    xev.xfocus.detail = NotifyAncestor;

    // Send FocusOut (to toplevel, or child?)
    XSendEvent (mDisplay, mWindow, True, FocusChangeMask, &xev);

    return NS_OK;
}


void
compzillaWindow::OnMouseMove (nsIDOMEvent *aDOMEvent)
{
    nsCOMPtr<nsIDOMMouseEvent> mouseEv = do_QueryInterface (aDOMEvent);
    SendMouseEvent (MotionNotify, mouseEv);

    WARNING ("compzillaWindow::OnMouseMove: mouseEv=%p\n", mouseEv.get());
}


void
compzillaWindow::OnDOMMouseScroll (nsIDOMEvent *aDOMEvent)
{
    nsCOMPtr<nsIDOMMouseEvent> mouseEv = do_QueryInterface (aDOMEvent);

    WARNING ("compzillaWindow::OnDOMMouseScroll: mouseEv=%p\n", mouseEv.get());

    // NOTE: We don't receive events if them if a modifier key is down with
    //       Mozilla 1.8.

    SendMouseEvent (ButtonPress, mouseEv, true);

    // DOMMouseScroll has no Release equivalent, so fake it.
    SendMouseEvent (ButtonRelease, mouseEv, true);
}
