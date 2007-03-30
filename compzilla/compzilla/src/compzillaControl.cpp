/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#include "nspr.h"
#include "nsMemory.h"
#include "nsNetCID.h"
#include "nsISupportsUtils.h"
#include "nsIIOService.h"
#include "nsIObserverService.h"
#include "nsIURI.h"
#include "nsIScriptSecurityManager.h"
#include "nsServiceManagerUtils.h"
#include "nsStringAPI.h"

#include "compzillaControl.h"

// These headers are used for finding the GdkWindow for a DOM window
#include "nsIBaseWindow.h"
#include "nsIDocShell.h"
#include "nsIScriptGlobalObject.h"
#include "nsIWidget.h"

#include <gdk/gdkx.h>
#include <gdk/gdkdisplay.h>
#include <gdk/gdkscreen.h>

#include <X11/Xutil.h>
#include <X11/Xatom.h>


#define DEBUG(format...) printf("   - " format)
#define INFO(format...) printf(" *** " format)
#define WARNING(format...) printf(" !!! " format)
#define ERROR(format...) fprintf(stderr, format)


CompositedWindow::CompositedWindow(Display *display, Window win)
    : mContent(),
      mDisplay(display),
      mWindow(win)
{
    XGrabServer (display);

    XGetWindowAttributes(display, win, &mAttr);

    if (mAttr.map_state == IsViewable) {
        mPixmap = XCompositeNameWindowPixmap (display, win);
    }

    XUngrabServer (display);

    /* 
     * Set up damage notification.  RawRectangles gives us smaller grain
     * changes, versus NonEmpty which seems to always include the entire
     * contents.
     */
    mDamage = XDamageCreate (display, win, XDamageReportRawRectangles);

    DEBUG ("XDamageCreate returned %x\n", mDamage);

    // Get handle for fetching window content
    XRenderPictFormat *format = XRenderFindVisualFormat(display, mAttr.visual);
    XRenderPictureAttributes pa;
    pa.subwindow_mode = IncludeInferiors; // Don't clip child widgets
    mPicture = XRenderCreatePicture (display, win, format, CPSubwindowMode, &pa);

    DEBUG ("XRenderCreatePicture returned %x\n", mPicture);

    // Redirect output
    //XCompositeRedirectWindow(display, win, CompositeRedirectManual);
    XCompositeRedirectWindow(display, win, CompositeRedirectAutomatic);

    // Create a clips, used for determining what to draw
    //mClip = XCreateRegion();

    XSelectInput(display, win, (PropertyChangeMask | EnterWindowMask | FocusChangeMask));
}


CompositedWindow::~CompositedWindow()
{
    WARNING ("CompositedWindow::~CompositedWindow %p, xid=%p\n", this, mWindow);

    if (mDamage) {
        XDamageDestroy(mDisplay, mDamage);
    }
    if (mPicture) {
        XRenderFreePicture(mDisplay, mPicture);
    }
}


NS_IMPL_ISUPPORTS1_CI(compzillaControl, compzillaIControl);


compzillaControl::compzillaControl()
{
    if (!sEmptyRegion) {
        //sEmptyRegion = XCreateRegion();
    }

    mWindowMap.Init(50);
}


compzillaControl::~compzillaControl()
{
    if (mRoot) {
        gdk_window_unref(mRoot);
    }
}


GdkFilterReturn
compzillaControl::gdk_filter_func (GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
    compzillaControl *control = reinterpret_cast<compzillaControl*>(data);
    return control->Filter (xevent, event);
}


#define DEMO_HACK 0

GdkWindow *
compzillaControl::GetNativeWindow(nsIDOMWindow *window)
{
    nsCOMPtr<nsIScriptGlobalObject> global = do_QueryInterface(window);

    if (global) {
        nsCOMPtr<nsIBaseWindow> baseWin = do_QueryInterface(global->GetDocShell());

        if (baseWin) {
            nsCOMPtr<nsIWidget> widget;
            baseWin->GetMainWidget(getter_AddRefs(widget));

            if (widget) {
                //widget->SetBorderStyle(eBorderStyle_none);
                //INFO ("Clearing mainwin border\n");
                    
                GdkWindow *iframe = (GdkWindow *)widget->GetNativeData(NS_NATIVE_WINDOW);
                GdkWindow *toplevel = gdk_window_get_toplevel(iframe);
                DEBUG ("GetNativeWindow: toplevel=0x%0x, iframe=0x%0x\n", 
                        GDK_DRAWABLE_XID(toplevel), GDK_DRAWABLE_XID(iframe));
                gdk_window_ref(toplevel);

                return toplevel;
            }
        }
    }

    WARNING ("Could not get GdkWindow for nsIDOMWindow %p\n", window);
    return NULL;
}


