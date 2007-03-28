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


CompositedWindow::CompositedWindow()
{
}


CompositedWindow::~CompositedWindow()
{
    if (mNativeWindow) {
        /*
        if (mDamage) {
            XDamageDestroy(GDK_WINDOW_XDISPLAY(mNativeWindow), mDamage);
        }
        if (mPicture) {
            XRenderFreePicture(GDK_WINDOW_XDISPLAY(mNativeWindow), mDamage);
        }
        */
        gdk_window_unref(mNativeWindow);
    }
}


compzillaControl::compzillaControl()
{
    mWindowMap.Init(50);
}


compzillaControl::~compzillaControl()
{
    gdk_window_unref(root);
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
                GdkWindow *iframe = (GdkWindow *)widget->GetNativeData(NS_NATIVE_WINDOW);
                GdkWindow *toplevel = gdk_window_get_toplevel(iframe);
                printf ("GetNativeWindow: toplevel=0x%0x, iframe=0x%0x\n", 
                        GDK_DRAWABLE_XID(toplevel), GDK_DRAWABLE_XID(iframe));
                return toplevel;
            }
        }
    }

    printf (" !!! Could not get GdkWindow for nsIDOMWindow %p\n", window);
    return NULL;
}


NS_IMETHODIMP
compzillaControl::RegisterWindowManager(nsIDOMWindow *window, compzillaIWindowManager* wm)
{
    Display *dpy;
    nsresult rv = NS_OK;
    int	composite_major, composite_minor;

    printf ("RegisterWindowManager\n");

    this->mainwin = GetNativeWindow(window);
    this->wm = wm;

#if DEMO_HACK
    GdkDisplay* gdk_dpy = gdk_display_open (":0");
    dpy = GDK_DISPLAY_XDISPLAY (gdk_dpy);
    root = gdk_window_foreign_new_for_display (gdk_dpy,
                                               XDefaultRootWindow (dpy));
#else
    root = gdk_get_default_root_window ();
    dpy = GDK_WINDOW_XDISPLAY (root);
#endif
    if (!dpy) {
        fprintf (stderr, "Can't open display\n");
        return -1; // XXX
    }

    if (!XRenderQueryExtension (dpy, &render_event, &render_error)) {
	fprintf (stderr, "No render extension\n");
	return -1; // XXX
    }
    if (!XQueryExtension (dpy, COMPOSITE_NAME, &composite_opcode,
			  &composite_event, &composite_error)) {
	fprintf (stderr, "No composite extension\n");
        return -1; // XXX
    }

    XCompositeQueryVersion (dpy, &composite_major, &composite_minor);
#if HAS_NAME_WINDOW_PIXMAP
    if (composite_major > 0 || composite_minor >= 2)
	hasNamePixmap = True;
#endif

    if (!XDamageQueryExtension (dpy, &damage_event, &damage_error))
    {
	fprintf (stderr, "No damage extension\n");
	return -1; // XXX
    }
    printf("\n\nDAMAGE INFO: event=%d, error=%d\n", damage_event, damage_error);

    if (!XFixesQueryExtension (dpy, &xfixes_event, &xfixes_error))
    {
	fprintf (stderr, "No XFixes extension\n");
	return -1; // XXX
    }
    printf("\n\nXFIXES INFO: event=%d, error=%d\n", xfixes_event, xfixes_error);

    gdk_window_set_events (root, (GdkEventMask)(GDK_SUBSTRUCTURE_MASK |
                                                GDK_STRUCTURE_MASK |
                                                GDK_PROPERTY_CHANGE_MASK));
    gdk_window_add_filter (root, gdk_filter_func, this);

#if false
    gdk_x11_grab_server ();

    // XXX this needs some gdk-ification
    Window *children;
    Window root_return, parent_return;
    int i;
    unsigned int nchildren;
    XQueryTree (dpy, GDK_WINDOW_XID (root), &root_return, &parent_return, &children, &nchildren);
    for (i = 0; i < nchildren; i++) {
        XCompositeRedirectSubwindows (dpy, children[i], CompositeRedirectAutomatic);
        printf ("adding window 0x%0x\n", children[i]);
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
compzillaControl::AddWindow (Window id, Window prev)
{
    // XXX we need to emit an event so that the javascript side of
    // things can create a canvas element and pass it back to us
#if false

    win				*new = malloc (sizeof (win));
    win				**p;
    
    if (!new)
	return;
    if (prev)
    {
	for (p = &list; *p; p = &(*p)->next)
	    if ((*p)->id == prev)
		break;
    }
    else
	p = &list;
    new->id = id;
    set_ignore (dpy, NextRequest (dpy));
    if (!XGetWindowAttributes (dpy, id, &new->a))
    {
	free (new);
	return;
    }
    new->damaged = 0;
#if CAN_DO_USABLE
    new->usable = False;
#endif
#if HAS_NAME_WINDOW_PIXMAP
    new->pixmap = None;
#endif
    new->picture = None;
    if (new->a.class == InputOnly)
    {
	new->damage_sequence = 0;
	new->damage = None;
    }
    else
    {
	new->damage_sequence = NextRequest (dpy);
	new->damage = XDamageCreate (dpy, id, XDamageReportNonEmpty);
    }
    new->extents = None;

    /* moved mode setting to one place */
    determine_mode (dpy, new);
    
    new->next = *p;
    *p = new;
    if (new->a.map_state == IsViewable)
	map_win (dpy, id, new->damage_sequence - 1);
#endif
}


GdkFilterReturn
compzillaControl::Filter (GdkXEvent *xevent, GdkEvent *event)
{
    XEvent *x11_event = (XEvent*)xevent;

    switch (x11_event->type) {
    case CreateNotify: {
        printf ("CreateNotify: window=0x%0x, x=%d, y=%d, width=%d, height=%d\n",
                x11_event->xcreatewindow.window,
                x11_event->xcreatewindow.x,
                x11_event->xcreatewindow.y,
                x11_event->xcreatewindow.width,
                x11_event->xcreatewindow.height);

        if (x11_event->xcreatewindow.window == GDK_DRAWABLE_XID(this->mainwin)) {
            printf("CreateNotify: discarding event on mainwin\n");
            return GDK_FILTER_CONTINUE;
        }

        CompositedWindow *compwin = new CompositedWindow();
        wm->WindowCreated (x11_event->xcreatewindow.window, 
                           x11_event->xcreatewindow.override_redirect != 0, 
                           getter_AddRefs(compwin->mContent));

        printf ("WindowCreated returned %p\n", compwin->mContent.get());

        if (!compwin->mContent) {
            delete compwin;
            break;
        }

        compwin->mNativeWindow = 
            gdk_window_foreign_new_for_display (gdk_drawable_get_display(this->root), 
                                                x11_event->xcreatewindow.window);
        gdk_window_add_filter (compwin->mNativeWindow, gdk_filter_func, this);
        gdk_window_set_events (compwin->mNativeWindow, GDK_PROPERTY_CHANGE_MASK);

        /* Don't need to receive all these events */
        /*
        XSelectInput(x11_event->xany.display, 
                     x11_event->xcreatewindow.window, 
                     (StructureNotifyMask |
                      PropertyChangeMask | 
                      ExposureMask |
                      VisibilityChangeMask | 
                      damage_event + XDamageNotify));
        */


        // Set up damage notification
        compwin->mDamage = XDamageCreate (x11_event->xany.display, 
                                         x11_event->xcreatewindow.window, 
                                         XDamageReportNonEmpty);

        printf ("XDamageCreate returned %x\n", compwin->mDamage);

        XCompositeRedirectWindow(x11_event->xany.display, 
                                 x11_event->xcreatewindow.window, 
                                 CompositeRedirectAutomatic);
        
        XWindowAttributes attr = { 0 };
        XGetWindowAttributes(x11_event->xany.display, 
                             x11_event->xcreatewindow.window, 
                             &attr);

        const XRenderPictFormat *format = XRenderFindVisualFormat(x11_event->xany.display, 
                                                                  attr.visual);

        // Get handle for fetching window content
        XRenderPictureAttributes pa = { 0 };
        pa.subwindow_mode = IncludeInferiors; // Don't clip child widgets
        compwin->mPicture = XRenderCreatePicture(x11_event->xany.display, 
                                                  x11_event->xcreatewindow.window, 
                                                  format, 
                                                  CPSubwindowMode, 
                                                  &pa);

        printf ("XRenderCreatePicture returned %x\n", compwin->mPicture);

        mWindowMap.Put (x11_event->xcreatewindow.window, compwin);

        break;
    }
    case ConfigureNotify: {
        printf ("ConfigureNotify: window=0x%0x, x=%d, y=%d, width=%d, height=%d\n",
                x11_event->xconfigure.window,
                x11_event->xconfigure.x,
                x11_event->xconfigure.y,
                x11_event->xconfigure.width,
                x11_event->xconfigure.height);

        CompositedWindow *compwin;
        if (!mWindowMap.Get(x11_event->xconfigure.window, &compwin)) {
            break;
        }

        CompositedWindow *abovewin;
        mWindowMap.Get(x11_event->xconfigure.above, &abovewin);

        wm->WindowConfigured (compwin->mContent,
                              x11_event->xconfigure.x,
                              x11_event->xconfigure.y,
                              x11_event->xconfigure.width,
                              x11_event->xconfigure.height,
                              abovewin ? abovewin->mContent : NULL);

        // XXX recreate our XImage backing store, but only if we're
        // mapped (i don't think we'll get damage until we're
        // displayed)
        break;
    }
    case ReparentNotify: {
        printf ("ReparentNotify: window=0x%0x, parent=0x%0x, x=%d, y=%d, override_redirect=%d\n",
                x11_event->xreparent.window,
                x11_event->xreparent.parent,
                x11_event->xreparent.x,
                x11_event->xreparent.y,
                x11_event->xreparent.override_redirect);

        if (x11_event->xreparent.window == GDK_DRAWABLE_XID(this->mainwin) &&
            x11_event->xreparent.parent != GDK_DRAWABLE_XID(this->root)) {
            printf("ReparentNotify: discarding event on mainwin\n");

            CompositedWindow *compwin;
            if (!mWindowMap.Get(x11_event->xreparent.window, &compwin)) {
                break;
            }

            wm->WindowDestroyed (compwin->mContent);
            mWindowMap.Remove (x11_event->xreparent.window);
        }
        break;
    }
    case DestroyNotify: {
        printf ("DestroyNotify: window=0x%0x\n", x11_event->xdestroywindow.window);

        CompositedWindow *compwin;
        if (!mWindowMap.Get(x11_event->xdestroywindow.window, &compwin)) {
            break;
        }

        wm->WindowDestroyed (compwin->mContent);
        mWindowMap.Remove (x11_event->xdestroywindow.window);
        break;
    }
    case MapNotify: {
        printf ("MapNotify: window=0x%0x\n", x11_event->xmap.window);

        CompositedWindow *compwin;
        if (!mWindowMap.Get(x11_event->xmap.window, &compwin)) {
            break;
        }

        wm->WindowMapped (compwin->mContent);
        break;
    }
    case UnmapNotify: {
        printf ("UnmapNotify: window=0x%0x\n", 
                x11_event->xunmap.window);

        CompositedWindow *compwin;
        if (!mWindowMap.Get(x11_event->xunmap.window, &compwin)) {
            break;
        }

        wm->WindowUnmapped (compwin->mContent);
        break;
    }
    case PropertyNotify: {
        printf ("PropertyChange: window=0x%0x, atom=%s\n", 
                x11_event->xproperty.window, 
                XGetAtomName(x11_event->xany.display, x11_event->xproperty.atom));
        break;
    }
    default:
        if (x11_event->type == damage_event + XDamageNotify) {
            printf("\n\nDAMAGE!!!\n\n");

            XDamageNotifyEvent *damage_ev = (XDamageNotifyEvent *)x11_event;
            XDamageSubtract(damage_ev->display, damage_ev->damage, None, None);
        }
#if SHAPE_EXTENSION
        else if (x11_event->type == shape_event + ShapeNotify) {
        }
#endif
        else {
            printf ("Unhandled window event %d\n", x11_event->type);
        }
        break;
    }

    return GDK_FILTER_REMOVE;
}


GdkFilterReturn
compzillaControl::gdk_filter_func (GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
    compzillaControl *control = reinterpret_cast<compzillaControl*>(data);
    return control->Filter (xevent, event);
}


NS_IMPL_ISUPPORTS1_CI(compzillaControl, compzillaIControl);

