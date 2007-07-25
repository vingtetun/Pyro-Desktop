/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#include <nspr.h>
#include <jsapi.h>
#include <nsMemory.h>
#include <nsHashPropertyBag.h>
#include <nsIBaseWindow.h>
#include <nsIDocShell.h>
#include <nsIScriptGlobalObject.h>
#ifndef MOZILLA_1_8_BRANCH
#include <nsPIDOMWindow.h>
#endif
#include <nsIWidget.h>

#include "compzillaControl.h"
#include "XAtoms.h"
#include "Debug.h"

extern "C" {
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkdisplay.h>
#include <gdk/gdkscreen.h>

#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/cursorfont.h>
}


// Global storage
PRLogModuleInfo *compzillaLog; // From Debug.h
XAtoms atoms;                  // From XAtoms.h


NS_IMPL_ISUPPORTS1_CI(compzillaControl, compzillaIControl);


int compzillaControl::sErrorCnt;
int compzillaControl::composite_event;
int compzillaControl::composite_error;
int compzillaControl::damage_event;
int compzillaControl::damage_error;
int compzillaControl::xfixes_event;
int compzillaControl::xfixes_error;
int compzillaControl::shape_event;
int compzillaControl::shape_error;


compzillaControl::compzillaControl()
{
    if (!compzillaLog) {
        compzillaLog = PR_NewLogModule ("compzilla");
    }

    mXDisplay = GDK_DISPLAY_XDISPLAY (gdk_display_get_default ());
    mXRoot = GDK_DRAWABLE_XID (gdk_get_default_root_window ());

    mWindowMap.Init(50);
}


compzillaControl::~compzillaControl()
{
    if (mMainwin) {
        gdk_window_unref (mMainwin);
    }
}


/*
 *
 * compzillaIControl Implementation...
 *
 */

NS_IMETHODIMP
compzillaControl::HasWindowManager(nsIDOMWindow *window,
                                   PRBool *retval)
{
    char *atom_name;
    Atom atom;

    // FIXME: Handle screens
    atom_name = g_strdup_printf ("WM_S%d", 0);
    atom = XInternAtom (mXDisplay, atom_name, FALSE);
    g_free (atom_name);

    *retval = XGetSelectionOwner (mXDisplay, atom) != None;

    SPEW ("HasWindowManager == %s\n", *retval ? "TRUE" : "FALSE");

    return NS_OK;
}


NS_IMETHODIMP
compzillaControl::RegisterWindowManager(nsIDOMWindow *window)
{
    nsresult rv = NS_OK;

    SPEW ("RegisterWindowManager: mDOMWindow=%p\n", window);
    mDOMWindow = window;

    // Get ALL events for ALL windows
    gdk_window_add_filter (NULL, gdk_filter_func, this);

    // Just ignore errors for now
    XSetErrorHandler (ErrorHandler);

    // Try to register as window manager
    rv = InitManagerWindow ();
    if (NS_FAILED(rv))
        return rv;

    rv = InitXAtoms ();
    if (NS_FAILED(rv))
        return rv;

    // Check extension versions
    rv = InitXExtensions ();
    if (NS_FAILED(rv))
        return rv;

    // Take over drawing
    rv = InitOverlay ();
    if (NS_FAILED(rv))
        return rv;

    // Select events and manage all existing windows
    rv = InitWindowState ();
    if (NS_FAILED(rv))
        return rv;

    return NS_OK;
}


NS_IMETHODIMP
compzillaControl::InternAtom (const char *property, PRUint32 *value)
{
    *value = (PRUint32) XInternAtom (mXDisplay, property, FALSE);
    return NS_OK;
}


NS_IMETHODIMP
compzillaControl::SetRootWindowProperty (PRInt32 prop, 
                                         PRInt32 type, 
                                         PRUint32 count, 
                                         PRUint32* valueArray)
{
    XChangeProperty (mXDisplay, mXRoot,
                     prop, type, 32,
                     PropModeReplace,
                     (unsigned char*)valueArray, count);

    return NS_OK;
}


NS_IMETHODIMP
compzillaControl::DeleteRootWindowProperty (PRInt32 prop)
{
    XDeleteProperty (mXDisplay, mXRoot,
                     prop);

    return NS_OK;
}


NS_IMETHODIMP
compzillaControl::SendConfigureNotify (PRUint32 xid,
                                       PRUint32 x, PRUint32 y,
                                       PRUint32 width, PRUint32 height,
                                       PRUint32 border,
                                       PRBool overrideRedirect)
{
    XEvent ev;

    memset (&ev, 0, sizeof (ev));

    ev.type = ConfigureNotify;
    ev.xconfigure.display = mXDisplay;
    ev.xconfigure.window = xid;
    ev.xconfigure.event = xid;
    ev.xconfigure.x = x;
    ev.xconfigure.y = y;
    ev.xconfigure.width = width;
    ev.xconfigure.height = height;
    ev.xconfigure.border_width = border;
    ev.xconfigure.above = None;
    ev.xconfigure.override_redirect = overrideRedirect;

    SPEW ("SendConfigureNotify (window=%p, x=%d, y=%d, width=%d, height=%d, "
          "border=%d, override=%d)\n",
          ev.xconfigure.window,
          ev.xconfigure.x,
          ev.xconfigure.y,
          ev.xconfigure.width,
          ev.xconfigure.height,
          ev.xconfigure.border_width,
          ev.xconfigure.override_redirect);

    XSendEvent (mXDisplay, xid, False, StructureNotifyMask, &ev);
}


