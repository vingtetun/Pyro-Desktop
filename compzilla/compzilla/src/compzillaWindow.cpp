/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#if MOZILLA_1_8_BRANCH
#include <nsStringAPI.h>
#else
#include <nsXPCOMStrings.h>
#endif

#include "compzillaWindow.h"
#include "compzillaRenderingContext.h"
#include "Debug.h"
#include "nsKeycodes.h"
#include "XAtoms.h"

#include <nsICanvasElement.h>        // unstable
#include <nsIDOMClassInfo.h>         // unstable
#include <nsIDOMEventTarget.h>
#include <nsIDOMHTMLCanvasElement.h> // unstable
#include <nsComponentManagerUtils.h>
#include <nsHashPropertyBag.h>       // unstable

extern "C" {
#include <stdio.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xcomposite.h>

#include <gdk/gdk.h>
#include <gdk/gdkkeys.h>
#include <gdk/gdkproperty.h>
#include <gdk/gdkx.h>
#include <gdk/gdkevents.h>
// FIXME: This just avoids the need to include GTK+ headers
extern uint32 gtk_get_current_event_time (void);
extern GdkEvent *gtk_get_current_event (void);
}

extern XAtoms atoms;


NS_IMPL_ADDREF(compzillaWindow)
NS_IMPL_RELEASE(compzillaWindow)
NS_INTERFACE_MAP_BEGIN(compzillaWindow)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, compzillaIWindow)
    NS_INTERFACE_MAP_ENTRY(compzillaIWindow)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsIDOMEventListener, nsIDOMKeyListener)
    NS_INTERFACE_MAP_ENTRY(nsIDOMKeyListener)
    NS_INTERFACE_MAP_ENTRY(nsIDOMMouseListener)
    NS_INTERFACE_MAP_ENTRY(nsIDOMUIListener)
    NS_IMPL_QUERY_CLASSINFO(compzillaWindow)
NS_INTERFACE_MAP_END
NS_IMPL_CI_INTERFACE_GETTER2(compzillaWindow, 
                             compzillaIWindow,
                             nsIDOMEventListener)


nsresult
CZ_NewCompzillaWindow(Display *display, 
                      Window win, 
                      XWindowAttributes *attrs,
                      compzillaWindow** retval)
{
    *retval = nsnull;

    compzillaWindow *window = new compzillaWindow (display, win, attrs);
    if (!window)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(window);
    *retval = window;

    return NS_OK;
}


compzillaWindow::compzillaWindow(Display *display, Window win, XWindowAttributes *attrs)
    : mDisplay(display),
      mWindow(win),
      mAttr(*attrs),
      mPixmap(None),
      mDamage(None),
      mLastEntered(None),
      mIsDestroyed(false),
      mIsRedirected(false)
{
    XSelectInput (display, win, (PropertyChangeMask | EnterWindowMask | FocusChangeMask));

    XShapeSelectInput (display, win, ShapeNotifyMask);

    // Get notified of global cursor changes.  
    // FIXME: This is not very useful, as X has no way of fetching the Cursor
    // for a given window.
    //XFixesSelectCursorInput (display, win, XFixesDisplayCursorNotifyMask);

    XGrabButton (display, AnyButton, AnyModifier, win, true, 
                 (ButtonPressMask | ButtonReleaseMask | ButtonMotionMask),
                 GrabModeSync, GrabModeSync, None, None);

    /* 
     * Set up damage notification.  RawRectangles gives us smaller grain
     * changes, versus NonEmpty which seems to always include the entire
     * contents.
     */
    mDamage = XDamageCreate (mDisplay, mWindow, XDamageReportRawRectangles);

    if (mAttr.map_state == IsViewable) {
        mAttr.map_state = IsUnmapped;
        Mapped (mAttr.override_redirect);
    }
}


compzillaWindow::~compzillaWindow()
{
    NS_ASSERT_OWNINGTHREAD(compzillaWindow);

    SPEW ("~compzillaWindow %p, xid=%p\n", this, mWindow);

    Destroyed ();

    // FIXME: Do we need this?
    //UnredirectWindow ();

    SPEW ("~compzillaWindow DONE\n");
}


void
compzillaWindow::UpdateAttributes ()
{
    XGetWindowAttributes(mDisplay, mWindow, &mAttr);
}


