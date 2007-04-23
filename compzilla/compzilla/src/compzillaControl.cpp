/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#define MOZILLA_INTERNAL_API

#include "nspr.h"
#include "nsIObserverService.h"

#include "nsIScriptContext.h"
#include "jsapi.h"

#include "compzillaControl.h"
#include "compzillaIRenderingContext.h"
#include "compzillaRenderingContext.h"

// These headers are used for finding the GdkWindow for a DOM window
#include "nsIBaseWindow.h"
#include "nsIDocShell.h"
#include "nsIScriptGlobalObject.h"
#ifndef MOZILLA_1_8_BRANCH
#include "nsPIDOMWindow.h"
#endif
#include "nsIWidget.h"

extern "C" {
#include <gdk/gdkx.h>
#include <gdk/gdkdisplay.h>
#include <gdk/gdkscreen.h>

#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xrender.h>

#define HAVE_XEVIE 0
#define HAVE_XEVIE_WRITEONLY 0
#if HAVE_XEVIE || HAVE_XEVIE_WRITEONLY
#include <X11/extensions/Xevie.h>
#endif
}

#if WITH_SPEW
#define SPEW(format...) printf("   - " format)
#else
#define SPEW(format...)
#endif

#define INFO(format...) printf(" *** " format)
#define WARNING(format...) printf(" !!! " format)
#define ERROR(format...) fprintf(stderr, format)

NS_IMPL_ISUPPORTS2_CI(compzillaControl, compzillaIControl, nsIDOMEventTarget);


int compzillaControl::sErrorCnt;
int compzillaControl::composite_event;
int compzillaControl::composite_error;
int compzillaControl::xevie_event;
int compzillaControl::xevie_error;
int compzillaControl::damage_event;
int compzillaControl::damage_error;
int compzillaControl::xfixes_event;
int compzillaControl::xfixes_error;
int compzillaControl::shape_event;
int compzillaControl::shape_error;
int compzillaControl::render_event;
int compzillaControl::render_error;


compzillaControl::compzillaControl()
    : mWindowCreateEvMgr ("windowcreate", this),
      mWindowDestroyEvMgr ("windowdestroy", this)
{
    mWindowMap.Init(50);
}


compzillaControl::~compzillaControl()
{
    if (mRoot) {
        gdk_window_unref(mRoot);
    }
    if (mMainwin) {
        gdk_window_unref(mMainwin);
    }

#if HAVE_XEVIE || HAVE_XEVIE_WRITEONLY
    // Stop intercepting mouse/keyboard events
    XevieEnd (mXDisplay);
#endif
}


/*
 *
 * compzillaIControl Implementation...
 *
 */

NS_IMETHODIMP
compzillaControl::RegisterWindowManager(nsIDOMWindow *window, compzillaIWindowManager* wm)
{
    Display *dpy;
    nsresult rv = NS_OK;

    SPEW ("RegisterWindowManager\n");

    mDOMWindow = window;
    mWM = wm;

    mRoot = gdk_get_default_root_window();
    mDisplay = gdk_display_get_default ();
    mXDisplay = GDK_DISPLAY_XDISPLAY (mDisplay);

    // Get ALL events for ALL windows
    gdk_window_add_filter (NULL, gdk_filter_func, this);

    // Just ignore errors for now
    XSetErrorHandler (ErrorHandler);

    // Check extension versions
    if (!InitXExtensions ()) {
        ERROR ("InitXExtensions failed");
        exit (1); // return NS_ERROR_UNEXPECTED;
    }

    // Take over drawing
    if (!InitOutputWindow ()) {
        ERROR ("InitOutputWindow failed");
        exit (1); // return NS_ERROR_UNEXPECTED;
    }

    // Register as window manager
    if (!InitWindowState ()) {
        ERROR ("InitWindowState failed");
        exit (1); // return NS_ERROR_UNEXPECTED;
    }

    return rv;
}


NS_IMETHODIMP
compzillaControl::InternAtom (const char *property, PRUint32 *value)
{
    *value = (PRUint32) XInternAtom (mXDisplay, property, FALSE);
    return NS_OK;
}


