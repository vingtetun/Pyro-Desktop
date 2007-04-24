/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#define MOZILLA_INTERNAL_API

#include "compzillaWindow.h"
#include "compzillaIRenderingContext.h"
#include "compzillaRenderingContext.h"
#include "nsKeycodes.h"

#include <nsIDOMEventTarget.h>
#include <nsICanvasElement.h>
#include <nsISupportsUtils.h>
#if MOZILLA_1_8_BRANCH
#include <nsStringAPI.h>
#else
#include <nsXPCOMStrings.h>
#endif
#include "nsHashPropertyBag.h"

extern "C" {
#include <stdio.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xcomposite.h>
#if HAVE_XEVIE
#include <X11/extensions/Xevie.h>
#endif

#include "XAtoms.h"

#include <gdk/gdkkeys.h>
extern uint32 gtk_get_current_event_time (void);
#include <gdk/gdkevents.h>
extern GdkEvent *gtk_get_current_event (void);
}

#if WITH_SPEW
#define SPEW(format...) printf("   - " format)
#else
#define SPEW(format...)
#endif

#define INFO(format...) printf(" *** " format)
#define WARNING(format...) printf(" !!! " format)
#define ERROR(format...) fprintf(stderr, format)

NS_IMPL_ADDREF(compzillaWindow)
NS_IMPL_RELEASE(compzillaWindow)
NS_INTERFACE_MAP_BEGIN(compzillaWindow)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMEventTarget)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsIDOMEventListener, nsIDOMKeyListener)
    NS_INTERFACE_MAP_ENTRY(nsIDOMKeyListener)
    NS_INTERFACE_MAP_ENTRY(nsIDOMMouseListener)
    NS_INTERFACE_MAP_ENTRY(nsIDOMUIListener)
    NS_INTERFACE_MAP_ENTRY(compzillaIWindow)
NS_INTERFACE_MAP_END

extern XAtoms atoms;

compzillaWindow::compzillaWindow(Display *display, Window win, compzillaIWindowManager* wm)
    : mWM(wm),
      mDisplay(display),
      mWindow(win),
      mPixmap(0),
      mDamage(0),
      mLastEntered(0),
      mDestroyEvMgr("destroy"),
      mMoveResizeEvMgr("moveresize"),
      mShowEvMgr("show"),
      mHideEvMgr("hide"),
      mPropertyChangeEvMgr("propertyChange")
{
    NS_INIT_ISUPPORTS();

    // Redirect output entirely
    XCompositeRedirectWindow (display, win, CompositeRedirectManual);

    // Compiz selects only these.  Need more?
    XSelectInput (display, win, (PropertyChangeMask | EnterWindowMask | FocusChangeMask));
    XShapeSelectInput (display, win, ShapeNotifyMask);

    EnsurePixmap ();
}


compzillaWindow::~compzillaWindow()
{
    SPEW ("compzillaWindow::~compzillaWindow %p, xid=%p\n", this, mWindow);

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

    mContentNodes.Clear();
}


void
compzillaWindow::EnsurePixmap()
{
    if (mPixmap) {
        return;
    }

    XGrabServer (mDisplay);

    XGetWindowAttributes(mDisplay, mWindow, &mAttr);
    if (mAttr.map_state == IsViewable) {
        // Set up persistent offscreen window contents pixmap.
        mPixmap = XCompositeNameWindowPixmap (mDisplay, mWindow);

        /* 
         * Set up damage notification.  RawRectangles gives us smaller grain
         * changes, versus NonEmpty which seems to always include the entire
         * contents.
         */
        mDamage = XDamageCreate (mDisplay, mWindow, XDamageReportRawRectangles);
    }

    XUngrabServer (mDisplay);
}


void
compzillaWindow::ConnectListeners (bool connect, nsCOMPtr<nsISupports> aContent)
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

    nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface (aContent);
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


NS_IMETHODIMP
compzillaWindow::AddContentNode (nsISupports* aContent)
{
    mContentNodes.AppendObject (aContent);
    ConnectListeners (true, aContent);

    return NS_OK;
}