NS_IMETHODIMP
compzillaControl::Kill (PRUint32 xid)
{
    Unmap (xid);
    XDestroyWindow (mXDisplay, xid);
}


NS_IMETHODIMP
compzillaControl::MoveToTop (PRUint32 xid)
{
    XWindowChanges changes;

    changes.sibling = None;
    changes.stack_mode = Above;

    XConfigureWindow (mXDisplay, xid,
                      CWStackMode,
                      &changes);
}


NS_IMETHODIMP
compzillaControl::MoveToBottom (PRUint32 xid)
{
    XWindowChanges changes;

    changes.sibling = None;
    changes.stack_mode = Below;

    XConfigureWindow (mXDisplay, xid,
                      CWStackMode,
                      &changes);
}


NS_IMETHODIMP
compzillaControl::Configure (PRUint32 xid,
                             PRUint32 x, PRUint32 y,
                             PRUint32 width, PRUint32 height,
                             PRUint32 border)
{
    XWindowChanges changes;

    changes.x = x;
    changes.y = y;
    changes.width = width;
    changes.height = height;
    changes.border_width = border;

    SPEW ("Calling XConfigureWindow (window=%p, x=%d, y=%d, "
          "width=%d, height=%d, border=%d)\n",
          xid, x, y, width, height, border);

    XConfigureWindow (mXDisplay, xid,
                      (CWX | CWY | CWWidth | CWHeight | CWBorderWidth),
                      &changes);
}


NS_IMETHODIMP
compzillaControl::Map (PRUint32 xid)
{
    XMapWindow (mXDisplay, xid);
}


NS_IMETHODIMP
compzillaControl::Unmap (PRUint32 xid)
{
    XUnmapWindow (mXDisplay, xid);
}


NS_IMETHODIMP 
compzillaControl::AddObserver (compzillaIControlObserver *aObserver)
{
    SPEW ("compzillaWindow::AddObserver %p - %p\n", this, aObserver);

    mObservers.AppendObject (aObserver);

    /* 
     * When initially adding an observer, call windowCreate for all existing
     * windows.
     */
    mWindowMap.Enumerate (&compzillaControl::CallWindowCreateCb, aObserver);

    return NS_OK;
}


NS_IMETHODIMP 
compzillaControl::RemoveObserver (compzillaIControlObserver *aObserver)
{
    SPEW ("RemoveObserver control=%p, observer=%p\n", this, aObserver);

    // Allow a caller to remove O(N^2) behavior by removing end-to-start.
    for (PRUint32 i = mObservers.Count() - 1; i != PRUint32(-1); --i) {
        if (mObservers.ObjectAt(i) == aObserver) {
            mObservers.RemoveObjectAt (i);
            break;
        }
    }

    return NS_OK;
}


/*
 *
 * Private methods...
 *
 */

GdkFilterReturn
compzillaControl::gdk_filter_func (GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
    compzillaControl *control = reinterpret_cast<compzillaControl*>(data);
    return control->Filter (xevent, event);
}


nsresult
compzillaControl::GetNativeWidget(nsIDOMWindow *window, nsIWidget **widget)
{
#ifdef MOZILLA_1_8_BRANCH
    nsCOMPtr<nsIScriptGlobalObject> global = do_QueryInterface(window);
#else
    nsCOMPtr<nsPIDOMWindow> global = do_QueryInterface(window);
#endif

    if (global) {
        nsCOMPtr<nsIBaseWindow> baseWin = do_QueryInterface(global->GetDocShell());
        if (baseWin) {
            baseWin->GetMainWidget(widget);
            return NS_OK;
        }
    }

    return NS_ERROR_FAILURE;
}


GdkWindow *
compzillaControl::GetNativeWindow(nsIDOMWindow *window)
{
    nsCOMPtr<nsIWidget> widget;

    if (NS_OK == GetNativeWidget (window, getter_AddRefs (widget))) {
        GdkWindow *iframe = (GdkWindow *)widget->GetNativeData (NS_NATIVE_WINDOW);
        GdkWindow *toplevel = gdk_window_get_toplevel (iframe);

        SPEW ("GetNativeWindow: toplevel=0x%0x, iframe=0x%0x\n", 
              GDK_DRAWABLE_XID (toplevel), GDK_DRAWABLE_XID (iframe));
        return toplevel;
    }

    WARNING ("Could not get GdkWindow for nsIDOMWindow %p\n", window);
    return NULL;
}


nsresult
compzillaControl::InitXAtoms ()
{
    if (!XInternAtoms (mXDisplay, 
                       atom_names, sizeof (atom_names) / sizeof (atom_names[0]), 
                       False, 
                       atoms.a)) {
        return NS_ERROR_FAILURE;
    }
    return NS_OK;
}


