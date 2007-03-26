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

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>


compzillaControl::compzillaControl()
{
    printf ("ctor\n");
}


compzillaControl::~compzillaControl()
{
}

NS_IMETHODIMP
compzillaControl::RegisterWindowManager()
{
    Display *dpy;

    printf ("RegisterWindowManager\n");

    nsresult rv = NS_OK;
    int	composite_major, composite_minor;

    root = gdk_get_default_root_window ();

    dpy = GDK_WINDOW_XDISPLAY (root);
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

    gdk_x11_grab_server ();

    // XXX this needs some gdk-ification
    Window *children;
    Window root_return, parent_return;
    int i;
    unsigned int nchildren;
    XQueryTree (dpy, GDK_WINDOW_XID (root), &root_return, &parent_return, &children, &nchildren);
    for (i = 0; i < nchildren; i++) {
        XCompositeRedirectSubwindows (dpy, children[i], CompositeRedirectManual);
        AddWindow (children[i], i ? children[i-1] : None);
    }
    XFree (children);

    gdk_x11_ungrab_server ();

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

NS_IMPL_ISUPPORTS1_CI(compzillaControl, compzillaIControl);

