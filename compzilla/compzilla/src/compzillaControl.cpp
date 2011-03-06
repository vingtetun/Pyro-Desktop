/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#include <nspr.h>
#include <jsapi.h>             // unstable
#include <nsMemory.h>
#include <nsHashPropertyBag.h> // unstable
#include <nsIBaseWindow.h>     // unstable
#include <nsIDocShell.h>       // unstable
#include <nsIDOMClassInfo.h>   // unstable
#include <nsIInterfaceRequestorUtils.h>
#include <nsIWebNavigation.h>  // unstable

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


NS_IMPL_CLASSINFO(compzillaControl, NULL, 0, COMPZILLA_CONTROL_CID)
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


compzillaControl::compzillaControl() {
    if (!compzillaLog) {
        compzillaLog = PR_NewLogModule ("compzilla");
    }

    mXDisplay = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
    mXRoot = GDK_DRAWABLE_XID(gdk_get_default_root_window());

    // XXX Actually it looks like the code does not handle more than 50 windows
    // and even it it looks a lot I do think this worth a pref, for example you
    // imagine a system where you want to force having only one window at a
    // time or where you don't want to have too many windows open
    mWindowMap.Init(50);
}


compzillaControl::~compzillaControl() {
}


/* ========================================================================= *\
 * compzillaIControl Implementation...                                       *
\* ========================================================================= */

NS_IMETHODIMP
compzillaControl::HasWindowManager(nsIDOMWindow *window, PRBool *retval) {
  // FIXME: Handle screens
  char *atom_name = g_strdup_printf("WM_S%d", 0);
  Atom atom = XInternAtom(mXDisplay, atom_name, FALSE);
 g_free(atom_name);

  *retval = (XGetSelectionOwner(mXDisplay, atom) != None);

  SPEW("HasWindowManager == %s\n", *retval ? "TRUE" : "FALSE");
  return NS_OK;
}