nsresult
compzillaControl::InitXExtensions ()
{
    int opcode;
    if (!XQueryExtension (mXDisplay, COMPOSITE_NAME, &opcode, 
                          &composite_event, &composite_error)) {
	ERROR ("No composite extension\n");
        return NS_ERROR_NO_COMPOSITE_EXTENSTION;
    }

    int	composite_major, composite_minor;
    XCompositeQueryVersion (mXDisplay, &composite_major, &composite_minor);
    if (composite_major == 0 && composite_minor < 2) {
        ERROR ("Old composite extension does not support XCompositeGetOverlayWindow\n");
        return NS_ERROR_NO_COMPOSITE_EXTENSTION;
    }
    SPEW ("composite extension: major = %d, minor = %d, opcode = %d, event = %d, error = %d\n",
           composite_major, composite_minor, opcode, composite_event, composite_error);

    if (!XDamageQueryExtension (mXDisplay, &damage_event, &damage_error)) {
	ERROR ("No damage extension\n");
        return NS_ERROR_NO_DAMAGE_EXTENSTION;
    }
    SPEW ("damage extension: event = %d, error = %d\n",
           damage_event, damage_error);

    if (!XFixesQueryExtension (mXDisplay, &xfixes_event, &xfixes_error)) {
	ERROR ("No XFixes extension\n");
        return NS_ERROR_NO_XFIXES_EXTENSTION;
    }
    SPEW ("xfixes extension: event = %d, error = %d\n",
           xfixes_event, xfixes_error);

    if (!XShapeQueryExtension (mXDisplay, &shape_event, &shape_error)) {
	ERROR ("No Shaped window extension\n");
        return NS_ERROR_NO_SHAPE_EXTENSTION;
    }
    SPEW ("shape extension: event = %d, error = %d\n",
           shape_event, shape_error);

    return NS_OK;
}


nsresult
compzillaControl::InitWindowState ()
{
    XGrabServer (mXDisplay);

    long ev_mask = (SubstructureRedirectMask |
                    SubstructureNotifyMask |
                    StructureNotifyMask |
                    ResizeRedirectMask |
                    PropertyChangeMask |
                    FocusChangeMask);
    XSelectInput (mXDisplay, mXRoot, ev_mask);

    if (ClearErrors (mXDisplay)) {
        WARNING ("Another window manager is already running on screen: %d\n", 1);

        ev_mask &= ~(SubstructureRedirectMask);
        XSelectInput (mXDisplay, mXRoot, ev_mask);
    }

    if (mIsWindowManager) {
        // Start managing any existing windows
        Window root_notused, parent_notused;
        Window *children;
        unsigned int nchildren;

        XQueryTree (mXDisplay, 
                    mXRoot, 
                    &root_notused, 
                    &parent_notused, 
                    &children, 
                    &nchildren);

        for (int i = 0; i < nchildren; i++) {
            if (children[i] != mOverlay
                && children[i] != GDK_WINDOW_XID (mMainwin))
            AddWindow (children[i]);
        }

        XFree (children);
    }

    // Set the root window cursor, used when windows don't specify one.
    Cursor normal = XCreateFontCursor (mXDisplay, XC_left_ptr);
    XDefineCursor (mXDisplay, mXRoot, normal);

    // Get notified of global cursor changes.  
    // FIXME: This is not very useful, as X has no way of fetching the Cursor
    // for a given window.
    //XFixesSelectCursorInput (mXDisplay, mXRoot, XFixesDisplayCursorNotifyMask);

    XUngrabServer (mXDisplay);

    return NS_OK;
}


bool
compzillaControl::ReplaceSelectionOwner (Window newOwner, Atom atom)
{
    Window currentOwner = XGetSelectionOwner (mXDisplay, atom);
    if (currentOwner != None) {
        // Listen for destory on existing selection owner window
        XSelectInput (mXDisplay, currentOwner, StructureNotifyMask);
    }

    XSetSelectionOwner (mXDisplay, atom, newOwner, CurrentTime);

    if (XGetSelectionOwner (mXDisplay, atom) != newOwner)
        return FALSE;

    // Send client message indicating that we are now the WM
    XClientMessageEvent ev;
    ev.type = ClientMessage;
    ev.window = mXRoot;
    ev.message_type = XInternAtom (mXDisplay, "MANAGER", FALSE);
    ev.format = 32;
    ev.data.l[0] = CurrentTime;
    ev.data.l[1] = atom;

    XSendEvent (mXDisplay, mXRoot, False, StructureNotifyMask, (XEvent*)&ev);

    // Wait for the currentOwner window to go away
    if (currentOwner != None) {
        XEvent event;
        do {
            XWindowEvent (mXDisplay, currentOwner, StructureNotifyMask, &event);
        } while (event.type != DestroyNotify);
    }

    return TRUE;
}