NS_IMETHODIMP
compzillaControl::RegisterWindowManager(nsIDOMWindow *window, compzillaIWindowManager* wm)
{
    Display *dpy;
    nsresult rv = NS_OK;
    int	composite_major, composite_minor;

    DEBUG ("RegisterWindowManager\n");

    mWM = wm;

    mMainwin = GetNativeWindow(window);
    gdk_window_set_override_redirect(mMainwin, true);

#if DEMO_HACK
    mDisplay = gdk_display_open (":0");
    mRoot = gdk_window_foreign_new_for_display (mDisplay, XDefaultRootWindow (mXDisplay));
#else
    mRoot = gdk_get_default_root_window();
    mDisplay = gdk_display_get_default ();
#endif

    mXDisplay = GDK_DISPLAY_XDISPLAY (mDisplay);

    if (!XRenderQueryExtension (mXDisplay, &render_event, &render_error)) {
	ERROR ("No render extension\n");
	return -1; // XXX
    }
    if (!XQueryExtension (mXDisplay, COMPOSITE_NAME, &composite_opcode,
			  &composite_event, &composite_error)) {
	ERROR ("No composite extension\n");
        return -1; // XXX
    }

    XCompositeQueryVersion (mXDisplay, &composite_major, &composite_minor);
#if HAS_NAME_WINDOW_PIXMAP
    if (composite_major > 0 || composite_minor >= 2)
	hasNamePixmap = True;
#endif

    if (!XDamageQueryExtension (mXDisplay, &damage_event, &damage_error))
    {
	ERROR ("No damage extension\n");
	return -1; // XXX
    }
    DEBUG("DAMAGE INFO: event=%d, error=%d\n", damage_event, damage_error);

    if (!XFixesQueryExtension (mXDisplay, &xfixes_event, &xfixes_error))
    {
	ERROR ("No XFixes extension\n");
	return -1; // XXX
    }
    DEBUG("XFIXES INFO: event=%d, error=%d\n", xfixes_event, xfixes_error);

    gdk_window_set_events (mRoot, (GdkEventMask) (GDK_SUBSTRUCTURE_MASK |
                                                  GDK_STRUCTURE_MASK |
                                                  GDK_PROPERTY_CHANGE_MASK));
    
    // Get ALL events for ALL windows
    gdk_window_add_filter (NULL, gdk_filter_func, this);

    XSetErrorHandler(ErrorHandler);

    //mOverlay = XCompositeGetOverlayWindow (mXDisplay, GDK_WINDOW_XID (mRoot));
    //ShowOutputWindow ();


#if false
    gdk_x11_grab_server ();

    // XXX this needs some gdk-ification
    Window *children;
    Window root_return, parent_return;
    int i;
    unsigned int nchildren;
    XQueryTree (mXDisplay, GDK_WINDOW_XID (mRoot), &root_return, &parent_return, 
                &children, &nchildren);
    for (i = 0; i < nchildren; i++) {
        XCompositeRedirectSubwindows (mXDisplay, children[i], CompositeRedirectAutomatic);
        DEBUG ("adding window 0x%0x\n", children[i]);
        AddWindow (children[i], i ? children[i-1] : None);
    }
    XFree (children);

    gdk_x11_ungrab_server ();
#endif

    // not sure if we need this here.  it's to perform the initial
    // compositing to the root buffer.  what we really want, though,
    // is to perform an initial composite onto each toplevel window.

    // paint_all (dpy, None);

    return rv;
}


void 
compzillaControl::ShowOutputWindow()
{
    XserverRegion region = XFixesCreateRegion (mXDisplay, NULL, 0);

    XFixesSetWindowShapeRegion (mXDisplay,
                                mOverlay,
                                ShapeBounding,
                                0, 0, 
                                0);
    XFixesSetWindowShapeRegion (mXDisplay,
                                mOverlay,
                                ShapeInput,
                                0, 0, 
                                region);

    XFixesDestroyRegion (mXDisplay, region);
}


