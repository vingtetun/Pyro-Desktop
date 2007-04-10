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

#include <gdk/gdkx.h>
#include <gdk/gdkdisplay.h>
#include <gdk/gdkscreen.h>

#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xcomposite.h>

#if HAVE_XEVIE
#include <X11/extensions/Xevie.h>
#endif


// Show windows from display :0
#define DEMO_HACK 0


#define DEBUG(format...) printf("   - " format)
#define INFO(format...) printf(" *** " format)
#define WARNING(format...) printf(" !!! " format)
#define ERROR(format...) fprintf(stderr, format)


NS_IMPL_ISUPPORTS1_CI(compzillaControl, compzillaIControl);


int compzillaControl::sErrorCnt;


compzillaControl::compzillaControl()
{
    mWindowMap.Init(50);
}


compzillaControl::~compzillaControl()
{
    if (mRoot) {
        gdk_window_unref(mRoot);
    }

#if HAVE_XEVIE
    // Stop intercepting mouse/keyboard events
    XevieEnd (mXDisplay);
#endif
}


GdkFilterReturn
compzillaControl::gdk_filter_func (GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
    compzillaControl *control = reinterpret_cast<compzillaControl*>(data);
    return control->Filter (xevent, event);
}


GdkWindow *
compzillaControl::GetNativeWindow(nsIDOMWindow *window)
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

            if (widget) {
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

    if (!XDamageQueryExtension (mXDisplay, &damage_event, &damage_error)) {
	ERR_RET ("No damage extension\n");
    }

#if HAVE_XEVIE
    if (!XQueryExtension (mXDisplay, "XEVIE", &opcode, &xevie_event, &xevie_error)) {
	ERR_RET ("No Xevie extension\n");
    }

    int evie_major, evie_minor;
    if (!XevieQueryVersion (mXDisplay, &evie_major, &evie_minor)) {
	ERR_RET ("No Xevie extension\n");
    }
#endif

    if (!XFixesQueryExtension (mXDisplay, &xfixes_event, &xfixes_error)) {
	ERR_RET ("No XFixes extension\n");
    }

#undef ERR_RET

    return true;
}


NS_IMETHODIMP
compzillaControl::RegisterWindowManager(nsIDOMWindow *window, compzillaIWindowManager* wm)
{
    Display *dpy;
    nsresult rv = NS_OK;

    DEBUG ("RegisterWindowManager\n");

    mDOMWindow = window;
    mWM = wm;

#if DEMO_HACK
    mDisplay = gdk_display_open (":0");
    mRoot = gdk_window_foreign_new_for_display (mDisplay, XDefaultRootWindow (mXDisplay));
#else
    mRoot = gdk_get_default_root_window();
    mDisplay = gdk_display_get_default ();
#endif
    mXDisplay = GDK_DISPLAY_XDISPLAY (mDisplay);

    // Check extension versions
    if (!InitXExtensions ()) {
        ERROR ("InitXExtensions failed");
        exit (1); // return NS_ERROR_UNEXPECTED;
    }

    // Register as window manager
    if (!InitWindowState ()) {
        ERROR ("InitWindowState failed");
        exit (1); // return NS_ERROR_UNEXPECTED;
    }

    // Take over drawing
    if (!InitOutputWindow ()) {
        ERROR ("InitOutputWindow failed");
        exit (1); // return NS_ERROR_UNEXPECTED;
    }

    // not sure if we need this here.  it's to perform the initial
    // compositing to the root buffer.  what we really want, though,
    // is to perform an initial composite onto each toplevel window.

    // paint_all (dpy, None);

    return rv;
}