nsresult
compzillaControl::InitManagerWindow ()
{
    SPEW ("InitManagerWindow\n");

    XSetWindowAttributes attrs;
    attrs.override_redirect = True;
    attrs.event_mask = PropertyChangeMask;
  
    // FIXME: Do this for each screen
    mManagerWindow = XCreateWindow (mXDisplay,
                                    mXRoot,
                                    -100, -100, 1, 1,
                                    0,
                                    CopyFromParent,
                                    CopyFromParent,
                                    (Visual *)CopyFromParent,
                                    CWOverrideRedirect | CWEventMask,
                                    &attrs);

    char *atom_name;
    Atom atom;

    // FIXME: Handle screens
    atom_name = g_strdup_printf ("WM_S%d", 0);
    atom = XInternAtom (mXDisplay, atom_name, FALSE);
    g_free (atom_name);

    mIsWindowManager = ReplaceSelectionOwner (mManagerWindow, atom);
    if (!mIsWindowManager) {
        ERROR ("Couldn't acquire window manager selection");
        goto error;
    }

    // FIXME: Handle screens
    atom_name = g_strdup_printf ("_NET_WM_CM_S%d", 0);
    atom = XInternAtom (mXDisplay, atom_name, FALSE);
    g_free (atom_name);

    mIsCompositor = ReplaceSelectionOwner (mManagerWindow, atom);
    if (!mIsCompositor) {
        ERROR ("Couldn't acquire compositor selection");
        goto error;
    }

    return NS_OK;

 error:
    XDestroyWindow (mXDisplay, mManagerWindow);
    mManagerWindow = None;

    return NS_ERROR_FAILURE;
}


nsresult
compzillaControl::InitOverlay ()
{
    // Create the overlay window, and make the X server stop drawing windows
    mOverlay = XCompositeGetOverlayWindow (mXDisplay, mXRoot);
    if (!mOverlay) {
        ERROR ("failed call to XCompositeGetOverlayWindow\n");
        return NS_ERROR_FAILURE;
    }

    mMainwin = GetNativeWindow (mDOMWindow);
    if (!mMainwin) {
        ERROR ("failed to get native window\n");
        return NS_ERROR_FAILURE;
    }

    gdk_window_ref (mMainwin);
    gdk_window_set_override_redirect(mMainwin, true);

    // Put the our window into the overlay
    XReparentWindow (mXDisplay, GDK_DRAWABLE_XID (mMainwin), mOverlay, 0, 0);

    ShowOverlay (true);
    EnableOverlayInput (true);

    return NS_OK;
}


void 
compzillaControl::ShowOverlay (bool show)
{
    XserverRegion xregion;

    if (show) {
        XRectangle rect = { 0, 0, 
                            DisplayWidth (mXDisplay, 0), 
                            DisplayHeight (mXDisplay, 0) };
        xregion = XFixesCreateRegion (mXDisplay, &rect, 1);
    } else {
        xregion = XFixesCreateRegion (mXDisplay, NULL, 0);
    }

    XFixesSetWindowShapeRegion (mXDisplay,
                                mOverlay,
                                ShapeBounding,
                                0, 0, 
                                xregion);

    XFixesDestroyRegion (mXDisplay, xregion);
}


void
compzillaControl::EnableOverlayInput (bool receiveInput)
{
    XserverRegion xregion;

    if (receiveInput) {
        XRectangle rect = { 0, 0, 
                            DisplayWidth (mXDisplay, 0), 
                            DisplayHeight (mXDisplay, 0) };
        xregion = XFixesCreateRegion (mXDisplay, &rect, 1);
    } else {
        xregion = XFixesCreateRegion (mXDisplay, NULL, 0);
    }

    XFixesSetWindowShapeRegion (mXDisplay,
                                mOverlay,
                                ShapeInput,
                                0, 0, 
                                xregion);

    XFixesDestroyRegion (mXDisplay, xregion);
}


int 
compzillaControl::ErrorHandler (Display *dpy, XErrorEvent *err)
{
    sErrorCnt++;

    char str[128];
    char *name = 0;
    int  o;

    XGetErrorDatabaseText (dpy, "XlibMessage", "XError", "", str, 128);
    ERROR ("%s", str);

    o = err->error_code - damage_error;
    switch (o) {
    case BadDamage:
	name = "BadDamage";
	break;
    default:
	break;
    }

    if (name) {
        ERROR (": %s\n  ", name);
    } else {
	XGetErrorText (dpy, err->error_code, str, 128);
	ERROR (": %s\n  ", str);
    }

    XGetErrorDatabaseText (dpy, "XlibMessage", "ResourceID", "%d", str, 128);
    ERROR (str, err->resourceid);
    ERROR ("\n  ");

    XGetErrorDatabaseText (dpy, "XlibMessage", "MajorCode", "%d", str, 128);
    ERROR (str, err->request_code);

    sprintf (str, "%d", err->request_code);
    XGetErrorDatabaseText (dpy, "XRequest", str, "", str, 128);
    if (strcmp (str, "") != 0) {
	ERROR (" (%s)", str);
    }
    ERROR ("\n  ");

    XGetErrorDatabaseText (dpy, "XlibMessage", "MinorCode", "%d", str, 128);
    ERROR (str, err->minor_code);
    ERROR ("\n");

    return 0;
}


int 
compzillaControl::ClearErrors (Display *dpy)
{
    XSync (dpy, FALSE);

    int lastcnt = sErrorCnt;
    sErrorCnt = 0;
    return lastcnt;
}


PLDHashOperator 
compzillaControl::CallWindowCreateCb (const PRUint32& key, 
                                      nsRefPtr<compzillaWindow>& win, 
                                      void *userdata)
{
    compzillaIControlObserver *observer = 
        static_cast<compzillaIControlObserver *>(userdata);
    compzillaIWindow *iwin = win;
    
    observer->WindowCreate (iwin);
}