NS_IMETHODIMP
compzillaWindow::RemoveContentNode (nsISupports* aContent)
{
    // Allow a caller to remove O(N^2) behavior by removing end-to-start.
    for (PRUint32 i = mContentNodes.Count() - 1; i != PRUint32(-1); --i) {
        if (mContentNodes.ObjectAt(i) == aContent) {
            mContentNodes.RemoveObjectAt (i);
            ConnectListeners (false, aContent);
            return NS_OK;
        }
    }

    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
compzillaWindow::GetStringProperty (PRUint32 prop, nsAString& value)
{
    SPEW ("GetStringProperty (prop = %s)\n", XGetAtomName (mDisplay, prop));

    Atom actual_type;
    int format;
    unsigned long nitems;
    unsigned long bytes_after_return;
    unsigned char *data;

    XGetWindowProperty (mDisplay, mWindow, (Atom) prop,
                        0, BUFSIZ, false, XA_STRING, 
                        &actual_type, &format, &nitems, &bytes_after_return, 
                        &data);

    // XXX this is wrong - it's not always ASCII.  look at metacity's
    // handling of it (it calls a gdk function to convert the text
    // property to utf8).
    value = NS_ConvertASCIItoUTF16 ((const char*)data);

    SPEW (" + %s\n", data);

    return NS_OK;
}

NS_IMETHODIMP
compzillaWindow::GetAtomProperty (PRUint32 prop, PRUint32* value)
{
    SPEW ("GetAtomProperty (prop = %s)\n", XGetAtomName (mDisplay, prop));

    Atom actual_type;
    int format;
    unsigned long nitems;
    unsigned long bytes_after_return;
    unsigned char *data;

    XGetWindowProperty (mDisplay, mWindow, (Atom)prop,
                        0, BUFSIZ, false, XA_ATOM,
                        &actual_type, &format, &nitems, &bytes_after_return, 
                        &data);

    *value = *(PRUint32*)data;

    return NS_OK;
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

        SPEW ("compzillaWindow::HandleEvent: unhandled type=%s, target=%p!!!\n", 
              cdata, target.get ());
    }

    return NS_OK;
}


/*
 * Do the reverse of nsGtkEventHandler.cpp:nsPlatformToDOMKeyCode...
 */
unsigned int
compzillaWindow::DOMKeyCodeToKeySym (PRUint32 vkCode)
{
    int i, length = 0;

    // since X has different key symbols for upper and lowercase letters and
    // mozilla does not, just use uppercase for now.
    if (vkCode >= nsIDOMKeyEvent::DOM_VK_A && vkCode <= nsIDOMKeyEvent::DOM_VK_Z)
        return vkCode - nsIDOMKeyEvent::DOM_VK_A + GDK_A;

    // numbers
    if (vkCode >= nsIDOMKeyEvent::DOM_VK_0 && vkCode <= nsIDOMKeyEvent::DOM_VK_9)
        return vkCode - nsIDOMKeyEvent::DOM_VK_0 + GDK_0;

    // keypad numbers
    if (vkCode >= nsIDOMKeyEvent::DOM_VK_NUMPAD0 && vkCode <= nsIDOMKeyEvent::DOM_VK_NUMPAD9)
        return vkCode - nsIDOMKeyEvent::DOM_VK_NUMPAD0 + GDK_KP_0;

    // map Sun Keyboard special keysyms
    if (IS_XSUN_XSERVER(mDisplay)) {
        length = sizeof(nsSunKeycodes) / sizeof(struct nsKeyConverter);
        for (i = 0; i < length; i++) {
            if (nsSunKeycodes[i].vkCode == vkCode) {
                return nsSunKeycodes[i].keysym;
            }
        }
    }

    // misc other things
    length = sizeof(nsKeycodes) / sizeof(struct nsKeyConverter);
    for (i = 0; i < length; i++) {
        if (nsKeycodes[i].vkCode == vkCode) {
            return nsKeycodes[i].keysym;
        }
    }

    if (vkCode >= nsIDOMKeyEvent::DOM_VK_F1 && vkCode <= nsIDOMKeyEvent::DOM_VK_F24)
        return vkCode - nsIDOMKeyEvent::DOM_VK_F1 + GDK_F1;

    return 0;
}