NS_IMETHODIMP
compzillaControl::GetStringProperty (PRUint32 xid, const char *property, nsAString& value)
{
    DEBUG ("GetStringProperty (prop = %s)\n", property);

    Atom prop = XInternAtom (mXDisplay, property, FALSE);
    Atom actual_type;
    int format;
    unsigned long nitems;
    unsigned long bytes_after_return;
    unsigned char *data;

    XGetWindowProperty (mXDisplay, xid, prop,
                        0, BUFSIZ, false, XA_STRING, 
                        &actual_type, &format, &nitems, &bytes_after_return, 
                        &data);

    // XXX this is wrong - it's not always ASCII.  look at metacity's
    // handling of it (it calls a gdk function to convert the text
    // property to utf8).
    value = NS_ConvertASCIItoUTF16 ((const char*)data);

    return NS_OK;
}


bool
compzillaControl::InitWindowState ()
{
    // Get ALL events for ALL windows
    gdk_window_add_filter (NULL, gdk_filter_func, this);

    // Just ignore errors for now
    XSetErrorHandler (ErrorHandler);

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

    XUngrabServer (mXDisplay);

#if HAVE_XEVIE
    // Intercept mouse/keyboard events
    XevieStart (mXDisplay);
    XevieSelectInput (mXDisplay, (ButtonPressMask | 
                                  ButtonReleaseMask |
                                  KeyPressMask | 
                                  KeyReleaseMask));
#endif

#if 0
    XGrabServer (mXDisplay);

    Window *children;
    Window root_notused, parent_notused;
    unsigned int nchildren;

    XQueryTree (mXDisplay, 
                GDK_WINDOW_XID (mRoot), 
                &root_notused, 
                &parent_notused, 
                &children, 
                &nchildren);

    for (int i = 0; i < nchildren; i++) {
        AddWindow (children[i]);
    }

    XFree (children);

    XGrabServer (mXDisplay);
#endif

    return true;
}