NS_IMETHODIMP
compzillaControl::GetStringProperty (PRUint32 xid, PRUint32 prop, nsAString& value)
{
    SPEW ("GetStringProperty (prop = %s)\n", XGetAtomName (mXDisplay, prop));

    Atom actual_type;
    int format;
    unsigned long nitems;
    unsigned long bytes_after_return;
    unsigned char *data;

    XGetWindowProperty (mXDisplay, xid, (Atom) prop,
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
compzillaControl::GetAtomProperty (PRUint32 xid, PRUint32 prop, PRUint32* value)
{
    SPEW ("GetAtomProperty (prop = %s)\n", XGetAtomName (mXDisplay, prop));

    Atom actual_type;
    int format;
    unsigned long nitems;
    unsigned long bytes_after_return;
    unsigned char *data;

    XGetWindowProperty (mXDisplay, xid, (Atom)prop,
                        0, BUFSIZ, false, XA_ATOM,
                        &actual_type, &format, &nitems, &bytes_after_return, 
                        &data);

    *value = *(PRUint32*)data;

    return NS_OK;
}


NS_IMETHODIMP
compzillaControl::SendConfigureNotify (PRUint32 xid,
                                       PRUint32 x, PRUint32 y,
                                       PRUint32 width, PRUint32 height,
                                       PRUint32 border)
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
    ev.xconfigure.override_redirect = False; /* XXX */

    SPEW ("SendConfigureNotify (window=%p, x=%d, y=%d, width=%d, height=%d, border=%d, override=%d)\n",
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
compzillaControl::ConfigureWindow (PRUint32 xid,
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

    XConfigureWindow (mXDisplay, xid,
                      (CWX | CWY | CWWidth | CWHeight | CWBorderWidth),
                      &changes);
}


/*
 * 
 * nsIDOMEventTarget Implementation...
 *
 */

NS_IMETHODIMP
compzillaControl::AddEventListener (const nsAString & type, 
                                    nsIDOMEventListener *listener, 
                                    PRBool useCapture)
{
    if (type.EqualsLiteral ("windowcreate")) {
        return mWindowCreateEvMgr.AddEventListener (type, listener);
    }
    return NS_ERROR_INVALID_ARG;
}


NS_IMETHODIMP
compzillaControl::RemoveEventListener (const nsAString & type, 
                                       nsIDOMEventListener *listener, 
                                       PRBool useCapture)
{
    if (type.EqualsLiteral ("windowcreate")) {
        return mWindowCreateEvMgr.RemoveEventListener (type, listener);
    }
    return NS_ERROR_INVALID_ARG;
}


NS_IMETHODIMP
compzillaControl::DispatchEvent (nsIDOMEvent *evt, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
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


nsCOMPtr<nsIWidget>
compzillaControl::GetNativeWidget(nsIDOMWindow *window)
{
#ifdef MOZILLA_1_8_BRANCH
    nsCOMPtr<nsIScriptGlobalObject> global = do_QueryInterface(window);
#else
    nsCOMPtr<nsPIDOMWindow> global = do_QueryInterface(window);
#endif

    if (global) {
        nsCOMPtr<nsIBaseWindow> baseWin = do_QueryInterface(global->GetDocShell());
        if (baseWin) {
            nsCOMPtr<nsIWidget> widget;
            baseWin->GetMainWidget(getter_AddRefs(widget));
            return widget;
        }
    }

    return NULL;
}


GdkWindow *
compzillaControl::GetNativeWindow(nsIDOMWindow *window)
{
    nsCOMPtr<nsIWidget> widget = GetNativeWidget (window);
    if (widget) {
        GdkWindow *iframe = (GdkWindow *)widget->GetNativeData (NS_NATIVE_WINDOW);
        GdkWindow *toplevel = gdk_window_get_toplevel (iframe);

        SPEW ("GetNativeWindow: toplevel=0x%0x, iframe=0x%0x\n", 
              GDK_DRAWABLE_XID (toplevel), GDK_DRAWABLE_XID (iframe));
        return toplevel;
    }

    WARNING ("Could not get GdkWindow for nsIDOMWindow %p\n", window);
    return NULL;
}


bool
compzillaControl::InitXExtensions ()
{
#define ERR_RET(_str) do { ERROR (_str); return false; } while (0)

    if (!XRenderQueryExtension (mXDisplay, &render_event, &render_error)) {
	ERR_RET ("No render extension\n");
    }

    int opcode;
    if (!XQueryExtension (mXDisplay, COMPOSITE_NAME, &opcode, 
                          &composite_event, &composite_error)) {
	ERR_RET ("No composite extension\n");
    }

    int	composite_major, composite_minor;
    XCompositeQueryVersion (mXDisplay, &composite_major, &composite_minor);
    if (composite_major == 0 && composite_minor < 2) {
        ERR_RET ("Old composite extension does not support XCompositeGetOverlayWindow\n");
    }
    SPEW ("composite extension: major = %d, minor = %d, opcode = %d, event = %d, error = %d\n",
           composite_major, composite_minor, opcode, composite_event, composite_error);

    if (!XDamageQueryExtension (mXDisplay, &damage_event, &damage_error)) {
	ERR_RET ("No damage extension\n");
    }
    SPEW ("damage extension: event = %d, error = %d\n",
           damage_event, damage_error);

#if HAVE_XEVIE || HAVE_XEVIE_WRITEONLY
    if (!XQueryExtension (mXDisplay, "XEVIE", &opcode, &xevie_event, &xevie_error)) {
	ERR_RET ("No Xevie extension\n");
    }

    int evie_major, evie_minor;
    if (!XevieQueryVersion (mXDisplay, &evie_major, &evie_minor)) {
	ERR_RET ("No Xevie extension\n");
    }
    SPEW ("xevie extension: major = %d, minor = %d, opcode = %d, event = %d, error = %d\n",
           evie_major, evie_minor, opcode, evie_event, evie_error);
#endif

    if (!XFixesQueryExtension (mXDisplay, &xfixes_event, &xfixes_error)) {
	ERR_RET ("No XFixes extension\n");
    }
    SPEW ("xfixes extension: event = %d, error = %d\n",
           xfixes_event, xfixes_error);

    if (!XShapeQueryExtension (mXDisplay, &shape_event, &shape_error)) {
	ERR_RET ("No Shaped window extension\n");
    }
    SPEW ("shape extension: event = %d, error = %d\n",
           shape_event, shape_error);

#undef ERR_RET

    return true;
}


bool
compzillaControl::InitWindowState ()
{
    if (!InitManagerWindow ())
        return FALSE;

    XGrabServer (mXDisplay);

    long ev_mask = (SubstructureRedirectMask |
                    SubstructureNotifyMask |
                    StructureNotifyMask |
                    PropertyChangeMask);
    XSelectInput (mXDisplay, GDK_WINDOW_XID (mRoot), ev_mask);

    if (ClearErrors (mXDisplay)) {
        WARNING ("Another window manager is already running on screen: %d\n", 1);

        ev_mask &= ~(SubstructureRedirectMask);
        XSelectInput (mXDisplay, GDK_WINDOW_XID (mRoot), ev_mask);
    }

#if HAVE_XEVIE
    // Intercept mouse/keyboard events
    XevieStart (mXDisplay);
    XevieSelectInput (mXDisplay, (ButtonPressMask | 
                                  ButtonReleaseMask |
                                  KeyPressMask | 
                                  KeyReleaseMask));
#elif HAVE_XEVIE_WRITEONLY
    XevieStart (mXDisplay);
    //XevieSelectInput (mXDisplay, NoEventMask);
#endif

    if (mIsWindowManager) {
        // Start managing any existing windows
        Window root_notused, parent_notused;
        Window *children;
        unsigned int nchildren;

        XQueryTree (mXDisplay, 
                    GDK_WINDOW_XID (mRoot), 
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

    XUngrabServer (mXDisplay);

    return true;
}


bool
compzillaControl::InitManagerWindow ()
{
    SPEW ("InitManagerWindow\n");

    XSetWindowAttributes attrs;

    attrs.override_redirect = True;
    attrs.event_mask = PropertyChangeMask;
  
    // FIXME: Do this for each screen, and don't kill existing WMs.  See
    //        compiz's display.c:addDisplay.

    mManagerWindow = XCreateWindow (mXDisplay,
                                    GDK_WINDOW_XID (mRoot),
                                    -100, -100, 1, 1,
                                    0,
                                    CopyFromParent,
                                    CopyFromParent,
                                    (Visual *)CopyFromParent,
                                    CWOverrideRedirect | CWEventMask,
                                    &attrs);

    char *atom_name;
    Atom atom;

    atom_name = g_strdup_printf ("_NET_WM_CM_S%d", 0);
    atom = XInternAtom (mXDisplay, atom_name, FALSE);
    g_free (atom_name);

    XSetSelectionOwner (mXDisplay, atom, mManagerWindow, CurrentTime);
    mIsWindowManager = XGetSelectionOwner (mXDisplay, atom) == mManagerWindow;

    if (!mIsWindowManager) {
        ERROR ("Couldn't acquire window manager selection");
    }

    atom_name = g_strdup_printf ("WM_S%d", 0);
    atom = XInternAtom (mXDisplay, atom_name, FALSE);
    g_free (atom_name);

    XSetSelectionOwner (mXDisplay, atom, mManagerWindow, CurrentTime);
    mIsCompositor = XGetSelectionOwner (mXDisplay, atom) == mManagerWindow;
    
    if (!mIsCompositor) {
        ERROR ("Couldn't acquire compositor selection");
    }

    return TRUE;
}


bool
compzillaControl::InitOutputWindow ()
{
    // Create the overlay window, and make the X server stop drawing windows
    mOverlay = XCompositeGetOverlayWindow (mXDisplay, GDK_WINDOW_XID (mRoot));
    if (!mOverlay) {
        ERROR ("failed call to XCompositeGetOverlayWindow\n");
        return false;
    }

    mMainwin = GetNativeWindow (mDOMWindow);
    if (!mMainwin) {
        ERROR ("failed to get native window\n");
        return false;
    }
    gdk_window_ref (mMainwin);
    gdk_window_set_override_redirect(mMainwin, true);

    // Put the our window into the overlay
    XReparentWindow (mXDisplay, GDK_DRAWABLE_XID (mMainwin), mOverlay, 0, 0);

    // FIXME: This causes BadMatch errors, because we aren't mapped yet.
    //XSetInputFocus (mXDisplay, GDK_DRAWABLE_XID (mMainwin), RevertToPointerRoot, CurrentTime);

    ShowOutputWindow ();

    return true;
}


void 
compzillaControl::ShowOutputWindow()
{
    // NOTE: Ripped off from compiz.  Not sure why this is needed.  Maybe to
    //       support multiple monitors with different dimensions?

    // mMainwin isn't mapped yet
    /*
    XserverRegion region = XFixesCreateRegionFromWindow (mXDisplay,
                                                         GDK_DRAWABLE_XID (mMainwin),
                                                         WindowRegionBounding);
    */

#if HAVE_XEVIE
    XserverRegion empty = XFixesCreateRegion (mXDisplay, NULL, 0);

    XFixesSetWindowShapeRegion (mXDisplay,
                                mOverlay,
                                ShapeBounding,
                                0, 0, 
                                0);

    XFixesSetWindowShapeRegion (mXDisplay,
                                mOverlay,
                                ShapeInput,
                                0, 0, 
                                empty);

    XFixesDestroyRegion (mXDisplay, empty);
#else 
    XRectangle rect = { 0, 0, DisplayWidth (mXDisplay, 0), DisplayHeight (mXDisplay, 0) };
    XserverRegion region = XFixesCreateRegion (mXDisplay, &rect, 1);

    XFixesSetWindowShapeRegion (mXDisplay,
                                mOverlay,
                                ShapeBounding,
                                0, 0, 
                                region);

    XFixesSetWindowShapeRegion (mXDisplay,
                                mOverlay,
                                ShapeInput,
                                0, 0, 
                                region);

    XFixesDestroyRegion (mXDisplay, region);
#endif
}


void 
compzillaControl::HideOutputWindow()
{
    // NOTE: Ripped off from compiz.  Not sure why this is needed.  Maybe to
    //       support multiple monitors with different dimensions?

    XserverRegion region = XFixesCreateRegion (mXDisplay, NULL, 0);

    XFixesSetWindowShapeRegion (mXDisplay,
                                mOverlay,
                                ShapeBounding,
                                0, 0, 
                                region);

    XFixesDestroyRegion (mXDisplay, region);
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


void
compzillaControl::AddWindow (Window win)
{
    INFO ("AddWindow for window %p\n", win);

    nsAutoPtr<compzillaWindow> compwin(new compzillaWindow(mXDisplay, win));

    if (compwin->mAttr.c_class == InputOnly) {
        WARNING ("AddWindow ignoring InputOnly window %p\n", win);
        return;
    }

    nsCOMPtr<nsISupports> domContent;
    mWM->WindowCreated (win, 
                        compwin->mAttr.override_redirect != 0,
                        compwin->mAttr.x,
                        compwin->mAttr.y,
                        compwin->mAttr.width,
                        compwin->mAttr.height,
                        compwin->mAttr.map_state == IsViewable,
                        getter_AddRefs(domContent));

    INFO ("WindowCreated returned %p\n", domContent.get());

    if (domContent) {
        compwin->SetContent (domContent);
        mWindowMap.Put (win, compwin);

        if (compwin->mAttr.map_state == IsViewable) {
            MapWindow (win);
        }

        // Stored the compwin in mWindowMap, so don't delete it now.
        compwin.forget();
    }
}


void
compzillaControl::DestroyWindow (Window win)
{
    compzillaWindow *compwin = FindWindow (win);
    if (compwin) {
        if (compwin->mContent) {
            mWM->WindowDestroyed (compwin->mContent);
        }

        // Damage is not valid if the window is already destroyed
        compwin->mDamage = 0;
        mWindowMap.Remove (win);
    }
}


void
compzillaControl::ForgetWindow (Window win)
{
    compzillaWindow *compwin = FindWindow (win);
    if (compwin) {
        if (compwin->mContent) {
            mWM->WindowDestroyed (compwin->mContent);
        }

        mWindowMap.Remove (win);
    }
}


void
compzillaControl::MapWindow (Window win)
{
    compzillaWindow *compwin = FindWindow (win);
    if (compwin && compwin->mContent) {
        // Make sure we've got a handle to the content
        compwin->EnsurePixmap ();
        mWM->WindowMapped (compwin->mContent);
    }
}


void
compzillaControl::UnmapWindow (Window win)
{
    compzillaWindow *compwin = FindWindow (win);
    if (compwin && compwin->mContent) {
        mWM->WindowUnmapped (compwin->mContent);
    }
}


void
compzillaControl::PropertyChanged (Window win, Atom prop)
{
    compzillaWindow *compwin = FindWindow (win);
    if (compwin && compwin->mContent) {
        mWM->PropertyChanged (compwin->mContent, (PRUint32)prop);
    }
}


void
compzillaControl::WindowDamaged (Window win, XRectangle *rect)
{
    compzillaWindow *compwin = FindWindow (win);
    if (compwin && compwin->mContent) {        
        nsCOMPtr<nsIDOMHTMLCanvasElement> canvas = do_QueryInterface (compwin->mContent);
        nsCOMPtr<compzillaIRenderingContextInternal> internal;
        nsresult rv = canvas->GetContext (NS_LITERAL_STRING ("compzilla"), 
                                          getter_AddRefs (internal));

        if (NS_SUCCEEDED (rv)) {
            compwin->EnsurePixmap ();
            internal->SetDrawable (mXDisplay, compwin->mPixmap, compwin->mAttr.visual);
            internal->Redraw (nsRect (rect->x, rect->y, rect->width, rect->height));
        }
    }
}


compzillaWindow *
compzillaControl::FindWindow (Window win)
{
    compzillaWindow *compwin;
    mWindowMap.Get(win, &compwin);
    return compwin;
}


GdkFilterReturn
compzillaControl::Filter (GdkXEvent *xevent, GdkEvent *event)
{
    XEvent *x11_event = (XEvent*) xevent;

    switch (x11_event->type) {
    case CreateNotify: {
        SPEW ("CreateNotify: window=0x%0x, x=%d, y=%d, width=%d, height=%d, override=%d\n",
               x11_event->xcreatewindow.window,
               x11_event->xcreatewindow.x,
               x11_event->xcreatewindow.y,
               x11_event->xcreatewindow.width,
               x11_event->xcreatewindow.height,
               x11_event->xcreatewindow.override_redirect);

        if (x11_event->xcreatewindow.window == GDK_DRAWABLE_XID (this->mMainwin)) {
            WARNING ("CreateNotify: discarding event on mainwin\n");
            return GDK_FILTER_REMOVE;
        }

        AddWindow(x11_event->xcreatewindow.window);
        break;
    }
    case DestroyNotify: {
        SPEW ("DestroyNotify: window=0x%0x\n", x11_event->xdestroywindow.window);

        DestroyWindow (x11_event->xdestroywindow.window);
        break;
    }
    case ConfigureNotify: {
        break;
    }
    case ConfigureRequest: {
        SPEW ("ConfigureRequest: window=%p, x=%d, y=%d, width=%d, height=%d, border=%d, override=%d\n",
              x11_event->xconfigure.window,
              x11_event->xconfigure.x,
              x11_event->xconfigure.y,
              x11_event->xconfigure.width,
              x11_event->xconfigure.height,
              x11_event->xconfigure.border_width,
              x11_event->xconfigure.override_redirect);

        compzillaWindow *compwin = FindWindow (x11_event->xconfigure.window);

        if (compwin && compwin->mContent) {
            compzillaWindow *abovewin = FindWindow (x11_event->xconfigure.above);

            mWM->WindowConfigured (compwin->mContent,
                                   x11_event->xconfigure.x,
                                   x11_event->xconfigure.y,
                                   x11_event->xconfigure.width,
                                   x11_event->xconfigure.height,
                                   x11_event->xconfigure.border_width,
                                   abovewin ? abovewin->mContent : NULL);
        }

        break;
    }
    case ReparentNotify: {
        SPEW ("ReparentNotify: window=%p, parent=%p, x=%d, y=%d, override_redirect=%d\n",
              x11_event->xreparent.window,
              x11_event->xreparent.parent,
              x11_event->xreparent.x,
              x11_event->xreparent.y,
              x11_event->xreparent.override_redirect);

        if (x11_event->xreparent.window == GDK_DRAWABLE_XID (this->mMainwin)) {
            // Keep the new parent so we can ignore events on it.
            mMainwinParent = x11_event->xreparent.parent;

            // Let the mainwin draw normally by unredirecting our new parent.
            ForgetWindow (x11_event->xreparent.parent);

            WARNING ("ReparentNotify: discarding event on mainwin\n");
            return GDK_FILTER_REMOVE;
        }

        if (x11_event->xreparent.window == mMainwinParent) {
            WARNING ("ReparentNotify: discarding event on mainwin's parent\n");
            return GDK_FILTER_REMOVE;
        }

        compzillaWindow *compwin = FindWindow (x11_event->xreparent.window);

        if (x11_event->xreparent.parent == GDK_DRAWABLE_XID (mRoot) && !compwin) {
            AddWindow (x11_event->xreparent.window);
        } else if (compwin) {
            DestroyWindow (x11_event->xreparent.window);
        }

        break;
    }
    case MapRequest:
        XMapRaised (mXDisplay, x11_event->xmaprequest.window);
        break;
    case MapNotify: {
        SPEW ("MapNotify: window=0x%0x, override=%d\n", x11_event->xmap.window, 
              x11_event->xmap.override_redirect);

        MapWindow (x11_event->xmap.window);
        break;
    }
    case UnmapNotify: {
        SPEW ("UnmapNotify: window=0x%0x\n", x11_event->xunmap.window);

        UnmapWindow (x11_event->xunmap.window);
        break;
    }
    case PropertyNotify: {
        SPEW ("PropertyChange: window=0x%0x, atom=%s\n", 
              x11_event->xproperty.window, 
              XGetAtomName(x11_event->xany.display, x11_event->xproperty.atom));
        PropertyChanged (x11_event->xproperty.window, x11_event->xproperty.atom);
        break;
    }
    case _KeyPress:
    case KeyRelease:
    case ButtonPress:
    case ButtonRelease:
    case MotionNotify: {
        switch (x11_event->type) {
        case _KeyPress:
            SPEW ("KeyPress: window=0x%0x, state=%d, keycode=%d\n", 
                  x11_event->xkey.window, x11_event->xkey.state, x11_event->xkey.keycode);
            break;
        case KeyRelease:
            SPEW ("KeyRelease: window=0x%0x, state=%d, keycode=%d\n", 
                  x11_event->xkey.window, x11_event->xkey.state, x11_event->xkey.keycode);
            break;
        case ButtonPress:
            SPEW ("ButtonPress: window=%p, x=%d, y=%d, x_root=%d, y_root=%d, button=%d\n", 
                  x11_event->xbutton.window, x11_event->xbutton.x, x11_event->xbutton.y,
                  x11_event->xbutton.x_root, x11_event->xbutton.y_root,
                  x11_event->xbutton.button);
            break;
        case ButtonRelease:
            SPEW ("ButtonRelease: window=0x%0x, x=%d, y=%d, button=%d\n", 
                  x11_event->xbutton.window, x11_event->xbutton.x, x11_event->xbutton.y,
                  x11_event->xbutton.button);
            break;
        case MotionNotify:
            SPEW ("MotionNotify: window=0x%0x, x=%d, y=%d, state=%d\n", 
                  x11_event->xmotion.window, x11_event->xmotion.x, x11_event->xmotion.y, 
                  x11_event->xmotion.state);
            break;
        }

#if HAVE_XEVIE
        // Redirect all mouse/kayboard events to our mozilla window.
        /*
        Window mainwin = GDK_WINDOW_XID (mMainwin);
        if (x11_event->xany.window != mainwin) {
            x11_event->xany.window = mainwin;
            XevieSendEvent (mXDisplay, x11_event, XEVIE_MODIFIED);
        } else {
            XevieSendEvent (mXDisplay, x11_event, XEVIE_UNMODIFIED);
        }
        */
        XevieSendEvent (mXDisplay, x11_event, XEVIE_UNMODIFIED);

        nsCOMPtr<nsIWidget> widget = GetNativeWidget(mDOMWindow);
        if (widget) {
            // Call nsIWidget::DispatchEvent?
        }

        // Don't let Gtk process the original event.
        return GDK_FILTER_REMOVE;
#endif

        break;
    }
    case Expose:
        ERROR ("Expose event win=%p\n", x11_event->xexpose.window);
        break;
    default:
        if (x11_event->type == damage_event + XDamageNotify) {
            XDamageNotifyEvent *damage_ev = (XDamageNotifyEvent *) x11_event;

#if 0
            SPEW ("DAMAGE: drawable=%p, x=%d, y=%d, width=%d, height=%d\n", 
                  damage_ev->drawable, damage_ev->area.x, damage_ev->area.y, 
                  damage_ev->area.width, damage_ev->area.height);
#endif

            WindowDamaged (damage_ev->drawable, &damage_ev->area);

            return GDK_FILTER_REMOVE;
        }
        else if (x11_event->type == shape_event + ShapeNotify) {
            NS_NOTYETIMPLEMENTED ("ShapeNotify event");
        }
        else {
            ERROR ("Unhandled window event %d\n", x11_event->type);
        }
        break;
    }

    return GDK_FILTER_CONTINUE;
}
