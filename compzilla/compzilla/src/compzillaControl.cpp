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

#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xdamage.h>
#include <X11/extensions/Xrender.h>

compzillaControl::compzillaControl()
{
    printf ("ctor\n");

    window_map = new nsTHashtable<DictionaryEntry>();
    window_map->Init (50);
}


compzillaControl::~compzillaControl()
{
    delete window_map;
}

#define DEMO_HACK 0

NS_IMETHODIMP
compzillaControl::RegisterWindowManager(compzillaIWindowManager* wm)
{
    Display *dpy;
    nsresult rv = NS_OK;
    int	composite_major, composite_minor;

    printf ("RegisterWindowManager\n");

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

    if (!XRenderQueryExtension (dpy, &render_event, &render_error))
    {
	fprintf (stderr, "No render extension\n");
	return -1; // XXX
    }
    if (!XQueryExtension (dpy, COMPOSITE_NAME, &composite_opcode,
			  &composite_event, &composite_error))
    {
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
    if (!XFixesQueryExtension (dpy, &xfixes_event, &xfixes_error))
    {
	fprintf (stderr, "No XFixes extension\n");
	return -1; // XXX
    }

    gdk_window_set_events (root,
                           (GdkEventMask) (GDK_SUBSTRUCTURE_MASK |
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
    Window w = x11_event->xany.window;

    switch (x11_event->type) {
    case CreateNotify: {
        printf ("CreateNotify: window=0x%0x, x=%d, y=%d, width=%d, height=%d\n",
                x11_event->xcreatewindow.window,
                x11_event->xcreatewindow.x,
                x11_event->xcreatewindow.y,
                x11_event->xcreatewindow.width,
                x11_event->xcreatewindow.height);

        nsISupports *rv;

        wm->WindowCreated (x11_event->xcreatewindow.window, x11_event->xcreatewindow.override_redirect != 0, &rv);

        printf ("WindowCreated returned %p\n", rv);

        if (rv != NULL) {
            DictionaryEntry* a = window_map->PutEntry(x11_event->xcreatewindow.window);
            a->content = getter_AddRefs(rv);

            //XCompositeRedirectSubwindows (GDK_WINDOW_XDISPLAY (root), x11_event->xcreatewindow.window, CompositeRedirectAutomatic);
        }

        break;
    }
    case ConfigureNotify: {
        printf ("ConfigureNotify: window=0x%0x, x=%d, y=%d, width=%d, height=%d\n",
                x11_event->xconfigure.window,
                x11_event->xconfigure.x,
                x11_event->xconfigure.y,
                x11_event->xconfigure.width,
                x11_event->xconfigure.height);

        DictionaryEntry* b = window_map->GetEntry(x11_event->xconfigure.window);
        if (b != NULL) {
            nsISupports *win = b->content;
            nsISupports *above = NULL;

            b = window_map->GetEntry(x11_event->xconfigure.above);
            if (b != NULL)
                above = b->content;

            printf ("calling WindowConfigured with %p\n", win);

            wm->WindowConfigured (win,
                                  x11_event->xconfigure.x,
                                  x11_event->xconfigure.y,
                                  x11_event->xconfigure.width,
                                  x11_event->xconfigure.height,
                                  above);

            // XXX recreate our XImage backing store, but only if we're
            // mapped (i don't think we'll get damage until we're
            // displayed)
        }
        break;
    }
    case DestroyNotify: {
        DictionaryEntry* b = window_map->GetEntry(x11_event->xconfigure.window);
        if (b != NULL) {
            nsISupports *win = b->content;

            printf ("DestroyNotify: window=0x%0x\n", 
                    x11_event->xdestroywindow.window);

            wm->WindowDestroyed (win);

            window_map->RemoveEntry (x11_event->xconfigure.window);
        }
        break;
    }
    case MapNotify: {
        printf ("MapNotify: window=0x%0x\n", 
                x11_event->xmap.window);
        DictionaryEntry* b = window_map->GetEntry(x11_event->xconfigure.window);
        if (b != NULL) {
            nsISupports *win = b->content;
            wm->WindowMapped (win);
        }
        break;
    }
    case UnmapNotify: {
        printf ("UnmapNotify: window=0x%0x\n", 
                x11_event->xunmap.window);
        DictionaryEntry* b = window_map->GetEntry(x11_event->xconfigure.window);
        if (b != NULL) {
            nsISupports *win = b->content;
            wm->WindowUnmapped (win);
        }
        break;
    }
    case PropertyNotify:
        printf ("PropertyChange: window=0x%0x\n", 
                x11_event->xproperty.window);
        // XXX generate event "windowpropertychanged"
        break;
    default:
        printf ("unhandled window event %d\n", x11_event->type);
        break;
    }

    return GDK_FILTER_CONTINUE;
}

GdkFilterReturn
compzillaControl::gdk_filter_func (GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
    compzillaControl *control = reinterpret_cast<compzillaControl*>(data);
    return control->Filter (xevent, event);
}

NS_IMPL_ISUPPORTS1_CI(compzillaControl, compzillaIControl);