bool
compzillaControl::InitOutputWindow ()
{
    mMainwin = GetNativeWindow(mDOMWindow);
    if (!mMainwin) {
        ERROR ("failed to get native window\n");
        return false;
    }

    gdk_window_set_override_redirect(mMainwin, true);

    // Create the overlay window, and make the X server stop drawing windows
    mOverlay = XCompositeGetOverlayWindow (mXDisplay, GDK_WINDOW_XID (mRoot));
    if (!mOverlay) {
        ERROR ("failed call to XCompositeGetOverlayWindow\n");
        return false;
    }

    // Put the our window into the overlay
    XReparentWindow (mXDisplay, GDK_DRAWABLE_XID (mMainwin), mOverlay, 0, 0);

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

    XRectangle rect = { 0, 0, 640, 480 };
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
compzillaControl::ErrorHandler (Display *, XErrorEvent *)
{
    sErrorCnt++;
    ERROR ("\nGOT AN ERROR!!\n\n");
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
compzillaControl::PropertyChanged (Window win, const char *prop)
{
    compzillaWindow *compwin = FindWindow (win);
    if (compwin && compwin->mContent) {
        mWM->PropertyChanged (compwin->mContent, prop);
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
    case ConfigureRequest: 
        XMoveResizeWindow (mXDisplay, 
                           x11_event->xconfigurerequest.window,
                           MAX (x11_event->xconfigurerequest.x, 30),
                           MAX (x11_event->xconfigurerequest.y, 30),
                           x11_event->xconfigurerequest.width,
                           x11_event->xconfigurerequest.height);
        break;
    case ConfigureNotify: {
        DEBUG ("ConfigureNotify: window=0x%0x, x=%d, y=%d, width=%d, height=%d\n",
                x11_event->xconfigure.window,
                x11_event->xconfigure.x,
                x11_event->xconfigure.y,
                x11_event->xconfigure.width,
                x11_event->xconfigure.height);

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

        compzillaWindow *compwin = FindWindow (x11_event->xreparent.window);

        if (x11_event->xreparent.parent == GDK_DRAWABLE_XID (mRoot) && !compwin) {
            AddWindow (x11_event->xreparent.window);
        } else if (compwin) {
            /* 
             * This is the only case where a window is removed but not
             * destroyed. We must remove our event mask and all passive
             * grabs.  -- compiz
             */
            XSelectInput (mXDisplay, x11_event->xreparent.window, NoEventMask);
            XShapeSelectInput (mXDisplay, x11_event->xreparent.window, NoEventMask);
            XUngrabButton (mXDisplay, AnyButton, AnyModifier, x11_event->xreparent.window);

            DestroyWindow (x11_event->xreparent.window);
        }

        break;
    }
    case MapRequest:
        XMapRaised (mXDisplay, x11_event->xmaprequest.window);
        break;
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
        PropertyChanged (x11_event->xproperty.window,
                         XGetAtomName(x11_event->xany.display, x11_event->xproperty.atom));
        break;
    }
    case _KeyPress:
    case KeyRelease:
    case ButtonPress:
    case ButtonRelease:
    case MotionNotify: {
        switch (x11_event->type) {
        case _KeyPress:
            DEBUG ("KeyPress: window=0x%0x, state=%d, keycode=%d\n", 
                   x11_event->xkey.window, x11_event->xkey.state, x11_event->xkey.keycode);
            break;
        case KeyRelease:
            DEBUG ("KeyRelease: window=0x%0x, state=%d, keycode=%d\n", 
                   x11_event->xkey.window, x11_event->xkey.state, x11_event->xkey.keycode);
            break;
        case ButtonPress:
            DEBUG ("ButtonPress: window=0x%0x, x=%d, y=%d, button=%d\n", 
                   x11_event->xbutton.window, x11_event->xbutton.x, x11_event->xbutton.y,
                   x11_event->xbutton.button);
            break;
        case ButtonRelease:
            DEBUG ("ButtonRelease: window=0x%0x, x=%d, y=%d, button=%d\n", 
                   x11_event->xbutton.window, x11_event->xbutton.x, x11_event->xbutton.y,
                   x11_event->xbutton.button);
            break;
        case MotionNotify:
            DEBUG ("MotionNotify: window=0x%0x, x=%d, y=%d, state=%d\n", 
                   x11_event->xmotion.window, x11_event->xmotion.x, x11_event->xmotion.y, 
                   x11_event->xmotion.state);
            break;
        }

#if HAVE_XEVIE
        // Redirect all mouse/kayboard events to our mozilla window.
        Window mainwin = GDK_WINDOW_XID (mMainwin);
        if (x11_event->xany.window != mainwin) {
            x11_event->xany.window = mainwin;
            XevieSendEvent (mXDisplay, x11_event, XEVIE_MODIFIED);
        } else {
            XevieSendEvent (mXDisplay, x11_event, XEVIE_UNMODIFIED);
        }
        // Call nsIWidget::DispatchEvent?
#endif

        // Don't let Gtk process the original event.
        return GDK_FILTER_CONTINUE;
    }
    default:
        if (x11_event->type == damage_event + XDamageNotify) {
            XDamageNotifyEvent *damage_ev = (XDamageNotifyEvent *)x11_event;

            // Signal the server to send more damage events
            XDamageSubtract (damage_ev->display, damage_ev->damage, None, None);

            compzillaWindow *compwin = FindWindow (damage_ev->drawable);
            if (compwin) {
                DEBUG ("DAMAGE: drawable=%p, damage=%p, x=%d, y=%d, width=%d, height=%d\n", 
                       damage_ev->drawable, damage_ev->damage, damage_ev->area.x,
                       damage_ev->area.y, damage_ev->area.width, damage_ev->area.height);

                nsCOMPtr<compzillaIRenderingContextInternal> internal;
                nsCOMPtr<nsIDOMHTMLCanvasElement> canvas = do_QueryInterface (compwin->mContent);

                nsresult rv = canvas->GetContext (NS_LITERAL_STRING ("compzilla"), getter_AddRefs (internal));
                if (NS_SUCCEEDED (rv)) {
                    internal->CopyImageDataFrom (mXDisplay, compwin->mPixmap,
                                                 damage_ev->area.x, damage_ev->area.y,
                                                 damage_ev->area.width, damage_ev->area.height);
                }
            }
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



