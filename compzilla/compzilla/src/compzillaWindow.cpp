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
    mDamage = XDamageCreate (display, win, XDamageReportNonEmpty);

    // Redirect output
    //XCompositeRedirectWindow (display, win, CompositeRedirectManual);
    XCompositeRedirectWindow (display, win, CompositeRedirectAutomatic);

    XSelectInput(display, win, (PropertyChangeMask | EnterWindowMask | FocusChangeMask));

    EnsurePixmap();
}


compzillaWindow::~compzillaWindow()
{
    WARNING ("compzillaWindow::~compzillaWindow %p, xid=%p\n", this, mWindow);

    // Don't send events
    // FIXME: This crashes for some reason
    //ConnectListeners (false);

    // Let the window render itself
    XCompositeUnredirectWindow (mDisplay, mWindow, CompositeRedirectAutomatic);

    if (mDamage) {
        XDamageDestroy(mDisplay, mDamage);
    }
    if (mPixmap) {
        XFreePixmap(mDisplay, mPixmap);
    }
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
	"keypress",

	// nsIDOMMouseListener events
	"mousedown",
	"mouseup",
	"mouseover",
	"mouseout",
	"mousein",
	"click",
	"dblclick",

	// nsIDOMUIListener events
	"activate",
	"focusin",
	"focusout",
        "DOMFocusIn",
        "DOMFocusOut",
        "resize",

	// HandleEvent events
	"mousemove",
	"DOMMouseScroll",

	NULL
    };

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
    const PRUnichar *widedata;
    aDOMEvent->GetType (type);
    NS_LossyConvertUTF16toASCII ctype(type);
    const char *cdata;
    NS_CStringGetData (ctype, &cdata);

    nsCOMPtr<nsIDOMEventTarget> target;
    aDOMEvent->GetTarget (getter_AddRefs (target));

    if (strcmp (cdata, "mousemove") == 0) {
        nsCOMPtr<nsIDOMMouseEvent> mouseEv = do_QueryInterface (aDOMEvent);
        SendMouseEvent (MotionNotify, mouseEv);
    } else {
        WARNING ("compzillaWindow::HandleEvent: type=%s, target=%p!!!\n", cdata, target.get ());
    }

    // Eat DOMMouseScroll events
    aDOMEvent->StopPropagation ();
    aDOMEvent->PreventDefault ();

    return NS_OK;
}


NS_IMETHODIMP
compzillaWindow::KeyDown (nsIDOMEvent* aDOMEvent)
{
    WARNING ("compzillaWindow::KeyDown: W00T!!!\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
compzillaWindow::KeyUp (nsIDOMEvent* aDOMEvent)
{
    WARNING ("compzillaWindow::KeyUp: W00T!!!\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
compzillaWindow::KeyPress(nsIDOMEvent* aDOMEvent)
{
    WARNING ("compzillaWindow::KeyPress: W00T!!!\n");
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
compzillaWindow::SendMouseEvent (int eventType, nsIDOMMouseEvent *mouseEv)
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
        XSetInputFocus (mDisplay, mWindow, RevertToParent, timestamp);
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
	ERROR ("compzillaWindow::SendMouseEvent: Unknown event type %d\n", eventType);
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
    }

    DEBUG ("compzillaWindow::SendMouseEvent: XSendEvent win=%p, child=%p, x=%d, y=%d, state=%p, "
           "button=%u, timestamp=%d\n", 
	   mWindow, destChild, x, y, state, button + 1, timestamp);

    XSendEvent (mDisplay, destChild, True, xevMask, &xev);
    //XevieSendEvent(mDisplay, &xev, XEVIE_MODIFIED);

    // Stop processing event
    mouseEv->StopPropagation ();
    mouseEv->PreventDefault ();
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
    nsCOMPtr<nsIDOMMouseEvent> mouseEv = do_QueryInterface (aDOMEvent);
    WARNING ("compzillaWindow::MouseClick: Ignored\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
compzillaWindow::MouseDblClick (nsIDOMEvent* aDOMEvent)
{
    nsCOMPtr<nsIDOMMouseEvent> mouseEv = do_QueryInterface (aDOMEvent);
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
    WARNING ("compzillaWindow::FocusIn: W00T!!!\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
compzillaWindow::FocusOut (nsIDOMEvent* aDOMEvent)
{
    WARNING ("compzillaWindow::FocusOut: W00T!!!\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}