void 
compzillaControl::HideOutputWindow()
{
    XserverRegion region = XFixesCreateRegion (mXDisplay, NULL, 0);

    XFixesSetWindowShapeRegion (mXDisplay,
                                mOverlay,
                                ShapeBounding,
                                0, 0, 
                                region);

    XFixesDestroyRegion (mXDisplay, region);
}


PLDHashOperator 
compzillaControl::AddWindowDamage (const PRUint32& key,
                                   CompositedWindow *entry, 
                                   void *userData)
{
    Region workRegion = (Region) userData;
    if (entry->mAttr.map_state == IsViewable) {
        //XSubtractRegion(workRegion, sEmptyRegion, entry->mClip);
    }
    return PL_DHASH_NEXT;
}


int 
compzillaControl::ErrorHandler (Display *, XErrorEvent *)
{
    ERROR ("\nGOT AN ERROR!!\n\n");
    return 0;
}


void 
compzillaControl::GetDamage(Region region)
{
    static Region workRegion = NULL;
    if (!workRegion) {
        //workRegion = XCreateRegion ();
    }

    //XSubtractRegion (region, sEmptyRegion, workRegion);
    
    mWindowMap.EnumerateRead (AddWindowDamage, workRegion);
}


void
compzillaControl::AddWindow (Window win)
{
    INFO ("AddWindow for window %p\n", win);

    nsAutoPtr<CompositedWindow> compwin(new CompositedWindow(mXDisplay, win));

    if (compwin->mAttr.c_class == InputOnly) {
        WARNING ("AddWindow ignoring InputOnly window %p\n", win);
        return;
    }

    mWM->WindowCreated (win, 
                        compwin->mAttr.override_redirect != 0, 
                        getter_AddRefs(compwin->mContent));

    INFO ("WindowCreated returned %p\n", compwin->mContent.get());

    if (compwin->mContent) {
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
    CompositedWindow *compwin = FindWindow (win);
    if (compwin) {
        if (compwin->mContent) {
            mWM->WindowDestroyed (compwin->mContent);
        }

        // These are not valid if the window is already destroyed
        compwin->mDamage = 0;
        compwin->mPicture = 0;
        mWindowMap.Remove (win);
    }
}


void
compzillaControl::ForgetWindow (Window win)
{
    CompositedWindow *compwin = FindWindow (win);
    if (compwin) {
        if (compwin->mContent) {
            mWM->WindowDestroyed (compwin->mContent);
        }

        // Let the window render itself
        XCompositeUnredirectWindow (mXDisplay, win, CompositeRedirectManual);
        mWindowMap.Remove (win);
    }
}


void
compzillaControl::MapWindow (Window win)
{
    CompositedWindow *compwin = FindWindow (win);
    if (compwin && compwin->mContent) {
        mWM->WindowMapped (compwin->mContent);
    }
}


void
compzillaControl::UnmapWindow (Window win)
{
    CompositedWindow *compwin = FindWindow (win);
    if (compwin && compwin->mContent) {
        mWM->WindowUnmapped (compwin->mContent);
    }
}


PLDHashOperator 
compzillaControl::FindWindowForFrame (const PRUint32& key,
                                      CompositedWindow *entry, 
                                      void *userData)
{
    CompositedWindow **frame = (CompositedWindow **) userData;
    if (entry->mFrame == *frame) {
        *frame = entry;
        return PL_DHASH_STOP;
    }
    return PL_DHASH_NEXT;
}


CompositedWindow *
compzillaControl::FindToplevelWindow (Window win)
{
    CompositedWindow *compwin = FindWindow (win);

    if (compwin && compwin->mAttr.override_redirect) {
        // Likely a frame window
        if (compwin->mAttr.c_class == InputOnly) {
            CompositedWindow *found = compwin;
            mWindowMap.EnumerateRead (FindWindowForFrame, &found);

            if (found != compwin) {
                return found;
            }
        }

        return NULL;
    }

    return compwin;
}


CompositedWindow *
compzillaControl::FindWindow (Window win)
{
    CompositedWindow *compwin;
    mWindowMap.Get(win, &compwin);
    return compwin;
}


GdkFilterReturn
compzillaControl::Filter (GdkXEvent *xevent, GdkEvent *event)
{
    XEvent *x11_event = (XEvent*)xevent;

    switch (x11_event->type) {
    case CreateNotify: {
        DEBUG ("CreateNotify: window=0x%0x, x=%d, y=%d, width=%d, height=%d\n",
                x11_event->xcreatewindow.window,
                x11_event->xcreatewindow.x,
                x11_event->xcreatewindow.y,
                x11_event->xcreatewindow.width,
                x11_event->xcreatewindow.height);

        if (x11_event->xcreatewindow.window == GDK_DRAWABLE_XID(this->mMainwin)) {
            WARNING ("CreateNotify: discarding event on mainwin\n");
            return GDK_FILTER_REMOVE;
        }

        AddWindow(x11_event->xcreatewindow.window);
        break;
    }
    case DestroyNotify: {
        DEBUG ("DestroyNotify: window=0x%0x\n", x11_event->xdestroywindow.window);

        DestroyWindow (x11_event->xdestroywindow.window);
        break;
    }
    case ConfigureNotify: {
        DEBUG ("ConfigureNotify: window=0x%0x, x=%d, y=%d, width=%d, height=%d\n",
                x11_event->xconfigure.window,
                x11_event->xconfigure.x,
                x11_event->xconfigure.y,
                x11_event->xconfigure.width,
                x11_event->xconfigure.height);

        CompositedWindow *compwin = FindWindow (x11_event->xconfigure.window);

        if (compwin && compwin->mContent) {
            CompositedWindow *abovewin = FindWindow (x11_event->xconfigure.above);

            mWM->WindowConfigured (compwin->mContent,
                                   x11_event->xconfigure.x,
                                   x11_event->xconfigure.y,
                                   x11_event->xconfigure.width,
                                   x11_event->xconfigure.height,
                                   abovewin ? abovewin->mContent : NULL);
        }

        // XXX recreate our XImage backing store, but only if we're
        // mapped (i don't think we'll get damage until we're
        // displayed)
        break;
    }
    case ReparentNotify: {
        DEBUG ("ReparentNotify: window=0x%0x, parent=0x%0x, x=%d, y=%d, override_redirect=%d\n",
                x11_event->xreparent.window,
                x11_event->xreparent.parent,
                x11_event->xreparent.x,
                x11_event->xreparent.y,
                x11_event->xreparent.override_redirect);

        if (x11_event->xreparent.window == GDK_DRAWABLE_XID(this->mMainwin)) {
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

        CompositedWindow *compwin = FindWindow (x11_event->xreparent.window);

        if (x11_event->xreparent.parent == GDK_DRAWABLE_XID(this->mRoot) && !compwin) {
            AddWindow (x11_event->xreparent.window);
        } else if (compwin) {
            /* 
             * This is the only case where a window is removed but not
             * destroyed. We must remove our event mask and all passive
             * grabs. 
             */
            XSelectInput (mXDisplay, x11_event->xreparent.window, NoEventMask);
            XShapeSelectInput (mXDisplay, x11_event->xreparent.window, NoEventMask);
            XUngrabButton (mXDisplay, AnyButton, AnyModifier, x11_event->xreparent.window);

            DestroyWindow(x11_event->xreparent.window);
        }

        break;
    }
    case MapNotify: {
        DEBUG ("MapNotify: window=0x%0x\n", x11_event->xmap.window);

        MapWindow (x11_event->xmap.window);
        break;
    }
    case UnmapNotify: {
        DEBUG ("UnmapNotify: window=0x%0x\n", x11_event->xunmap.window);

        UnmapWindow (x11_event->xunmap.window);
        break;
    }
    case PropertyNotify: {
        DEBUG ("PropertyChange: window=0x%0x, atom=%s\n", 
               x11_event->xproperty.window, 
               XGetAtomName(x11_event->xany.display, x11_event->xproperty.atom));
        break;
    }
    default:
        if (x11_event->type == damage_event + XDamageNotify) {
            XDamageNotifyEvent *damage_ev = (XDamageNotifyEvent *)x11_event;

            CompositedWindow *compwin = FindWindow (damage_ev->drawable);
            if (compwin) {
                DEBUG("DAMAGE: drawable=%p, damage=%p, x=%d, y=%d, width=%d, height=%d\n", 
                      damage_ev->drawable, damage_ev->damage, damage_ev->area.x,
                      damage_ev->area.y, damage_ev->area.width, damage_ev->area.height);
            }

            // Signal the server to send more damage events
            XDamageSubtract(damage_ev->display, damage_ev->damage, None, None);
        }
#if SHAPE_EXTENSION
        else if (x11_event->type == shape_event + ShapeNotify) {
        }
#endif
        else {
            WARNING ("Unhandled window event %d\n", x11_event->type);
        }
        break;
    }

    return GDK_FILTER_CONTINUE;
}