void
compzillaWindow::BindWindow ()
{
    RedirectWindow ();

    if (!mPixmap) {
        XGrabServer (mDisplay);

        UpdateAttributes ();

        if (mAttr.map_state == IsViewable) {
            // Set up persistent offscreen window contents pixmap.
            mPixmap = XCompositeNameWindowPixmap (mDisplay, mWindow);

            if (mPixmap == None) {
                ERROR ("XCompositeNameWindowPixmap failed for window %p\n", mWindow);
            }
        }

        XUngrabServer (mDisplay);
    }
}


void 
compzillaWindow::ReleaseWindow ()
{
    if (mPixmap) {
        XFreePixmap (mDisplay, mPixmap);
        mPixmap = None;
    }
}


void 
compzillaWindow::RedirectWindow ()
{
    if (mIsRedirected)
        return;

    XCompositeRedirectWindow (mDisplay, mWindow, CompositeRedirectManual);
    mIsRedirected = true;
}

   
void 
compzillaWindow::UnredirectWindow ()
{
    if (!mIsRedirected)
        return;

    ReleaseWindow ();

    XCompositeUnredirectWindow (mDisplay, mWindow, CompositeRedirectManual);
    mIsRedirected = false;
}


void
compzillaWindow::ConnectListeners (bool connect, nsCOMPtr<nsISupports> aContent)
{
    static const nsString events[] = {
	// nsIDOMKeyListener events
	NS_LITERAL_STRING("keydown"),
	NS_LITERAL_STRING("keyup"),
	// "keypress",

	// nsIDOMMouseListener events
	NS_LITERAL_STRING("mousedown"),
	NS_LITERAL_STRING("mouseup"),
	NS_LITERAL_STRING("mouseover"),
	NS_LITERAL_STRING("mouseout"),
	NS_LITERAL_STRING("mousein"),
	// "click",
	// "dblclick",

	// nsIDOMUIListener events
	// "activate",
	NS_LITERAL_STRING("focusin"),
	NS_LITERAL_STRING("focusout"),
        NS_LITERAL_STRING("DOMFocusIn"),
        NS_LITERAL_STRING("DOMFocusOut"),
        NS_LITERAL_STRING("resize"),

	// HandleEvent events
	NS_LITERAL_STRING("focus"),
	NS_LITERAL_STRING("blur"),
	NS_LITERAL_STRING("mousemove"),
	NS_LITERAL_STRING("DOMMouseScroll"),

        nsString() // Must be last element
    };

    nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface (aContent);
    nsCOMPtr<nsIDOMEventListener> listener = 
	do_QueryInterface (NS_ISUPPORTS_CAST (nsIDOMKeyListener *, this));

    if (!target || !listener) {
        SPEW ("ConnectListeners: Trying to %s invalid listener!\n",
              connect ? "connect" : "disconnect");
	return;
    }

    for (int i = 0; !events [i].IsEmpty (); i++) {
	if (connect) {
	    target->AddEventListener (events [i], listener, PR_FALSE);
	} else {
	    target->RemoveEventListener (events [i], listener, PR_FALSE);
	}
    }
}


NS_IMETHODIMP 
compzillaWindow::GetNativeWindowId (PRInt32 *aId)
{
    *aId = mWindow;
    return NS_OK;
}


NS_IMETHODIMP
compzillaWindow::AddContentNode (nsIDOMHTMLCanvasElement* aContent)
{
    SPEW ("AddContentNode this=%p, canvas=%p\n", this, aContent);

    if (mIsDestroyed)
        return NS_ERROR_FAILURE;

    nsCOMPtr<compzillaIRenderingContextInternal> internal;
    nsresult rv = aContent->GetContext (NS_LITERAL_STRING ("compzilla"), 
                                        getter_AddRefs (internal));
    if (NS_FAILED (rv))
        return rv;
    if (!internal)
        return NS_ERROR_FAILURE;

    mContentNodes.AppendObject (aContent);
    ConnectListeners (true, aContent);

    aContent->SetWidth (mAttr.width);
    aContent->SetHeight (mAttr.height);

    /* when initially adding a content node, we need to force a redraw
       to that node if we have an existing pixmap. */
    if (mPixmap) {
        XRectangle r;
        r.x = r.y = 0;
        r.width = mAttr.width;
        r.height = mAttr.height;

        RedrawContentNode (aContent, &r);
    }

    return NS_OK;
}