NS_IMETHODIMP
compzillaControl::RegisterWindowManager(nsIDOMWindow *window) {
  nsresult rv = NS_OK;

  SPEW("RegisterWindowManager: mDOMWindow=%p\n", window);
  mDOMWindow = window;

#ifdef CAUTIOUS_FILTER
  // Get ALL events for root window
  gdk_window_add_filter(gdk_get_default_root_window(), gdk_filter_func, this);
#else
  // Get ALL events for ALL windows
  gdk_window_add_filter(NULL, gdk_filter_func, this);
#endif

  // Just ignore errors for now
  XSetErrorHandler(ErrorHandler);

  // Try to register as window manager
  rv = InitManagerWindow();
  if (NS_FAILED(rv))
      return rv;

  rv = InitXAtoms();
  if (NS_FAILED(rv))
      return rv;

  // Check extension versions
  rv = InitXExtensions();
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
compzillaControl::InternAtom(const char *property, PRUint32 *value) {
  *value = (PRUint32) XInternAtom(mXDisplay, property, FALSE);
  return NS_OK;
}


NS_IMETHODIMP
compzillaControl::SetRootWindowProperty (PRInt32 prop, PRInt32 type,
                                         PRUint32 count,
                                         PRUint32* valueArray) {

  // XXX What is 32? Is there a constant somewhere for that?
  XChangeProperty (mXDisplay, mXRoot, prop, type, 32,
                   PropModeReplace, (unsigned char*)valueArray, count);

  return NS_OK;
}


NS_IMETHODIMP
compzillaControl::DeleteRootWindowProperty(PRInt32 prop) {
  XDeleteProperty (mXDisplay, mXRoot, prop);
  return NS_OK;
}


NS_IMETHODIMP
compzillaControl::SendConfigureNotify (PRUint32 xid,
                                       PRUint32 x, PRUint32 y,
                                       PRUint32 width, PRUint32 height,
                                       PRUint32 border,
                                       PRBool overrideRedirect) {
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

  XSendEvent(mXDisplay, xid, False, StructureNotifyMask, &ev);
}


NS_IMETHODIMP
compzillaControl::Kill(PRUint32 xid) {
  Unmap (xid);
  XDestroyWindow (mXDisplay, xid);
}


NS_IMETHODIMP
compzillaControl::MoveToTop(PRUint32 xid) {
  XWindowChanges changes;

  changes.sibling = None;
  changes.stack_mode = Above;

  XConfigureWindow (mXDisplay, xid, CWStackMode, &changes);
}


NS_IMETHODIMP
compzillaControl::MoveToBottom(PRUint32 xid) {
  XWindowChanges changes;

  changes.sibling = None;
  changes.stack_mode = Below;

  XConfigureWindow (mXDisplay, xid, CWStackMode, &changes);
}


NS_IMETHODIMP
compzillaControl::Configure (PRUint32 xid,
                             PRUint32 x, PRUint32 y,
                             PRUint32 width, PRUint32 height,
                             PRUint32 border) {
  nsRefPtr<compzillaWindow> win = FindWindow (xid);
  if (win) {
    win->QueueResize (x, y, width, height, border);
    return NS_OK;
  }

  XWindowChanges changes;
  changes.x = x;
  changes.y = y;
  changes.width = width;
  changes.height = height;
  changes.border_width = border;

  SPEW ("Configure: Calling XConfigureWindow (window=%p, x=%d, y=%d, "
        "width=%d, height=%d, border=%d)\n", xid, x, y, width, height, border);

  XConfigureWindow (mXDisplay, xid,
                    (CWX | CWY | CWWidth | CWHeight | CWBorderWidth), &changes);
}


NS_IMETHODIMP
compzillaControl::Map(PRUint32 xid) {
  XMapWindow (mXDisplay, xid);
}


NS_IMETHODIMP
compzillaControl::Unmap(PRUint32 xid) {
  XUnmapWindow (mXDisplay, xid);
}


NS_IMETHODIMP
compzillaControl::AddObserver(compzillaIControlObserver *aObserver) {
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
compzillaControl::RemoveObserver (compzillaIControlObserver *aObserver) {
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


/* ========================================================================= *\
 * Private methods...                                                        *
\* ========================================================================= */

GdkFilterReturn
compzillaControl::gdk_filter_func(GdkXEvent *xevent,
                                  GdkEvent *event, gpointer data) {
  compzillaControl *control = reinterpret_cast<compzillaControl*>(data);
  return control->Filter (xevent, event);
}


nsresult
compzillaControl::GetNativeWidget(nsIDOMWindow *window, nsIWidget **widget) {
  nsCOMPtr<nsIWebNavigation> webNav = do_GetInterface (window);
  if (!webNav)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIBaseWindow> baseWin = do_QueryInterface (webNav);
  if (!baseWin)
    return NS_ERROR_FAILURE;

  baseWin->GetMainWidget (widget);
  return NS_OK;
}


GdkWindow *
compzillaControl::GetNativeWindow(nsIDOMWindow *window) {
  nsCOMPtr<nsIWidget> widget;

  if (NS_OK == GetNativeWidget(window, getter_AddRefs(widget)))
    return (GdkWindow *)widget->GetNativeData (NS_NATIVE_WINDOW);

  WARNING ("Could not get GdkWindow for nsIDOMWindow %p\n", window);
  return NULL;
}


nsresult
compzillaControl::InitXAtoms () {
  if (!XInternAtoms (mXDisplay,
                     atom_names, sizeof (atom_names) / sizeof (atom_names[0]),
                     False,
                     atoms.a)) {
      return NS_ERROR_FAILURE;
  }
  return NS_OK;
}


nsresult
compzillaControl::InitXExtensions () {
  int opcode;
  if (!XQueryExtension(mXDisplay, COMPOSITE_NAME, &opcode,
                       &composite_event, &composite_error)) {
    ERROR("No composite extension\n");
    return NS_ERROR_NO_COMPOSITE_EXTENSTION;
  }

  int composite_major, composite_minor;
  XCompositeQueryVersion(mXDisplay, &composite_major, &composite_minor);
  if (composite_major == 0 && composite_minor < 2) {
    ERROR("Old composite extension does not support XCompositeGetOverlayWindow\n");
    return NS_ERROR_NO_COMPOSITE_EXTENSTION;
  }

  SPEW ("composite extension: major = %d, minor = %d, opcode = %d, event = %d, error = %d\n",
         composite_major, composite_minor, opcode, composite_event, composite_error);

  if (!XDamageQueryExtension(mXDisplay, &damage_event, &damage_error)) {
	  ERROR("No damage extension\n");
    return NS_ERROR_NO_DAMAGE_EXTENSTION;
  }

  SPEW ("damage extension: event = %d, error = %d\n", damage_event, damage_error);

  if (!XFixesQueryExtension (mXDisplay, &xfixes_event, &xfixes_error)) {
	  ERROR("No XFixes extension\n");
    return NS_ERROR_NO_XFIXES_EXTENSTION;
  }

  SPEW ("xfixes extension: event = %d, error = %d\n", xfixes_event, xfixes_error);

#if HAVE_XSHAPE
  if (!XShapeQueryExtension (mXDisplay, &shape_event, &shape_error)) {
	  ERROR("No Shaped window extension\n");
    return NS_ERROR_NO_SHAPE_EXTENSTION;
  }

  SPEW ("shape extension: event = %d, error = %d\n", shape_event, shape_error);
#endif

  return NS_OK;
}


nsresult
compzillaControl::InitWindowState() {
  XGrabServer (mXDisplay);

  long ev_mask = (SubstructureRedirectMask |
                  SubstructureNotifyMask |
                  StructureNotifyMask |
                  ResizeRedirectMask |
                  PropertyChangeMask |
                  FocusChangeMask);
  XSelectInput (mXDisplay, mXRoot, ev_mask);

  if (ClearErrors(mXDisplay)) {
    WARNING ("Another window manager is already running on screen: %d\n", 1);

    ev_mask &= ~(SubstructureRedirectMask);
    XSelectInput (mXDisplay, mXRoot, ev_mask);
  }

  if (mIsWindowManager) {
    // Start managing any existing windows
    Window root_notused, parent_notused;
    Window *children;
    unsigned int nchildren;

    XQueryTree (mXDisplay, mXRoot, &root_notused,
                &parent_notused, &children, &nchildren);

    for (int i = 0; i < nchildren; i++) {
      if (children[i] != mOverlay && children[i] != mMainwinParent) {
        AddWindow (children[i]);
      }
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
compzillaControl::ReplaceSelectionOwner(Window newOwner, Atom atom) {
  Window currentOwner = XGetSelectionOwner(mXDisplay, atom);
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
  // XXX What is 32 again?
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
compzillaControl::InitManagerWindow () {
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
compzillaControl::InitOverlay () {
  // Create the overlay window, and make the X server stop drawing windows
  mOverlay = XCompositeGetOverlayWindow (mXDisplay, mXRoot);
  if (!mOverlay) {
    ERROR ("failed call to XCompositeGetOverlayWindow\n");
    return NS_ERROR_FAILURE;
  }

  GdkWindow *mainwin = GetNativeWindow (mDOMWindow);
  if (!mainwin) {
    ERROR ("failed to get native window\n");
    return NS_ERROR_FAILURE;
  }

  mMainwin = GDK_DRAWABLE_XID (mainwin);

  gdk_window_set_override_redirect(mainwin, true);

  GdkWindow *mainwinParent = gdk_window_get_toplevel (mainwin);
  mMainwinParent = GDK_DRAWABLE_XID (mainwinParent);

  SPEW("InitOverlay: toplevel=0x%0x, iframe=0x%0x\n", mMainwinParent, mMainwin);

  // Put the our window into the overlay
  XReparentWindow (mXDisplay, mMainwinParent, mOverlay, 0, 0);

  ShowOverlay (true);
  EnableOverlayInput (true);

  return NS_OK;
}


void
compzillaControl::ShowOverlay (bool show) {
  XserverRegion xregion;

  if (show) {
    XRectangle rect = { 0, 0,
                        DisplayWidth (mXDisplay, 0),
                        DisplayHeight (mXDisplay, 0) };
    xregion = XFixesCreateRegion (mXDisplay, &rect, 1);
  } else {
    xregion = XFixesCreateRegion (mXDisplay, NULL, 0);
  }

  XFixesSetWindowShapeRegion(mXDisplay,
                             mOverlay,
                             ShapeBounding,
                             0, 0,
                             xregion);

  XFixesDestroyRegion (mXDisplay, xregion);
}


void
compzillaControl::EnableOverlayInput (bool receiveInput) {
  XserverRegion xregion;

  if (receiveInput) {
    XRectangle rect = { 0, 0,
                        DisplayWidth (mXDisplay, 0),
                        DisplayHeight (mXDisplay, 0) };
    xregion = XFixesCreateRegion (mXDisplay, &rect, 1);
  } else {
    xregion = XFixesCreateRegion (mXDisplay, NULL, 0);
  }

  XFixesSetWindowShapeRegion(mXDisplay,
                             mOverlay,
                             ShapeInput,
                             0, 0,
                             xregion);

  XFixesDestroyRegion (mXDisplay, xregion);
}


int
compzillaControl::ErrorHandler (Display *dpy, XErrorEvent *err) {
  sErrorCnt++;

  char str[128];
  XGetErrorDatabaseText (dpy, "XlibMessage", "XError", "", str, 128);
  ERROR ("%s", str);

  char *name = 0;
  int o = err->error_code - damage_error;
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
compzillaControl::ClearErrors (Display *dpy) {
  XSync (dpy, FALSE);

  int lastcnt = sErrorCnt;
  sErrorCnt = 0;
  return lastcnt;
}


PLDHashOperator
compzillaControl::CallWindowCreateCb (const PRUint32& key,
                                      nsRefPtr<compzillaWindow>& win,
                                      void *userdata) {
  compzillaIControlObserver *observer =
    static_cast<compzillaIControlObserver *>(userdata);
  compzillaIWindow *iwin = win;
  observer->WindowCreate (iwin);
}


void
compzillaControl::AddWindow (Window win) {
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

#ifdef CAUTIOUS_FILTER
  // Handle events for this window
  gdk_window_add_filter (gdk_window_foreign_new(win), gdk_filter_func, this);
#endif

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
compzillaControl::DestroyWindow (nsRefPtr<compzillaWindow> win,
                                 Window xwin) {
  if (win) {
    compzillaIWindow *iwin = win;
    for (PRUint32 i = mObservers.Count() - 1; i != PRUint32(-1); --i) {
      nsCOMPtr<compzillaIControlObserver> observer = mObservers.ObjectAt(i);
      observer->WindowDestroy (iwin);
    }

    win->Destroyed ();
  }

  mWindowMap.Remove (xwin);
}


void
compzillaControl::RootClientMessaged (Atom type, int format, long *data/*[5]*/) {
  for (PRUint32 i = mObservers.Count() - 1; i != PRUint32(-1); --i) {
    nsCOMPtr<compzillaIControlObserver> observer = mObservers.ObjectAt(i);
    observer->RootClientMessageRecv ((long) type,
                                     format,
                                     data[0],
                                     data[1],
                                     data[2],
                                     data[3],
                                     data[4]);
  }
}


already_AddRefed<compzillaWindow>
compzillaControl::FindWindow (Window win) {
  compzillaWindow *compwin;
  mWindowMap.Get(win, &compwin);
  return compwin;
}


void
compzillaControl::PrintEvent (XEvent *xev) {
  switch (xev->type) {
  case ClientMessage:
    SPEW("ClientMessage: window=0x%0x, type=%s, format=%d\n",
         xev->xclient.window,
         XGetAtomName (mXDisplay, xev->xclient.message_type),
         xev->xclient.format);
    break;

    case CreateNotify:
      SPEW("CreateNotify: window=0x%0x, parent=0x%0x, x=%d, y=%d, width=%d, height=%d, "
           "override=%d\n",
           xev->xcreatewindow.window,
           xev->xcreatewindow.parent,
           xev->xcreatewindow.x,
           xev->xcreatewindow.y,
           xev->xcreatewindow.width,
           xev->xcreatewindow.height,
           xev->xcreatewindow.override_redirect);
      break;

    case DestroyNotify:
      SPEW("DestroyNotify: event_win=0x%0x, window=0x%0x\n",
           xev->xdestroywindow.event,
           xev->xdestroywindow.window);
      break;

    case ConfigureNotify:
      SPEW("ConfigureNotify: window=0x%0x, x=%d, y=%d, width=%d, height=%d, "
           "border=%d, above=0x%0x, override=%d\n",
           xev->xconfigure.window,
           xev->xconfigure.x,
           xev->xconfigure.y,
           xev->xconfigure.width,
           xev->xconfigure.height,
           xev->xconfigure.border_width,
           xev->xconfigure.above,
           xev->xconfigure.override_redirect);
        break;

    case ConfigureRequest:
      SPEW("ConfigureRequest: window=0x%0x, parent=0x%0x, x=%d, y=%d, width=%d, height=%d, "
           "border=%d, above=0x%0x\n",
           xev->xconfigurerequest.window,
           xev->xconfigurerequest.parent,
           xev->xconfigurerequest.x,
           xev->xconfigurerequest.y,
           xev->xconfigurerequest.width,
           xev->xconfigurerequest.height,
           xev->xconfigurerequest.border_width,
           xev->xconfigurerequest.above);
        break;

    case ReparentNotify:
      SPEW("ReparentNotify: window=0x%0x, parent=0x%0x, x=%d, y=%d, override_redirect=%d\n",
           xev->xreparent.window,
           xev->xreparent.parent,
           xev->xreparent.x,
           xev->xreparent.y,
           xev->xreparent.override_redirect);
        break;

    case MapRequest:
      SPEW("MapRequest: window=0x%0x, parent=0x%0x\n",
            xev->xmaprequest.window,
            xev->xmaprequest.parent);
        break;

    case MapNotify:
      SPEW("MapNotify: eventwin=0x%0x, window=0x%0x, override=%d\n",
           xev->xmap.event,
           xev->xmap.window,
           xev->xmap.override_redirect);
      break;

    case UnmapNotify:
      SPEW("UnmapNotify: eventwin=0x%0x, window=0x%0x, from_configure=%d\n",
           xev->xunmap.event,
           xev->xunmap.window,
           xev->xunmap.from_configure);
      break;

    case PropertyNotify:
#if DEBUG_EVENTS // Too much noise
      SPEW("PropertyChange: window=0x%0x, atom=%s\n", xev->xproperty.window,
           XGetAtomName(xev->xany.display, xev->xproperty.atom));
#endif
      break;

    case _KeyPress:
    case KeyRelease:
      SPEW("%s: window=0x%0x, subwindow=0x%0x, x=%d, y=%d, state=%d, keycode=%d\n",
           xev->xkey.type == _KeyPress ? "KeyPress" : "KeyRelease",
           xev->xkey.window,
           xev->xkey.subwindow,
           xev->xkey.state, xev->xkey.keycode);
      break;

    case ButtonPress:
    case ButtonRelease:
      SPEW("%s: window=0x%0x, subwindow=0x%0x, x=%d, y=%d, x_root=%d, y_root=%d, "
           "state=%d, button=%d\n",
           xev->xbutton.type == ButtonPress ? "ButtonPress" : "ButtonRelease",
           xev->xbutton.window,
           xev->xbutton.subwindow,
           xev->xbutton.x,
           xev->xbutton.y,
           xev->xbutton.x_root,
           xev->xbutton.y_root,
           xev->xbutton.state,
           xev->xbutton.button);
      break;

    case MotionNotify:
#if DEBUG_EVENTS // Too much noise
      SPEW("MotionNotify: window=0x%0x, x=%d, y=%d, state=%d\n",
           xev->xmotion.window, xev->xmotion.x, xev->xmotion.y,
           xev->xmotion.state);
#endif
      break;

    case _FocusIn:
    case _FocusOut:
      SPEW("%s: %s window=0x%0x, mode=%d, detail=%d\n",
           xev->xfocus.type == _FocusIn ? "FocusIn" : "FocusOut",
           xev->xfocus.send_event ? "(SYNTHETIC)" : "REAL",
           xev->xfocus.window,
           xev->xfocus.mode,
           xev->xfocus.detail);
      break;

    case EnterNotify:
    case LeaveNotify:
      SPEW("%s: %s window=0x%0x, subwindow=0x%0x, mode=%d, detail=%d, focus=%s, "
           "state=%d\n",
           xev->xcrossing.type == EnterNotify ? "EnterNotify" : "LeaveNotify",
           xev->xcrossing.send_event ? "(SYNTHETIC)" : "REAL",
           xev->xcrossing.window,
           xev->xcrossing.subwindow,
           xev->xcrossing.mode,
           xev->xcrossing.detail,
           xev->xcrossing.focus ? "TRUE" : "FALSE",
           xev->xcrossing.state);
      break;

    case Expose:
      SPEW("Expose: window=0x%0x, count=%d\n",
           xev->xexpose.window,
           xev->xexpose.count);
      break;

    case VisibilityNotify:
      SPEW("VisibilityNotify: window=0x%0x, state=%d\n",
           xev->xvisibility.window,
           xev->xvisibility.state);
      break;

    default:
      if (xev->type == damage_event + XDamageNotify) {
        XDamageNotifyEvent *damage_ev = (XDamageNotifyEvent *) xev;
        SPEW_EVENT("DAMAGE: drawable=0x%0x, x=%d, y=%d, width=%d, height=%d\n",
                   damage_ev->drawable, damage_ev->area.x, damage_ev->area.y,
                   damage_ev->area.width, damage_ev->area.height);
      } else if (xev->type == xfixes_event + XFixesCursorNotify) {
        XFixesCursorNotifyEvent *cursor_ev = (XFixesCursorNotifyEvent *) xev;
        const char *cursor_val = gdk_x11_get_xatom_name (cursor_ev->cursor_name);

        SPEW("CursorNotify: window=0x%0x, cursor='%s'\n",
             cursor_ev->window,
             cursor_val);
      } else if (xev->type == shape_event + ShapeNotify) {
        XShapeEvent *shape_ev = (XShapeEvent *) xev;

        SPEW("ShapeNotify: window=0x%0x, kind=%s, shaped=%s, x=%d, y=%d, "
             "width=%d, height=%d\n",
             shape_ev->window,
             shape_ev->kind == ShapeBounding ? "bounding" : "clip",
             shape_ev->shaped ? "TRUE" : "FALSE",
             shape_ev->x, shape_ev->y, shape_ev->width, shape_ev->height);
      } else {
        ERROR ("Unhandled window event %d on 0x%0x\n", xev->type, xev->xany.window);
      }
      break;
  }
}


Window
compzillaControl::GetEventXWindow (XEvent *xev) {
  switch (xev->type) {
    case ClientMessage:
      return xev->xclient.window;
    case CreateNotify:
      return xev->xcreatewindow.window;
    case DestroyNotify:
      return xev->xdestroywindow.window;
    case ConfigureNotify:
      return xev->xconfigure.window;
    case ConfigureRequest:
      return xev->xconfigurerequest.window;
    case ReparentNotify:
      return xev->xreparent.window;
    case MapRequest:
      return xev->xmaprequest.window;
    case MapNotify:
      return xev->xmap.window;
    case UnmapNotify:
      return xev->xunmap.window;
    case PropertyNotify:
      return xev->xproperty.window;
    case _FocusIn:
    case _FocusOut:
      return xev->xfocus.window;
    default:
      if (xev->type == damage_event + XDamageNotify) {
        XDamageNotifyEvent *damage_ev = (XDamageNotifyEvent *) xev;
        return damage_ev->drawable;
      } else if (xev->type == xfixes_event + XFixesCursorNotify) {
        XFixesCursorNotifyEvent *cursor_ev = (XFixesCursorNotifyEvent *) xev;
        return cursor_ev->window;
      } else if (xev->type == shape_event + ShapeNotify) {
        XShapeEvent *shape_ev = (XShapeEvent *) xev;
        return shape_ev->window;
      }
  }
  return None;
}


GdkFilterReturn
compzillaControl::Filter (GdkXEvent *xevent, GdkEvent *event) {
  XEvent *xev = (XEvent*) xevent;

  if (xev->type == Expose || xev->type == VisibilityNotify) {
    return GDK_FILTER_CONTINUE;
  }

  PrintEvent (xev);

  Window xwin = GetEventXWindow (xev);
  nsRefPtr<compzillaWindow> win = FindWindow (xwin);

  switch (xev->type) {
    case ClientMessage:
      if (xwin == mXRoot) {
        RootClientMessaged (xev->xclient.message_type,
                            xev->xclient.format,
                            xev->xclient.data.l);
        return GDK_FILTER_REMOVE;
      } else if (win) {
        win->ClientMessaged (xev->xclient.message_type,
                             xev->xclient.format,
                             xev->xclient.data.l);
        return GDK_FILTER_REMOVE;
      }
      break;

    case CreateNotify:
      if (xwin == mMainwin) {
        WARNING ("CreateNotify: discarding event on mainwin\n");
        return GDK_FILTER_REMOVE;
      }

      if (xev->xcreatewindow.parent == mXRoot) {
        if (win) {
          ERROR ("CreateNotify: multiple create events for window id: 0x%0x\n", xwin);
        } else if (!XCheckTypedWindowEvent (mXDisplay, xwin, DestroyNotify, xev) &&
          !XCheckTypedWindowEvent (mXDisplay, xwin, ReparentNotify, xev)) {
          AddWindow (xwin);
        }
        return GDK_FILTER_REMOVE;
      }
      break;

    case DestroyNotify:
      if (win) {
        DestroyWindow (win, xwin);
        return GDK_FILTER_REMOVE;
      }
      break;

    case ConfigureNotify:
      if (win) {
#if CLEAR_PENDING_X_EVENTS
        while (XCheckTypedWindowEvent (mXDisplay, xwin, ConfigureNotify, xev)) {
          // Do nothing
        }
#endif

        // This is driven by compzilla or from an override_redirect itself.
        win->Configured (true,
                         xev->xconfigure.x,
                         xev->xconfigure.y,
                         xev->xconfigure.width,
                         xev->xconfigure.height,
                         xev->xconfigure.border_width,
                         NULL,
                         xev->xconfigure.override_redirect);
        return GDK_FILTER_REMOVE;
      }
      break;

    case ConfigureRequest:
      if (xev->xconfigurerequest.parent == mXRoot) {
          if (win) {
            while (XCheckTypedWindowEvent (mXDisplay, xwin, ConfigureRequest, xev)) {
              // Do nothing
            }

            nsRefPtr<compzillaWindow> aboveWin;
            if (xev->xconfigure.above != None)
              aboveWin = FindWindow (xev->xconfigure.above);

              // This is driven by the X app, not compzilla.
              win->Configured (false,
                               xev->xconfigurerequest.x,
                               xev->xconfigurerequest.y,
                               xev->xconfigurerequest.width,
                               xev->xconfigurerequest.height,
                               xev->xconfigurerequest.border_width,
                               aboveWin,
                               false);
          } else {
            // Window we are not monitoring, so allow the configure.
            Configure (xwin,
                       xev->xconfigurerequest.x,
                       xev->xconfigurerequest.y,
                       xev->xconfigurerequest.width,
                       xev->xconfigurerequest.height,
                       xev->xconfigurerequest.border_width);
          }
          return GDK_FILTER_REMOVE;
      }
      break;

    case ReparentNotify:
      if (xwin == mMainwin) {
        // Keep the new parent so we can ignore events on it.
        mMainwinParent = xev->xreparent.parent;

        nsRefPtr<compzillaWindow> parent = FindWindow (xev->xreparent.parent);
        if (parent) {
            // Let the mainwin draw normally by unredirecting our new parent.
            DestroyWindow (parent, xev->xreparent.parent);
        }

        WARNING ("ReparentNotify: discarding event on mainwin\n");
        return GDK_FILTER_REMOVE;
      }

      if (xwin == mMainwinParent) {
        WARNING ("ReparentNotify: discarding event on mainwin's parent\n");
        return GDK_FILTER_REMOVE;
      }

      if (xev->xreparent.parent == mXRoot) {
        if (win) {
          ERROR ("Reparent of existing toplevel window!\n");
        } else {
          AddWindow (xwin);
        }
      } else if (win) {
        /*
         * This is the only case where a window is removed but not
         * destroyed. We must remove our event mask and all passive
         * grabs.  -- compiz
         */
          XSelectInput (mXDisplay, xwin, NoEventMask);
#if HAVE_XSHAPE
          XShapeSelectInput (mXDisplay, xwin, NoEventMask);
#endif
          XUngrabButton (mXDisplay, AnyButton, AnyModifier, xwin);

          DestroyWindow (win, xwin);
      }

      return GDK_FILTER_REMOVE;

    case MapRequest:
      if (xev->xmaprequest.parent == mXRoot) {
        if (!XCheckTypedWindowEvent (mXDisplay, xwin, UnmapNotify, xev)) {
          XMapWindow (mXDisplay, xwin);
        }
        return GDK_FILTER_REMOVE;
      }
      break;

    case MapNotify:
      if (win) {
        if (!XCheckTypedWindowEvent (mXDisplay, xwin, UnmapNotify, xev)) {
          win->Mapped (xev->xmap.override_redirect);
        }
        return GDK_FILTER_REMOVE;
      }
      break;

    case UnmapNotify:
      if (win) {
        if (!XCheckTypedWindowEvent (mXDisplay, xwin, MapNotify, xev)) {
          win->Unmapped ();
        }
        return GDK_FILTER_REMOVE;
      }
      break;

    case PropertyNotify:
      if (win) {
        win->PropertyChanged (xev->xproperty.atom, xev->xproperty.state == PropertyDelete);
        return GDK_FILTER_REMOVE;
      }
      break;

    case _FocusIn:
      if (xev->xfocus.mode == NotifyUngrab) {
        SPEW("FocusIn: focusing main window!\n");
        EnableOverlayInput (true);
        return GDK_FILTER_REMOVE;
      }
      break;

    case _FocusOut:
      if ((xwin == mMainwin || xwin == mMainwinParent) &&
          xev->xfocus.mode == NotifyGrab) {
        SPEW ("FocusOut: UNfocusing main window!\n");
        EnableOverlayInput (false);
        return GDK_FILTER_REMOVE;
      }
      break;

    default:
      if (win && xev->type == damage_event + XDamageNotify) {
        XDamageNotifyEvent *damage_ev = (XDamageNotifyEvent *) xev;

        int cnt = 0;
        do {
          win->Damaged (&damage_ev->area);
          cnt++;
        }
#if CLEAR_PENDING_X_EVENTS
        while (XCheckTypedEvent (mXDisplay, damage_event + XDamageNotify, xev));
#else
        while (0);
#endif

        if (cnt > 1) {
          SPEW_EVENT ("DAMAGE: Handled %d pending events!\n", cnt);
        }

        return GDK_FILTER_REMOVE;
      } else if (xev->type == xfixes_event + XFixesCursorNotify) {
        XFixesCursorNotifyEvent *cursor_ev = (XFixesCursorNotifyEvent *) xev;

        NS_NOTYETIMPLEMENTED ("CursorNotify event");

        return GDK_FILTER_REMOVE;
      } else if (xev->type == shape_event + ShapeNotify) {
        XShapeEvent *shape_ev = (XShapeEvent *) xev;

        NS_NOTYETIMPLEMENTED ("ShapeNotify event");

        return GDK_FILTER_REMOVE;
      }
      break;
  }

  return GDK_FILTER_CONTINUE;
}