void
compzillaWindow::SendKeyEvent (int eventType, nsIDOMKeyEvent *keyEv)
{
    DOMTimeStamp timestamp;
    PRBool ctrl, shift, alt, meta;
    int state = 0;

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

    PRUint32 keycode;
    keyEv->GetKeyCode (&keycode);

    unsigned int xkeysym = DOMKeyCodeToKeySym (keycode);

#ifdef USE_GDK_KEYMAP
    // FIXME: There's probably some annoying reason, like XKB, we need to use 
    //        the GDK version.  But for now I'm in denial.
    GdkKeymapKey *keys = NULL;
    int n_keys = 0;
    if (!gdk_keymap_get_entries_for_keyval (gdk_keymap_get_for_display (gdk_display_get_default ()),
                                            xkeysym,
                                            &keys,
                                            &n_keys)) {
        ERROR ("Unknown keyval '%d' ignored.", xkeysym);
        return;
    }

    unsigned int xkeycode = 0;
    for (int i = 0; i < n_keys; i++) {
        if (keys [i].keycode) {
            xkeycode = keys [i].keycode;
            break;
        }
    }
    g_free (keys);

    SPEW ("compzillaWindow::SendKeyEvent: gdk_keymap_get_entries_for_keyval keysym=%p, "
          "keycode=%p\n", xkeysym, xkeycode);
#else
    unsigned int xkeycode = XKeysymToKeycode (mDisplay, xkeysym);
#endif

    // Build up the XEvent we will send
    XEvent xev = { 0 };
    xev.xkey.type = eventType;
    xev.xkey.serial = 0;
    xev.xkey.display = mDisplay;
    xev.xkey.window = mWindow;
    xev.xkey.root = mAttr.root;
    xev.xkey.time = timestamp;
    xev.xkey.state = state;
    xev.xkey.keycode = xkeycode;
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

    SPEW ("compzillaWindow::SendKeyEvent: %s%s win=%p, child=%p, state=%p, keycode=%u, "
          "timestamp=%d\n", 
          eventType == _KeyPress ? "PRESS" : "", 
          eventType == KeyRelease ? "RELEASE" : "", 
          mWindow, mWindow, state, xkeycode, timestamp);

    XSendEvent (mDisplay, mWindow, True, xevMask, &xev);

    keyEv->StopPropagation ();
    keyEv->PreventDefault ();
}


#if USE_GDK_KEY_EVENTING
void
compzillaWindow::SendKeyEvent (int eventType, nsIDOMKeyEvent *keyEv)
{
    GdkEvent *gdkev = gtk_get_current_event ();

    // Build up the XEvent we will send
    XEvent xev = { 0 };
    xev.xkey.type = eventType;
    xev.xkey.serial = 0;
    xev.xkey.display = mDisplay;
    xev.xkey.window = mWindow;
    xev.xkey.root = mAttr.root;
    xev.xkey.time = gdkev->key.time;
    xev.xkey.state = gdkev->key.state;
    xev.xkey.keycode = gdkev->key.hardware_keycode;
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

    SPEW ("compzillaWindow::SendKeyEvent: %s%s win=%p, child=%p, state=%p, keycode=%u, "
          "timestamp=%d\n", 
          eventType == _KeyPress ? "PRESS" : "", 
          eventType == KeyRelease ? "RELEASE" : "", 
          mWindow, mWindow, gdkev->key.state, gdkev->key.hardware_keycode, gdkev->key.time);

    XSendEvent (mDisplay, mWindow, True, xevMask, &xev);

    keyEv->StopPropagation ();
    keyEv->PreventDefault ();
}
#endif


NS_IMETHODIMP
compzillaWindow::KeyDown (nsIDOMEvent* aDOMEvent)
{
    nsCOMPtr<nsIDOMKeyEvent> keyEv = do_QueryInterface (aDOMEvent);
    NS_ASSERTION (keyEv, "Invalid key event");

    SendKeyEvent (_KeyPress, keyEv);
    return NS_OK;
}


NS_IMETHODIMP
compzillaWindow::KeyUp (nsIDOMEvent* aDOMEvent)
{
    nsCOMPtr<nsIDOMKeyEvent> keyEv = do_QueryInterface (aDOMEvent);
    NS_ASSERTION (keyEv, "Invalid key event");

    SendKeyEvent (KeyRelease, keyEv);
    return NS_OK;
}