NS_IMETHODIMP
compzillaWindow::RemoveContentNode (nsIDOMHTMLCanvasElement* aContent)
{
    SPEW ("RemoveContentNode this=%p, canvas=%p\n", this, aContent);

    if (mIsDestroyed)
        return NS_OK;

    // Allow a caller to remove O(N^2) behavior by removing end-to-start.
    for (PRUint32 i = mContentNodes.Count() - 1; i != PRUint32(-1); --i) {
        if (mContentNodes.ObjectAt(i) == aContent) {
            mContentNodes.RemoveObjectAt (i);
            ConnectListeners (false, aContent);
            break;
        }
    }

    return NS_OK;
}


NS_IMETHODIMP
compzillaWindow::AddObserver (compzillaIWindowObserver *aObserver)
{
    SPEW ("AddObserver this=%p, observer=%p\n", this, aObserver);

    if (mIsDestroyed)
        return NS_ERROR_FAILURE;

    mObservers.AppendObject (aObserver);

    /* 
     * When initially adding an observer, we need to force a configure event to
     * update map/override redirect state. 
     */
    aObserver->Configure (mAttr.map_state == IsViewable,
                          mAttr.override_redirect,
                          mAttr.x, mAttr.y,
                          mAttr.width, mAttr.height,
                          mAttr.border_width,
                          NULL);

    return NS_OK;
}


NS_IMETHODIMP
compzillaWindow::RemoveObserver (compzillaIWindowObserver *aObserver)
{
    SPEW ("RemoveObserver window=%p, observer=%p\n", this, aObserver);

    if (mIsDestroyed)
        return NS_OK;

    // Allow a caller to remove O(N^2) behavior by removing end-to-start.
    for (PRUint32 i = mObservers.Count() - 1; i != PRUint32(-1); --i) {
        if (mObservers.ObjectAt(i) == aObserver) {
            mObservers.RemoveObjectAt (i);
            break;
        }
    }

    return NS_OK;
}


nsresult
compzillaWindow::GetUTF8StringProperty (Atom prop, nsACString& utf8Value)
{
    SPEW ("GetUTF8StringProperty this=%p, prop=%s\n", this, XGetAtomName (mDisplay, prop));

    Atom actual_type;
    int format;
    unsigned long nitems;
    unsigned long bytes_after_return;
    unsigned char *data;

    if (XGetWindowProperty (mDisplay, 
                            mWindow, 
                            prop,
                            0,
                            BUFSIZ, 
                            false, 
                            AnyPropertyType,
                            &actual_type, 
                            &format, 
                            &nitems, 
                            &bytes_after_return, 
                            &data) != Success || 
        format == None) {
        SPEW (" + (Not Found)\n");
        return NS_ERROR_FAILURE;
    }

    if (actual_type == atoms.x.UTF8_STRING) {
        utf8Value = (char*)data;
    }
    else if (actual_type == XA_STRING) {
        char **list = NULL;
        int count;

        count = gdk_text_property_to_utf8_list (gdk_x11_xatom_to_atom (actual_type),
                                                format, data, nitems,
                                                &list);

        if (count == 0) {
            XFree (data);
            return NS_ERROR_FAILURE;
        }

        utf8Value = list[0];

        g_strfreev (list);
    }
    else {
        WARNING ("invalid type for string property '%s': '%s'\n",
                 XGetAtomName (mDisplay, prop),
                 XGetAtomName (mDisplay, actual_type));
        XFree (data);
        return NS_ERROR_FAILURE;
    }

    XFree (data);

    return NS_OK;
}


nsresult
compzillaWindow::GetAtomProperty (Atom prop, PRUint32* value)
{
    SPEW ("GetAtomProperty this=%p, prop=%s\n", this, XGetAtomName (mDisplay, prop));

    Atom actual_type;
    int format;
    unsigned long nitems;
    unsigned long bytes_after_return;
    unsigned char *data;

    if (XGetWindowProperty (mDisplay, 
                            mWindow, 
                            prop,
                            0, 
                            BUFSIZ, 
                            false, 
                            XA_ATOM,
                            &actual_type, 
                            &format, 
                            &nitems, 
                            &bytes_after_return, 
                            &data) != Success || 
        format == None) {
        SPEW (" + (Not Found)\n");
        return NS_ERROR_FAILURE;
    }

    *value = *(Atom*)data;

    SPEW (" + %d (%s)\n", *value, XGetAtomName (mDisplay, *value));

    XFree (data);

    return NS_OK;
}