void
compzillaControl::AddWindow (Window win)
{
    gdk_error_trap_push ();

    XWindowAttributes attrs;
    if (!XGetWindowAttributes(mXDisplay, win, &attrs)) {
        gdk_error_trap_pop ();
        return;
    }

    if (attrs.c_class == InputOnly) {
        INFO ("Ignoring InputOnly window %p\n", win);
        gdk_error_trap_pop ();
        return;
    }

    nsRefPtr<compzillaWindow> compwin;
    if (NS_OK != CZ_NewCompzillaWindow (mXDisplay, win, &attrs, getter_AddRefs (compwin))) {
        gdk_error_trap_pop ();
        return;
    }

    if (gdk_error_trap_pop ()) {
        ERROR ("Errors encountered registering window %p\n", win);
        return;
    }

    INFO ("Adding window %p %s\n", win, 
          attrs.override_redirect ? "(override-redirect)" : "");

    mWindowMap.Put (win, compwin);
    
    compzillaIWindow *iwin = compwin;

    for (PRUint32 i = mObservers.Count() - 1; i != PRUint32(-1); --i) {
        nsCOMPtr<compzillaIControlObserver> observer = mObservers.ObjectAt(i);
        observer->WindowCreate (iwin);
    }
}


void
compzillaControl::DestroyWindow (Window win)
{
    nsRefPtr<compzillaWindow> compwin = FindWindow (win);
    if (compwin) {
        compzillaIWindow *iwin = compwin;

        for (PRUint32 i = mObservers.Count() - 1; i != PRUint32(-1); --i) {
            nsCOMPtr<compzillaIControlObserver> observer = mObservers.ObjectAt(i);
            observer->WindowDestroy (iwin);
        }

        compwin->Destroyed ();
        mWindowMap.Remove (win);
    }
}


void
compzillaControl::MapWindow (Window win, bool override_redirect)
{
    nsRefPtr<compzillaWindow> compwin = FindWindow (win);
    if (compwin) {
        compwin->Mapped (override_redirect);
    }
}


void
compzillaControl::UnmapWindow (Window win)
{
    nsRefPtr<compzillaWindow> compwin = FindWindow (win);
    if (compwin) {
        compwin->Unmapped ();
    }
}


void 
compzillaControl::ConfigureWindow (bool isNotify,
                                   Window win,
                                   PRInt32 x, PRInt32 y,
                                   PRInt32 width, PRInt32 height,
                                   PRInt32 border,
                                   Window aboveWin,
                                   bool override_redirect)
{
    nsRefPtr<compzillaWindow> compwin = FindWindow (win);
    if (compwin) {
        nsRefPtr<compzillaWindow> aboveCompWin = FindWindow (aboveWin);

        compwin->Configured (isNotify,
                             x, y,
                             width, height,
                             border,
                             aboveCompWin,
                             override_redirect);
    } else if (!isNotify) {
        // Window we are not monitoring, so send the configure.
        Configure (win, x, y, width, height, border);
    }
}


void
compzillaControl::PropertyChanged (Window win, Atom prop, bool deleted)
{
    nsRefPtr<compzillaWindow> compwin = FindWindow (win);
    if (compwin) {
        compwin->PropertyChanged (prop, deleted);
    }
}


void
compzillaControl::DamageWindow (Window win, XRectangle *rect)
{
    nsRefPtr<compzillaWindow> compwin = FindWindow (win);
    if (compwin) {
        compwin->Damaged (rect);
    }
}


void
compzillaControl::ClientMessaged (Window win,
                                  Atom type, int format, long *data/*[5]*/)
{
    nsRefPtr<compzillaWindow> compwin = FindWindow (win);
    if (compwin) {
        compwin->ClientMessaged (type, format, data);
    }
    else {
        for (PRUint32 i = mObservers.Count() - 1; i != PRUint32(-1); --i) {
            nsCOMPtr<compzillaIControlObserver> observer = mObservers.ObjectAt(i);
            observer->RootClientMessageRecv ((long)type,
                                             format,
                                             data[0],
                                             data[1],
                                             data[2],
                                             data[3],
                                             data[4]);
        }
    }
}


already_AddRefed<compzillaWindow>
compzillaControl::FindWindow (Window win)
{
    compzillaWindow *compwin;
    mWindowMap.Get(win, &compwin);
    return compwin;
}