NS_IMETHODIMP
compzillaWindow::KeyPress(nsIDOMEvent* aDOMEvent)
{
    SPEW ("compzillaWindow::KeyPress: Ignored!!!\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}


void
compzillaWindow::TranslateClientXYToWindow (int *x, int *y, nsIDOMEventTarget *target)
{
    nsCOMPtr<nsICanvasElement> canvasElement = do_QueryInterface (target);
    if (!canvasElement) {
	return;
    }

    // FIXME: This is probably broken and not robust.  Need to adjust x,y for
    //        current cairo transform.

    nsIFrame *frame;
    canvasElement->GetPrimaryCanvasFrame (&frame);
    if (!frame) {
	return;
    }

    // roc says GetScreenRect is an X server roundtrip, though I can't see why.
    // "Instead, I guess you still need to do the adding up of offsets up the
    // frame tree, and appunits->pixels conversion"
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

    nsIDOMEventTarget *target;
    mouseEv->GetCurrentTarget (&target);
    TranslateClientXYToWindow (&x, &y, target);

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

#if DEBUG
    SPEW ("compzillaWindow::SendMouseEvent: win=%p, child=%p, x=%d, y=%d, state=%p, "
          "button=%u, timestamp=%d\n", 
          mWindow, destChild, x, y, state, button + 1, timestamp);
#endif

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
    NS_ASSERTION (mouseEv, "Invalid mouse event");

    SendMouseEvent (ButtonPress, mouseEv);
    return NS_OK;
}


NS_IMETHODIMP
compzillaWindow::MouseUp (nsIDOMEvent* aDOMEvent)
{
    nsCOMPtr<nsIDOMMouseEvent> mouseEv = do_QueryInterface (aDOMEvent);
    NS_ASSERTION (mouseEv, "Invalid mouse event");

    SendMouseEvent (ButtonRelease, mouseEv);
    return NS_OK;
}


NS_IMETHODIMP
compzillaWindow::MouseClick (nsIDOMEvent* aDOMEvent)
{
    SPEW ("compzillaWindow::MouseClick: Ignored\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
compzillaWindow::MouseDblClick (nsIDOMEvent* aDOMEvent)
{
    SPEW ("compzillaWindow::MouseDblClick: Ignored\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
compzillaWindow::MouseOver (nsIDOMEvent* aDOMEvent)
{
    nsCOMPtr<nsIDOMMouseEvent> mouseEv = do_QueryInterface (aDOMEvent);
    NS_ASSERTION (mouseEv, "Invalid mouse event");

    SendMouseEvent (EnterNotify, mouseEv);
    return NS_OK;
}


NS_IMETHODIMP
compzillaWindow::MouseOut (nsIDOMEvent* aDOMEvent)
{
    nsCOMPtr<nsIDOMMouseEvent> mouseEv = do_QueryInterface (aDOMEvent);
    NS_ASSERTION (mouseEv, "Invalid mouse event");

    SendMouseEvent (LeaveNotify, mouseEv);
    return NS_OK;
}


NS_IMETHODIMP
compzillaWindow::Activate (nsIDOMEvent* aDOMEvent)
{
    SPEW ("compzillaWindow::Activate: W00T!!!\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
compzillaWindow::FocusIn (nsIDOMEvent* aDOMEvent)
{
    SPEW ("compzillaWindow::FocusIn: win=%p\n", mWindow);

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
    SPEW ("compzillaWindow::FocusOut: win=%p\n", mWindow);

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
    NS_ASSERTION (mouseEv, "Invalid mouse event");

    SendMouseEvent (MotionNotify, mouseEv);
}


void
compzillaWindow::OnDOMMouseScroll (nsIDOMEvent *aDOMEvent)
{
    nsCOMPtr<nsIDOMMouseEvent> mouseEv = do_QueryInterface (aDOMEvent);
    NS_ASSERTION (mouseEv, "Invalid mouse event");

    SPEW ("compzillaWindow::OnDOMMouseScroll: mouseEv=%p\n", mouseEv.get());

    // NOTE: We don't receive events if them if a modifier key is down with
    //       Mozilla 1.8.

    SendMouseEvent (ButtonPress, mouseEv, true);

    // DOMMouseScroll has no Release equivalent, so fake it.
    SendMouseEvent (ButtonRelease, mouseEv, true);
}


/*
 * 
 * nsIDOMEventTarget Implementation...
 *
 */

NS_IMETHODIMP
compzillaWindow::AddEventListener (const nsAString & type, 
                                   nsIDOMEventListener *listener, 
                                   PRBool useCapture)
{
    if (type.EqualsLiteral ("destroy")) {
        return mDestroyEvMgr.AddEventListener (type, listener);
    } else if (type.EqualsLiteral ("moveresize")) {
        return mMoveResizeEvMgr.AddEventListener (type, listener);
    } else if (type.EqualsLiteral ("show")) {
        return mShowEvMgr.AddEventListener (type, listener);
    } else if (type.EqualsLiteral ("hide")) {
        return mHideEvMgr.AddEventListener (type, listener);
    } else if (type.EqualsLiteral ("propertychange")) {
        return mPropertyChangeEvMgr.AddEventListener (type, listener);
    } 
    return NS_ERROR_INVALID_ARG;
}


NS_IMETHODIMP
compzillaWindow::RemoveEventListener (const nsAString & type, 
                                      nsIDOMEventListener *listener, 
                                      PRBool useCapture)
{
    if (type.EqualsLiteral ("destroy")) {
        return mDestroyEvMgr.RemoveEventListener (type, listener);
    } else if (type.EqualsLiteral ("moveresize")) {
        return mMoveResizeEvMgr.RemoveEventListener (type, listener);
    } else if (type.EqualsLiteral ("show")) {
        return mShowEvMgr.RemoveEventListener (type, listener);
    } else if (type.EqualsLiteral ("hide")) {
        return mHideEvMgr.RemoveEventListener (type, listener);
    } else if (type.EqualsLiteral ("propertychange")) {
        return mPropertyChangeEvMgr.RemoveEventListener (type, listener);
    } 
    return NS_ERROR_INVALID_ARG;
}


NS_IMETHODIMP
compzillaWindow::DispatchEvent (nsIDOMEvent *evt, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

void
compzillaWindow::DestroyWindow ()
{
    for (PRUint32 i = mContentNodes.Count() - 1; i != PRUint32(-1); --i)
        mWM->WindowDestroyed (mContentNodes.ObjectAt(i));
}

void
compzillaWindow::MapWindow ()
{
    for (PRUint32 i = mContentNodes.Count() - 1; i != PRUint32(-1); --i)
        mWM->WindowMapped (mContentNodes.ObjectAt(i));
}

void
compzillaWindow::UnmapWindow ()
{
    for (PRUint32 i = mContentNodes.Count() - 1; i != PRUint32(-1); --i)
        mWM->WindowUnmapped (mContentNodes.ObjectAt(i));
}

void
compzillaWindow::PropertyChanged (Window win, Atom prop)
{

    nsIWritablePropertyBag *wbag;

    /* XXX check return value */
    NS_NewHashPropertyBag (&wbag);

    nsCOMPtr<nsIWritablePropertyBag2> wbag2 = do_QueryInterface(wbag);
    nsCOMPtr<nsIPropertyBag2> bag2 = do_QueryInterface(wbag);


    switch (prop) {
        // ICCCM properties

    case XA_WM_NAME:
    case XA_WM_ICON_NAME: {
        // XXX this is missing some massaging, since the WM_NAME
        // property isn't in utf8, but in some locale character set
        // (latin1?  who knows).  Check the gtk+ source on how to
        // handle this.
        nsString str;
        GetStringProperty (prop, str);
        wbag2->SetPropertyAsAString (NS_LITERAL_STRING (".text"), str);
        break;
    }

    case XA_WM_HINTS: {
        XWMHints *wmHints;

        wmHints = XGetWMHints (mDisplay, win);

        wbag2->SetPropertyAsInt32  (NS_LITERAL_STRING ("wmHints.flags"), wmHints->flags);
        wbag2->SetPropertyAsBool   (NS_LITERAL_STRING ("wmHints.input"), wmHints->input);
        wbag2->SetPropertyAsInt32  (NS_LITERAL_STRING ("wmHints.initialState"), wmHints->initial_state);
        wbag2->SetPropertyAsUint32 (NS_LITERAL_STRING ("wmHints.iconPixmap"), wmHints->icon_pixmap);
        wbag2->SetPropertyAsUint32 (NS_LITERAL_STRING ("wmHints.iconWindow"), wmHints->icon_window);
        wbag2->SetPropertyAsInt32  (NS_LITERAL_STRING ("wmHints.iconX"), wmHints->icon_x);
        wbag2->SetPropertyAsInt32  (NS_LITERAL_STRING ("wmHints.iconY"), wmHints->icon_y);
        wbag2->SetPropertyAsUint32 (NS_LITERAL_STRING ("wmHints.iconMask"), wmHints->icon_mask);
        wbag2->SetPropertyAsUint32 (NS_LITERAL_STRING ("wmHints.windowGroup"), wmHints->window_group);

        XFree (wmHints);
        break;
    }

    case XA_WM_NORMAL_HINTS: {
        XSizeHints sizeHints;
        long supplied;

        // XXX check return value
        XGetWMNormalHints (mDisplay, win, &sizeHints, &supplied);

        wbag2->SetPropertyAsInt32 (NS_LITERAL_STRING ("sizeHints.flags"), sizeHints.flags);

        wbag2->SetPropertyAsInt32 (NS_LITERAL_STRING ("sizeHints.x"), sizeHints.x);
        wbag2->SetPropertyAsInt32 (NS_LITERAL_STRING ("sizeHints.y"), sizeHints.y);
        wbag2->SetPropertyAsInt32 (NS_LITERAL_STRING ("sizeHints.width"), sizeHints.width);
        wbag2->SetPropertyAsInt32 (NS_LITERAL_STRING ("sizeHints.height"), sizeHints.height);

        wbag2->SetPropertyAsInt32 (NS_LITERAL_STRING ("sizeHints.minWidth"), sizeHints.min_width);
        wbag2->SetPropertyAsInt32 (NS_LITERAL_STRING ("sizeHints.minHeight"), sizeHints.min_height);

        wbag2->SetPropertyAsInt32 (NS_LITERAL_STRING ("sizeHints.maxWidth"), sizeHints.max_width);
        wbag2->SetPropertyAsInt32 (NS_LITERAL_STRING ("sizeHints.maxHeight"), sizeHints.max_height);

        wbag2->SetPropertyAsInt32 (NS_LITERAL_STRING ("sizeHints.widthInc"), sizeHints.width_inc);
        wbag2->SetPropertyAsInt32 (NS_LITERAL_STRING ("sizeHints.heightInc"), sizeHints.height_inc);

        wbag2->SetPropertyAsInt32 (NS_LITERAL_STRING ("sizeHints.minAspect.x"), sizeHints.min_aspect.x);
        wbag2->SetPropertyAsInt32 (NS_LITERAL_STRING ("sizeHints.minAspect.y"), sizeHints.min_aspect.y);

        wbag2->SetPropertyAsInt32 (NS_LITERAL_STRING ("sizeHints.maxAspect.x"), sizeHints.max_aspect.x);
        wbag2->SetPropertyAsInt32 (NS_LITERAL_STRING ("sizeHints.maxAspect.y"), sizeHints.max_aspect.y);

        if ((supplied & (PBaseSize|PWinGravity)) != 0) {
            wbag2->SetPropertyAsInt32 (NS_LITERAL_STRING ("sizeHints.baseWidth"), sizeHints.base_width);
            wbag2->SetPropertyAsInt32 (NS_LITERAL_STRING ("sizeHints.baseHeight"), sizeHints.base_height);

            wbag2->SetPropertyAsInt32 (NS_LITERAL_STRING ("sizeHints.winGravity"), sizeHints.win_gravity);
        }
    }
    case XA_WM_CLASS: {
        // 2 strings, separated by a \0
        break;
    }
    case XA_WM_TRANSIENT_FOR: {
        // the parent X window's canvas element
        break;
    }
    case XA_WM_CLIENT_MACHINE: {
        nsString str;
        GetStringProperty (prop, str);
        wbag2->SetPropertyAsAString (NS_LITERAL_STRING (".text"), str);
    }
    default:
        // ICCCM properties which don't have predefined atoms

        if (prop == atoms.x.WM_COLORMAP_WINDOWS) {
            // if we ever support this, shoot me..
        }
        else if (prop == atoms.x.WM_PROTOCOLS) {
            // an array of Atoms.
        }

        // non - ICCCM properties go here

        // EWMH properties

        else if (prop == atoms.x._NET_WM_NAME
                 || prop == atoms.x._NET_WM_VISIBLE_NAME
                 || prop == atoms.x._NET_WM_ICON_NAME
                 || prop == atoms.x._NET_WM_VISIBLE_ICON_NAME) {
            // utf8 encoded string
            nsString str;
            GetStringProperty (prop, str);
            wbag2->SetPropertyAsAString (NS_LITERAL_STRING (".text"), str);
        }
        else if (prop == atoms.x._NET_WM_DESKTOP) {
        }
        else if (prop == atoms.x._NET_WM_WINDOW_TYPE) {
            // XXX _NET_WM_WINDOW_TYPE is actually an array of atoms, not just 1.
            // this also needs fixing in the JS.
            PRUint32 atom;
            GetAtomProperty (prop, &atom);
            wbag2->SetPropertyAsUint32 (NS_LITERAL_STRING (".atom"), atom);
        }
        else if (prop == atoms.x._NET_WM_STATE) {
        }
        else if (prop == atoms.x._NET_WM_ALLOWED_ACTIONS) {
        }
        else if (prop == atoms.x._NET_WM_STRUT) {
        }
        else if (prop == atoms.x._NET_WM_STRUT_PARTIAL) {
        }
        else if (prop == atoms.x._NET_WM_ICON_GEOMETRY) {
        }
        else if (prop == atoms.x._NET_WM_ICON) {
        }
        else if (prop == atoms.x._NET_WM_PID) {
        }
        else if (prop == atoms.x._NET_WM_HANDLED_ICONS) {
        }
        else if (prop == atoms.x._NET_WM_USER_TIME) {
        }
        else if (prop == atoms.x._NET_FRAME_EXTENTS) {
        }

        break;
    }

    // XXX this might be wrong - it's a writable property bag, so
    // perhaps we need to re-initialize the property bag each time
    // through the loop?
    for (PRUint32 i = mContentNodes.Count() - 1; i != PRUint32(-1); --i)
        mWM->PropertyChanged (mContentNodes.ObjectAt(i), (PRUint32)prop, bag2);

    NS_RELEASE (wbag);
}

void
compzillaWindow::WindowDamaged (XRectangle *rect)
{
    for (PRUint32 i = mContentNodes.Count() - 1; i != PRUint32(-1); --i) {
        nsCOMPtr<nsIDOMHTMLCanvasElement> canvas = do_QueryInterface (mContentNodes.ObjectAt(i));

        nsCOMPtr<compzillaIRenderingContextInternal> internal;
        nsresult rv = canvas->GetContext (NS_LITERAL_STRING ("compzilla"), 
                                          getter_AddRefs (internal));

        if (NS_SUCCEEDED (rv)) {
            EnsurePixmap ();
            internal->SetDrawable (mDisplay, mPixmap, mAttr.visual);
            internal->Redraw (nsRect (rect->x, rect->y, rect->width, rect->height));
        }
    }
}

void
compzillaWindow::WindowConfigured (PRInt32 x, PRInt32 y,
                                   PRInt32 width, PRInt32 height,
                                   PRInt32 border,
                                   compzillaWindow *abovewin)
{
    for (PRUint32 i = mContentNodes.Count() - 1; i != PRUint32(-1); --i) {
        mWM->WindowConfigured (mContentNodes.ObjectAt(i),
                               x, y,
                               width, height,
                               border,
                               // this doesn't work given that abovewin has a list of content nodes..
                               // but really, we shouldn't have to worry about this, as you *can't* reliably
                               // specify a window to raise/lower above/below, since clients can't depend
                               // on the fact that other topevel windows are siblings of each other.
                               /*abovewin ? abovewin->mContent : */NULL);
    }
}