// All of the event listeners below return NS_OK to indicate that the
// event should not be consumed in the default case.

NS_IMETHODIMP
compzillaWindow::HandleEvent (nsIDOMEvent* aDOMEvent)
{
    nsAutoString type;
    aDOMEvent->GetType (type);

    if (type.EqualsLiteral ("mousemove")) {
        OnMouseMove (aDOMEvent);
    } else if (type.EqualsLiteral ("DOMMouseScroll")) {
        OnDOMMouseScroll (aDOMEvent);
    } else if (type.EqualsLiteral ("focus")) {
        FocusIn (aDOMEvent);
    } else if (type.EqualsLiteral ("blur")) {
        FocusOut (aDOMEvent);
    } else {
        nsCOMPtr<nsIDOMEventTarget> target;
        aDOMEvent->GetTarget (getter_AddRefs (target));

        SPEW_EVENT ("HandleEvent: Unhandled type=%s, target=%p!!!\n", 
                    PromiseFlatString (type).get(), target.get ());
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
        ERROR ("Unknown keyval '%d' ignored.\n", xkeysym);
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

    SPEW_EVENT ("SendKeyEvent: gdk_keymap_get_entries_for_keyval keysym=%p, "
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

    SPEW_EVENT ("SendKeyEvent: %s%s win=%p, child=%p, state=%p, keycode=%u, "
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

    SPEW_EVENT ("SendKeyEvent: %s%s win=%p, child=%p, state=%p, keycode=%u, "
                "timestamp=%d\n", 
                eventType == _KeyPress ? "PRESS" : "", 
                eventType == KeyRelease ? "RELEASE" : "", 
                mWindow, mWindow, gdkev->key.state, gdkev->key.hardware_keycode, 
                gdkev->key.time);

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
    SPEW_EVENT ("DOM KeyPress: Ignored. Expecting KeyDown/KeyUp instead.\n");
    return NS_OK;
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

    do {
        if (!XTranslateCoordinates (mDisplay, 
                                    last_child, 
                                    child, 
                                    *x, *y, x, y, 
                                    &new_child))
            break;

        last_child = child;
        child = new_child;
    } while (child && child != last_child);

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
    mouseEv->GetTarget (&target);
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

    SPEW_EVENT ("SendMouseEvent: win=%p, child=%p, x=%d, y=%d, state=%p, "
                "button=%u, timestamp=%d\n", 
                mWindow, destChild, x, y, state, button + 1, timestamp);

    if (eventType == ButtonPress) {
        gdk_pointer_ungrab (GDK_CURRENT_TIME);
        gdk_keyboard_ungrab (GDK_CURRENT_TIME);

        // Start a passive grab.  The window's app can override this.
        XGrabButton (mDisplay, AnyButton, AnyModifier, 
                     mWindow, True, 
                     (ButtonPressMask |
                      ButtonReleaseMask |
                      EnterWindowMask | 
                      LeaveWindowMask |
                      PointerMotionMask),
                     GrabModeAsync, 
                     GrabModeAsync, 
                     None,
                     None);

        XGrabKey (mDisplay, AnyKey, AnyModifier, 
                  mWindow, True, 
                  GrabModeAsync, 
                  GrabModeAsync);
    }

    XSendEvent (mDisplay, destChild, True, xevMask, &xev);

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
    SPEW_EVENT ("DOM MouseClick: Ignored. Expecting MouseDown/MouseUp instead.\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
compzillaWindow::MouseDblClick (nsIDOMEvent* aDOMEvent)
{
    SPEW_EVENT ("DOM MouseDblClick: Ignored. Expecting MouseDown/MouseUp instead.\n");
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
    SPEW_EVENT ("DOM Activate: Ignored.\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
compzillaWindow::FocusIn (nsIDOMEvent* aDOMEvent)
{
    SPEW_EVENT ("DOM FocusIn: win=%p\n", mWindow);

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
    SPEW_EVENT ("DOM FocusOut: win=%p\n", mWindow);

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

    SPEW_EVENT ("OnDOMMouseScroll: mouseEv=%p\n", mouseEv.get());

    // NOTE: We don't receive events if them if a modifier key is down with
    //       Mozilla 1.8.

    SendMouseEvent (ButtonPress, mouseEv, true);

    // DOMMouseScroll has no Release equivalent, so fake it.
    SendMouseEvent (ButtonRelease, mouseEv, true);
}


void
compzillaWindow::Destroyed ()
{
    SPEW ("DestroyWindow this=%p, window=%p, observers=%d, canvases=%d\n", 
          this, mWindow, mObservers.Count(), mContentNodes.Count());

    if (mIsDestroyed)
        return;

    mIsDestroyed = true;

    // Allow a caller to remove O(N^2) behavior by removing end-to-start.
    for (PRUint32 i = mContentNodes.Count() - 1; i != PRUint32(-1); --i) {
        ConnectListeners (false, mContentNodes.ObjectAt(i));
    }
    mContentNodes.Clear ();

    // Copy the observers so list iteration is reentrant.
    nsCOMArray<compzillaIWindowObserver> observers(mObservers);
    mObservers.Clear ();

    for (PRUint32 i = observers.Count() - 1; i != PRUint32(-1); --i) {
        observers.ObjectAt(i)->Destroy ();
    }
}


void
compzillaWindow::Mapped (bool override_redirect)
{
    if (mAttr.map_state == IsViewable ||
        mAttr.c_class == InputOnly)
        return;

    mAttr.map_state = IsViewable;
    mAttr.override_redirect = override_redirect;

    BindWindow ();

    for (PRUint32 i = mObservers.Count() - 1; i != PRUint32(-1); --i) {
        nsCOMPtr<compzillaIWindowObserver> observer = mObservers.ObjectAt(i);
        observer->Map (override_redirect);
    }
}


void
compzillaWindow::Unmapped ()
{
    if (mAttr.map_state != IsViewable)
        return;

    mAttr.map_state = IsUnmapped;
    
    ReleaseWindow ();

    for (PRUint32 i = mObservers.Count() - 1; i != PRUint32(-1); --i) {
        nsCOMPtr<compzillaIWindowObserver> observer = mObservers.ObjectAt(i);
        observer->Unmap ();
    }
}


nsresult
compzillaWindow::GetCardinalListProperty (Atom prop, 
                                          PRUint32 **values, 
                                          PRUint32 expected_nitems)
{
    SPEW ("GetCardinalListProperty this=%p, prop=%s\n", this, XGetAtomName (mDisplay, prop));

    Atom actual_type;
    int format;
    unsigned long bytes_after_return;
    unsigned char *data;
    unsigned long nitems;

    if (XGetWindowProperty (mDisplay, 
                            mWindow, 
                            prop,
                            0, 
                            expected_nitems, 
                            false, 
                            XA_CARDINAL,
                            &actual_type, 
                            &format, 
                            &nitems, 
                            &bytes_after_return, 
                            &data) != Success || 
        format == None) {
        SPEW (" + (Not Found)\n");
        return NS_ERROR_FAILURE;
    }

    if (nitems != expected_nitems) {
        ERROR ("XGetWindowProperty (%s) expected %d items, received %d\n",
               XGetAtomName (mDisplay, prop),
               expected_nitems,
               nitems);

        XFree (data);
        return NS_ERROR_FAILURE;
    }
            
    *values = (PRUint32*)data;
    return NS_OK;
}


NS_IMETHODIMP
compzillaWindow::GetProperty (PRUint32 iprop, nsIPropertyBag2 **bag2)
{
    *bag2 = nsnull;

    Atom prop = (Atom)iprop;

    nsISupports *bag;
    nsCOMPtr<nsIWritablePropertyBag2> wbag;
    nsCOMPtr<nsIPropertyBag2> rbag;

    nsresult rv = CallCreateInstance("@mozilla.org/hash-property-bag;1", &bag);
    if (NS_FAILED(rv)) {
        return rv;
    }
 
    wbag = do_QueryInterface (bag);
    rbag = do_QueryInterface (bag);

#define SET_BAG() do { \
    NS_ADDREF (rbag); \
    *bag2 = rbag; \
} while (0)

#define SET_PROP(_bag, _type, _key, _val) \
    (_bag)->SetPropertyAs##_type (NS_LITERAL_STRING (_key), _val)

    switch (prop) {
        // ICCCM properties

    case XA_WM_NAME:
    case XA_WM_ICON_NAME: {
        // XXX this is missing some massaging, since the WM_NAME
        // property isn't in utf8, but in some locale character set
        // (latin1?  who knows).  Check the gtk+ source on how to
        // handle this.
        nsCAutoString str;
        if (NS_OK == GetUTF8StringProperty (prop, str)) {
            SET_BAG ();
            SET_PROP(wbag, AUTF8String, "text", str);
        }
        break;
    }

    case XA_WM_HINTS: {
        XWMHints *wmHints;

        wmHints = XGetWMHints (mDisplay, mWindow);

        if (wmHints) {
            SET_BAG ();
            SET_PROP(wbag, Int32, "wmHints.flags", wmHints->flags);
            SET_PROP(wbag, Bool, "wmHints.input", wmHints->input);
            SET_PROP(wbag, Int32, "wmHints.initialState", wmHints->initial_state);
            SET_PROP(wbag, Uint32, "wmHints.iconPixmap", wmHints->icon_pixmap);
            SET_PROP(wbag, Uint32, "wmHints.iconWindow", wmHints->icon_window);
            SET_PROP(wbag, Int32, "wmHints.iconX", wmHints->icon_x);
            SET_PROP(wbag, Int32, "wmHints.iconY", wmHints->icon_y);
            SET_PROP(wbag, Uint32, "wmHints.iconMask", wmHints->icon_mask);
            SET_PROP(wbag, Uint32, "wmHints.windowGroup", wmHints->window_group);

            XFree (wmHints);
        }
        break;
    }

    case XA_WM_NORMAL_HINTS: {
        XSizeHints sizeHints;
        long supplied;

        // XXX check return value
        XGetWMNormalHints (mDisplay, mWindow, &sizeHints, &supplied);

        SET_BAG ();

        SET_PROP(wbag, Int32, "sizeHints.flags", sizeHints.flags);

        SET_PROP(wbag, Int32, "sizeHints.x", sizeHints.x);
        SET_PROP(wbag, Int32, "sizeHints.y", sizeHints.y);
        SET_PROP(wbag, Int32, "sizeHints.width", sizeHints.width);
        SET_PROP(wbag, Int32, "sizeHints.height", sizeHints.height);

        SET_PROP(wbag, Int32, "sizeHints.minWidth", sizeHints.min_width);
        SET_PROP(wbag, Int32, "sizeHints.minHeight", sizeHints.min_height);

        SET_PROP(wbag, Int32, "sizeHints.maxWidth", sizeHints.max_width);
        SET_PROP(wbag, Int32, "sizeHints.maxHeight", sizeHints.max_height);

        SET_PROP(wbag, Int32, "sizeHints.widthInc", sizeHints.width_inc);
        SET_PROP(wbag, Int32, "sizeHints.heightInc", sizeHints.height_inc);

        SET_PROP(wbag, Int32, "sizeHints.minAspect.x", sizeHints.min_aspect.x);
        SET_PROP(wbag, Int32, "sizeHints.minAspect.y", sizeHints.min_aspect.y);

        SET_PROP(wbag, Int32, "sizeHints.maxAspect.x", sizeHints.max_aspect.x);
        SET_PROP(wbag, Int32, "sizeHints.maxAspect.y", sizeHints.max_aspect.y);

        if ((supplied & (PBaseSize|PWinGravity)) != 0) {
            SET_PROP(wbag, Int32, "sizeHints.baseWidth", sizeHints.base_width);
            SET_PROP(wbag, Int32, "sizeHints.baseHeight", sizeHints.base_height);

            SET_PROP(wbag, Int32, "sizeHints.winGravity", sizeHints.win_gravity);
        }
        break;
    }
    case XA_WM_CLASS: {
        // 2 strings, separated by a \0

        char *instance, *_class;

        Atom actual_type;
        int format;
        unsigned long nitems;
        unsigned long bytes_after_return;
        unsigned char *data;

        XGetWindowProperty (mDisplay, mWindow, (Atom)prop,
                            0, BUFSIZ, false, XA_STRING,
                            &actual_type, &format, &nitems, &bytes_after_return, 
                            &data);

        instance = (char*)data;
        _class = instance + strlen (instance) + 1;

        SET_BAG ();
        SET_PROP (wbag, ACString, "instanceName", nsCAutoString (instance));
        SET_PROP (wbag, ACString, "className", nsCAutoString (_class));

        XFree (data);

        break;
    }
    case XA_WM_TRANSIENT_FOR: {
        // the parent X window's canvas element
        break;
    }
    case XA_WM_CLIENT_MACHINE: {
        nsCAutoString str;
        if (NS_OK == GetUTF8StringProperty (prop, str)) {
            SET_BAG ();
            SET_PROP(wbag, AUTF8String, "text", str);
        }
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
            nsCAutoString str;
            if (NS_OK == GetUTF8StringProperty (prop, str)) {
                SET_BAG ();
                SET_PROP(wbag, AUTF8String, "text", str);
            }
        }
        else if (prop == atoms.x._NET_WM_DESKTOP) {
        }
        else if (prop == atoms.x._NET_WM_WINDOW_TYPE) {
            // XXX _NET_WM_WINDOW_TYPE is actually an array of atoms, not just 1.
            // this also needs fixing in the JS.
            PRUint32 atom;
            if (NS_OK == GetAtomProperty (prop, &atom)) {
                SET_BAG ();
                SET_PROP(wbag, Uint32, "atom", atom);
            }
        }
        else if (prop == atoms.x._NET_WM_STATE) {
        }
        else if (prop == atoms.x._NET_WM_ALLOWED_ACTIONS) {
        }
        else if (prop == atoms.x._NET_WM_STRUT) {
            PRUint32 *cards;
            PRUint32 nitems;

            if (NS_OK == GetCardinalListProperty (prop, &cards, 4)) {
                SET_BAG ();
                SET_PROP (wbag, Uint32, "left", cards[0]);
                SET_PROP (wbag, Uint32, "right", cards[1]);
                SET_PROP (wbag, Uint32, "top", cards[2]);
                SET_PROP (wbag, Uint32, "bottom", cards[3]);
            
                XFree (cards);
            }
        }
        else if (prop == atoms.x._NET_WM_STRUT_PARTIAL) {
            PRUint32 *cards;
            PRUint32 nitems;

            if (NS_OK == GetCardinalListProperty (prop, &cards, 12)) {
                SET_BAG ();
                SET_PROP (wbag, Bool, "partial", true);

                SET_PROP (wbag, Uint32, "left", cards[0]);
                SET_PROP (wbag, Uint32, "right", cards[1]);
                SET_PROP (wbag, Uint32, "top", cards[2]);
                SET_PROP (wbag, Uint32, "bottom", cards[3]);

                SET_PROP (wbag, Uint32, "leftStartY", cards[4]);
                SET_PROP (wbag, Uint32, "leftEndY", cards[5]);
                SET_PROP (wbag, Uint32, "rightStartY", cards[6]);
                SET_PROP (wbag, Uint32, "rightEndY", cards[7]);

                SET_PROP (wbag, Uint32, "topStartX", cards[8]);
                SET_PROP (wbag, Uint32, "topEndX", cards[9]);
                SET_PROP (wbag, Uint32, "bottomStartX", cards[10]);
                SET_PROP (wbag, Uint32, "bottomEndX", cards[11]);
                
                XFree (cards);
            }
        }
        else if (prop == atoms.x._NET_WM_ICON_GEOMETRY) {
            PRUint32 *cards;
            PRUint32 nitems;

            if (NS_OK == GetCardinalListProperty (prop, &cards, 4)) {
                SET_BAG ();
                SET_PROP (wbag, Bool, "partial", false);
                SET_PROP(wbag, Uint32, "x", cards[0]);
                SET_PROP(wbag, Uint32, "y", cards[1]);
                SET_PROP(wbag, Uint32, "width", cards[2]);
                SET_PROP(wbag, Uint32, "height", cards[3]);
                XFree (cards);
            }
        }
        else if (prop == atoms.x._NET_WM_ICON) {
            Atom actual_type;
            int format;
            unsigned long nitems;
            unsigned long bytes_after_return;
            unsigned char *data;
            
            if (XGetWindowProperty (mDisplay, mWindow, prop,
                                    0, BUFSIZ, false, XA_CARDINAL,
                                    &actual_type, &format, &nitems, &bytes_after_return, 
                                    &data) == Success) {
                nsCAutoString dataStr;
                dataStr.Assign ((char *) data, (format / 8) * nitems);

                SET_BAG ();
                SET_PROP(wbag, ACString, "data", dataStr);
                XFree (data);
            }
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

#undef SET_PROP
#undef SET_BAG

    return NS_OK;
}


void
compzillaWindow::PropertyChanged (Atom prop, bool deleted)
{
    for (PRUint32 i = mObservers.Count() - 1; i != PRUint32(-1); --i) {
        nsCOMPtr<compzillaIWindowObserver> observer = mObservers.ObjectAt(i);
        observer->PropertyChange (prop, deleted);
    }
}


void
compzillaWindow::ClientMessaged (Atom type, int format, long *data/*[5]*/)
{
    for (PRUint32 i = mObservers.Count() - 1; i != PRUint32(-1); --i) {
        nsCOMPtr<compzillaIWindowObserver> observer = mObservers.ObjectAt(i);
        observer->ClientMessageRecv (type, format,
                                     data[0],
                                     data[1],
                                     data[2],
                                     data[3],
                                     data[4]);
    }
}


void
compzillaWindow::RedrawContentNode (nsIDOMHTMLCanvasElement *aContent, XRectangle *rect)
{
    nsCOMPtr<compzillaIRenderingContextInternal> internal;
    nsresult rv = aContent->GetContext (NS_LITERAL_STRING ("compzilla"), 
                                        getter_AddRefs (internal));

    if (NS_SUCCEEDED (rv)) {
        internal->SetDrawable (mDisplay, mPixmap, mAttr.visual);
        internal->Redraw (nsRect (rect->x, rect->y, rect->width, rect->height));
    }
}


void
compzillaWindow::Damaged (XRectangle *rect)
{
    BindWindow ();

    if (!rect) {
        XRectangle allrect = { mAttr.x, mAttr.y, mAttr.width, mAttr.height };
        Damaged (&allrect);
        return;
    }

    for (PRUint32 i = mContentNodes.Count() - 1; i != PRUint32(-1); --i) {
        RedrawContentNode (mContentNodes.ObjectAt(i), rect);
    }
}


bool
compzillaWindow::Resized (PRInt32 x, 
                          PRInt32 y,
                          PRInt32 width, 
                          PRInt32 height,
                          PRInt32 border)
{
    if (width != mAttr.width || 
        height != mAttr.height || 
        border != mAttr.border_width || 
        mAttr.override_redirect) {

        if (mIsRedirected) {
            ReleaseWindow ();

            mPixmap = XCompositeNameWindowPixmap (mDisplay, mWindow);
            if (mPixmap == None)
                return false;

            for (PRUint32 i = mContentNodes.Count() - 1; i != PRUint32(-1); --i) {
                nsIDOMHTMLCanvasElement *aContent = mContentNodes.ObjectAt (i);
                aContent->SetWidth (width);
                aContent->SetHeight (height);
            }

            Damaged (NULL);
        }
    }

    mAttr.x = x;
    mAttr.y = y;
    mAttr.width = width;
    mAttr.height = height;
    mAttr.border_width = border;

    return true;
}


void
compzillaWindow::Configured (bool isNotify,
                             PRInt32 x, PRInt32 y,
                             PRInt32 width, PRInt32 height,
                             PRInt32 border,
                             compzillaWindow *aboveWin,
                             bool override_redirect)
{
    mAttr.override_redirect = override_redirect;

    if (isNotify) {
        Resized (x, y, width, height, border);
    }

    if (!isNotify || override_redirect) {
        // abovewin doesn't work given that abovewin has a list of content
        // nodes...  but really, we shouldn't have to worry about this, as you
        // *can't* reliably specify a window to raise/lower above/below, since
        // clients can't depend on the fact that other topevel windows are
        // siblings of each other.
        nsCOMPtr<compzillaIWindow> above = aboveWin;

        for (PRUint32 i = mObservers.Count() - 1; i != PRUint32(-1); --i) {
            nsCOMPtr<compzillaIWindowObserver> observer = mObservers.ObjectAt(i);

            // FIXME: Respect the return value
            observer->Configure (mAttr.map_state == IsViewable,
                                 override_redirect,
                                 x, y,
                                 width, height,
                                 border,
                                 above);
        }
    }
}