void
compzillaControl::PrintEvent (XEvent *x11_event)
{
    switch (x11_event->type) {
    case ClientMessage:
        SPEW ("ClientMessage: window=0x%0x, type=%s, format=%d\n",
              x11_event->xclient.window,
              XGetAtomName (mXDisplay, x11_event->xclient.message_type),
              x11_event->xclient.format);
        break;
    case CreateNotify:
        SPEW ("CreateNotify: window=0x%0x, parent=%p, x=%d, y=%d, width=%d, height=%d, "
              "override=%d\n",
               x11_event->xcreatewindow.window,
               x11_event->xcreatewindow.parent,
               x11_event->xcreatewindow.x,
               x11_event->xcreatewindow.y,
               x11_event->xcreatewindow.width,
               x11_event->xcreatewindow.height,
               x11_event->xcreatewindow.override_redirect);
        break;
    case DestroyNotify:
        SPEW ("DestroyNotify: event_win=%p, window=%p\n", 
              x11_event->xdestroywindow.event,
              x11_event->xdestroywindow.window);
        break;
    case ConfigureNotify:
        SPEW ("ConfigureNotify: window=%p, x=%d, y=%d, width=%d, height=%d, "
              "border=%d, above=%p, override=%d\n",
              x11_event->xconfigure.window,
              x11_event->xconfigure.x,
              x11_event->xconfigure.y,
              x11_event->xconfigure.width,
              x11_event->xconfigure.height,
              x11_event->xconfigure.border_width,
              x11_event->xconfigure.above,
              x11_event->xconfigure.override_redirect);
        break;
    case ConfigureRequest:
        SPEW ("ConfigureRequest: window=%p, parent=%p, x=%d, y=%d, width=%d, height=%d, "
              "border=%d, above=%p\n",
              x11_event->xconfigurerequest.window,
              x11_event->xconfigurerequest.parent,
              x11_event->xconfigurerequest.x,
              x11_event->xconfigurerequest.y,
              x11_event->xconfigurerequest.width,
              x11_event->xconfigurerequest.height,
              x11_event->xconfigurerequest.border_width,
              x11_event->xconfigurerequest.above);
        break;
    case ReparentNotify:
        SPEW ("ReparentNotify: window=%p, parent=%p, x=%d, y=%d, override_redirect=%d\n",
              x11_event->xreparent.window,
              x11_event->xreparent.parent,
              x11_event->xreparent.x,
              x11_event->xreparent.y,
              x11_event->xreparent.override_redirect);
        break;
    case MapRequest:
        SPEW ("MapRequest: window=%p, parent=%p\n",
              x11_event->xmaprequest.window,
              x11_event->xmaprequest.parent);
        break;
    case MapNotify:
        SPEW ("MapNotify: eventwin=%p, window=%p, override=%d\n", 
              x11_event->xmap.event, 
              x11_event->xmap.window, 
              x11_event->xmap.override_redirect);
        break;
    case UnmapNotify:
        SPEW ("UnmapNotify: eventwin=%p, window=%p, from_configure=%d\n", 
              x11_event->xunmap.event,
              x11_event->xunmap.window,
              x11_event->xunmap.from_configure);
        break;
    case PropertyNotify:
#if DEBUG_EVENTS // Too much noise
        SPEW ("PropertyChange: window=0x%0x, atom=%s\n", 
              x11_event->xproperty.window, 
              XGetAtomName(x11_event->xany.display, x11_event->xproperty.atom));
#endif
        break;
    case _KeyPress:
    case KeyRelease:
        SPEW ("%s: window=0x%0x, subwindow=0x%0x, x=%d, y=%d, state=%d, keycode=%d\n", 
              x11_event->xkey.type == _KeyPress ? "KeyPress" : "KeyRelease",
              x11_event->xkey.window, 
              x11_event->xkey.subwindow, 
              x11_event->xkey.state, x11_event->xkey.keycode);
        break;
    case ButtonPress:
    case ButtonRelease:
        SPEW ("%s: window=%p, subwindow=%s, x=%d, y=%d, x_root=%d, y_root=%d, "
              "state=%d, button=%d\n", 
              x11_event->xbutton.type == ButtonPress ? "ButtonPress" : "ButtonRelease", 
              x11_event->xbutton.window, 
              x11_event->xbutton.subwindow, 
              x11_event->xbutton.x, 
              x11_event->xbutton.y,
              x11_event->xbutton.x_root, 
              x11_event->xbutton.y_root,
              x11_event->xbutton.state,
              x11_event->xbutton.button);
        break;
    case MotionNotify:
#if DEBUG_EVENTS // Too much noise
        SPEW ("MotionNotify: window=0x%0x, x=%d, y=%d, state=%d\n", 
              x11_event->xmotion.window, x11_event->xmotion.x, x11_event->xmotion.y, 
              x11_event->xmotion.state);
#endif
        break;
    case _FocusIn:
    case _FocusOut:
        SPEW ("%s: %s window=0x%0x, mode=%d, detail=%d\n", 
              x11_event->xfocus.type == _FocusIn ? "FocusIn" : "FocusOut",
              x11_event->xfocus.send_event ? "(SYNTHETIC)" : "REAL",
              x11_event->xfocus.window, 
              x11_event->xfocus.mode, 
              x11_event->xfocus.detail);
        break;
    case EnterNotify:
    case LeaveNotify:
        SPEW ("%s: %s window=0x%0x, subwindow=0x%0x, mode=%d, detail=%d, focus=%s, "
              "state=%d\n", 
              x11_event->xcrossing.type == EnterNotify ? "EnterNotify" : "LeaveNotify",
              x11_event->xcrossing.send_event ? "(SYNTHETIC)" : "REAL",
              x11_event->xcrossing.window, 
              x11_event->xcrossing.subwindow, 
              x11_event->xcrossing.mode, 
              x11_event->xcrossing.detail,
              x11_event->xcrossing.focus ? "TRUE" : "FALSE",
              x11_event->xcrossing.state);
        break;    
    case Expose:
        SPEW ("Expose: window=%p, count=%d\n", 
              x11_event->xexpose.window,
              x11_event->xexpose.count);
        break;
    case VisibilityNotify:
        SPEW ("VisibilityNotify: window=%p, state=%d\n", 
              x11_event->xvisibility.window,
              x11_event->xvisibility.state);
        break;
    default:
        if (x11_event->type == damage_event + XDamageNotify) {
            XDamageNotifyEvent *damage_ev = (XDamageNotifyEvent *) x11_event;
            SPEW_EVENT ("DAMAGE: drawable=%p, x=%d, y=%d, width=%d, height=%d\n", 
                        damage_ev->drawable, damage_ev->area.x, damage_ev->area.y, 
                        damage_ev->area.width, damage_ev->area.height);
        }
        else if (x11_event->type == xfixes_event + XFixesCursorNotify) {
            XFixesCursorNotifyEvent *cursor_ev = (XFixesCursorNotifyEvent *) x11_event;
            const char *cursor_val = gdk_x11_get_xatom_name (cursor_ev->cursor_name);

            SPEW ("CursorNotify: window=%p, cursor='%s'\n", cursor_ev->window, cursor_val);
        }
        else if (x11_event->type == shape_event + ShapeNotify) {
            XShapeEvent *shape_ev = (XShapeEvent *) x11_event;

            SPEW ("ShapeNotify: window=%p, kind=%s, shaped=%s, x=%d, y=%d, width=%d, height=%d\n",
                  shape_ev->window,
                  shape_ev->kind == ShapeBounding ? "bounding" : "clip",
                  shape_ev->shaped ? "TRUE" : "FALSE",
                  shape_ev->x, shape_ev->y, shape_ev->width, shape_ev->height);
        }
        else {
            ERROR ("Unhandled window event %d\n", x11_event->type);
        }
        break;
    }
}


GdkFilterReturn
compzillaControl::Filter (GdkXEvent *xevent, GdkEvent *event)
{
    XEvent *x11_event = (XEvent*) xevent;

    PrintEvent (x11_event);

    if (x11_event->xany.window == GDK_DRAWABLE_XID (this->mMainwin) &&
        x11_event->xany.type != _FocusIn && 
        x11_event->xany.type != _FocusOut) {
        ERROR ("IGNORING MAINWIN EVENT TYPE: %d", x11_event->xany.type);
        return GDK_FILTER_CONTINUE;
    }

    if (x11_event->xany.window == mManagerWindow) {
        ERROR ("Ignoring event on window manager internal: %p\n", mManagerWindow);
        return GDK_FILTER_REMOVE;
    }
    else if (x11_event->xany.window == mOverlay) {
        ERROR ("Ignoring event on overlay window: %p\n", mOverlay);
        return GDK_FILTER_REMOVE;
    }                                                                    
    else if (x11_event->xany.window == mMainwinParent) {
        ERROR ("Ignoring event on main window parent: %p\n", mMainwinParent);
        return GDK_FILTER_REMOVE;
    }                                                                    

    switch (x11_event->type) {
    case ClientMessage:
        ClientMessaged (x11_event->xclient.window,
                        x11_event->xclient.message_type,
                        x11_event->xclient.format,
                        x11_event->xclient.data.l);
        return GDK_FILTER_REMOVE;

    case CreateNotify:
        if (x11_event->xcreatewindow.window == GDK_DRAWABLE_XID (this->mMainwin)) {
            WARNING ("CreateNotify: discarding event on mainwin\n");
            return GDK_FILTER_REMOVE;
        }

        if (x11_event->xcreatewindow.parent != mXRoot)
            break;

        if (!XCheckTypedWindowEvent (mXDisplay, 
                                     x11_event->xcreatewindow.window,
                                     DestroyNotify, 
                                     x11_event) &&
            !XCheckTypedWindowEvent (mXDisplay, 
                                     x11_event->xcreatewindow.window,
                                     ReparentNotify, 
                                     x11_event)) {
            AddWindow (x11_event->xcreatewindow.window);
        }

        return GDK_FILTER_REMOVE;

    case DestroyNotify:
        DestroyWindow (x11_event->xdestroywindow.window);
        return GDK_FILTER_REMOVE;

    case ConfigureNotify:
#if CLEAR_PENDING_X_EVENTS
        while (XCheckTypedWindowEvent (mXDisplay, 
                                       x11_event->xconfigure.window,
                                       ConfigureNotify, 
                                       x11_event)) {
            // Do nothing
        }
#endif

        // This is driven by compzilla or from an override_redirect itself.
        ConfigureWindow (true,
                         x11_event->xconfigure.window,
                         x11_event->xconfigure.x,
                         x11_event->xconfigure.y,
                         x11_event->xconfigure.width,
                         x11_event->xconfigure.height,
                         x11_event->xconfigure.border_width,
                         x11_event->xconfigure.above,
                         x11_event->xconfigure.override_redirect);
        break;

    case ConfigureRequest:
        if (x11_event->xconfigurerequest.parent != mXRoot)
            break;

        while (XCheckTypedWindowEvent (mXDisplay, 
                                       x11_event->xconfigurerequest.window,
                                       ConfigureRequest, 
                                       x11_event)) {
            // Do nothing
        }

        // This is driven by the X app, not compzilla.
        ConfigureWindow (false,
                         x11_event->xconfigurerequest.window,
                         x11_event->xconfigurerequest.x,
                         x11_event->xconfigurerequest.y,
                         x11_event->xconfigurerequest.width,
                         x11_event->xconfigurerequest.height,
                         x11_event->xconfigurerequest.border_width,
                         x11_event->xconfigurerequest.above,
                         false);

        return GDK_FILTER_REMOVE;

    case ReparentNotify:
        if (x11_event->xreparent.window == GDK_DRAWABLE_XID (this->mMainwin)) {
            // Keep the new parent so we can ignore events on it.
            mMainwinParent = x11_event->xreparent.parent;

            // Let the mainwin draw normally by unredirecting our new parent.
            DestroyWindow (x11_event->xreparent.parent);

            WARNING ("ReparentNotify: discarding event on mainwin\n");
            return GDK_FILTER_REMOVE;
        }

        if (x11_event->xreparent.window == mMainwinParent) {
            WARNING ("ReparentNotify: discarding event on mainwin's parent\n");
            return GDK_FILTER_REMOVE;
        }

        if (x11_event->xreparent.parent == mXRoot) {
            nsRefPtr<compzillaWindow> toplevel = FindWindow (x11_event->xreparent.window);
            if (toplevel) {
                ERROR ("Reparent of existing toplevel window\n");
            } else {
                AddWindow (x11_event->xreparent.window);
            }
        } else {
            Window goneWin = x11_event->xreparent.window;

            /* 
             * This is the only case where a window is removed but not
             * destroyed. We must remove our event mask and all passive
             * grabs.  -- compiz
             */
            XSelectInput (mXDisplay, goneWin, NoEventMask);
            XShapeSelectInput (mXDisplay, goneWin, NoEventMask);
            XUngrabButton (mXDisplay, AnyButton, AnyModifier, goneWin);

            DestroyWindow (goneWin);
        }
        
        return GDK_FILTER_REMOVE;

    case MapRequest:
        if (x11_event->xmaprequest.parent != mXRoot)
            break;

        if (!XCheckTypedWindowEvent (mXDisplay, 
                                     x11_event->xmaprequest.window,
                                     UnmapNotify, 
                                     x11_event)) {
            XMapWindow (mXDisplay, x11_event->xmaprequest.window);
        }
        return GDK_FILTER_REMOVE;

    case MapNotify:
        if (!XCheckTypedWindowEvent (mXDisplay, 
                                     x11_event->xmap.window,
                                     UnmapNotify, 
                                     x11_event)) {
            MapWindow (x11_event->xmap.window, x11_event->xmap.override_redirect);
        }
        return GDK_FILTER_REMOVE;

    case UnmapNotify:
        if (!XCheckTypedWindowEvent (mXDisplay, 
                                     x11_event->xunmap.window,
                                     MapNotify, 
                                     x11_event)) {
            UnmapWindow (x11_event->xunmap.window);
        }
        return GDK_FILTER_REMOVE;

    case PropertyNotify:
        PropertyChanged (x11_event->xproperty.window, 
                         x11_event->xproperty.atom, 
                         x11_event->xproperty.state == PropertyDelete);
        break;

    case _FocusIn:
        // On FocusIn of root window due to ungrab, start receiving input again
        if (x11_event->xfocus.window == mXRoot &&
            x11_event->xfocus.mode == NotifyUngrab) {
            SPEW ("FocusIn: focusing main window!\n");
            EnableOverlayInput (true);
        }
        break;

    case _FocusOut:
        // On FocusOut of Moz window due to grab, kill all input
        if (x11_event->xfocus.window == GDK_DRAWABLE_XID (this->mMainwin) &&
            x11_event->xfocus.mode == NotifyGrab) {
            SPEW ("FocusOut: UNfocusing main window!\n");
            EnableOverlayInput (false);
        }
        break;

    default:
        if (x11_event->type == damage_event + XDamageNotify) {
            XDamageNotifyEvent *damage_ev = (XDamageNotifyEvent *) x11_event;

            int cnt = 0;
            do {
                DamageWindow (damage_ev->drawable, &damage_ev->area);
                cnt++;
            }
#if CLEAR_PENDING_X_EVENTS
            while (XCheckTypedEvent (mXDisplay, 
                                     damage_event + XDamageNotify,
                                     x11_event));
#else
            while (0);
#endif

            if (cnt > 1) {
                SPEW_EVENT ("DAMAGE: Handled %d pending events!\n", cnt);
            }

            return GDK_FILTER_REMOVE;
        }
        else if (x11_event->type == xfixes_event + XFixesCursorNotify) {
            XFixesCursorNotifyEvent *cursor_ev = (XFixesCursorNotifyEvent *) x11_event;
            NS_NOTYETIMPLEMENTED ("CursorNotify event");
            return GDK_FILTER_REMOVE;
        }
        else if (x11_event->type == shape_event + ShapeNotify) {
            NS_NOTYETIMPLEMENTED ("ShapeNotify event");
            return GDK_FILTER_REMOVE;
        }
        break;
    }

    return GDK_FILTER_CONTINUE;
}
