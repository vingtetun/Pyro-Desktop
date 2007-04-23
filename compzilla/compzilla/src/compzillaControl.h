/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#include <nsCOMPtr.h>
#define MOZILLA_INTERNAL_API
#include <nsClassHashtable.h>
#undef MOZILLA_INTERNAL_API
#include <nsIDOMEventTarget.h>
#include <nsIWidget.h>

#include "compzillaIControl.h"
#include "compzillaEventManager.h"
#include "compzillaWindow.h"

extern "C" {
#include <gdk/gdkwindow.h>
#include <X11/Xlib.h>
}

typedef struct {
    Atom _NET_ACTIVE_WINDOW;
    Atom _NET_CLIENT_LIST;
    Atom _NET_CLOSE_WINDOW;
    Atom _NET_CURRENT_DESKTOP;
    Atom _NET_DESKTOP_GEOMETRY;
    Atom _NET_DESKTOP_LAYOUT;
    Atom _NET_DESKTOP_NAMES;
    Atom _NET_DESKTOP_VIEWPORT;
    Atom _NET_FRAME_EXTENTS;
    Atom _NET_MOVERESIZE_WINDOW;
    Atom _NET_NUMBER_OF_DESKTOPS;
    Atom _NET_REQUEST_FRAME_EXTENTS;
    Atom _NET_RESTACK_WINDOW;
    Atom _NET_SHOWING_DESKTOP;
    Atom _NET_SUPPORTED;
    Atom _NET_SUPPORTING_WM_CHECK;
    Atom _NET_VIRTUAL_ROOTS;
    Atom _NET_WM_ALLOWED_ACTIONS;
    Atom _NET_WM_DESKTOP;
    Atom _NET_WM_HANDLED_ICONS;
    Atom _NET_WM_ICON;
    Atom _NET_WM_ICON_GEOMETRY;
    Atom _NET_WM_ICON_NAME;
    Atom _NET_WM_MOVERESIZE;
    Atom _NET_WM_NAME;
    Atom _NET_WM_PID;
    Atom _NET_WM_PING;
    Atom _NET_WM_STATE;
    Atom _NET_WM_STRUT;
    Atom _NET_WM_STRUT_PARTIAL;
    Atom _NET_WM_SYNC_REQUEST;
    Atom _NET_WM_USER_TIME;
    Atom _NET_WM_VISIBLE_ICON_NAME;
    Atom _NET_WM_VISIBLE_NAME;
    Atom _NET_WM_WINDOW_TYPE;
    Atom _NET_WORKAREA;
    Atom WM_COLORMAP_WINDOWS;
    Atom WM_PROTOCOLS;
} XAtoms;

class compzillaControl
    : public compzillaIControl,
      public nsIDOMEventTarget
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_COMPZILLAICONTROL
    NS_DECL_NSIDOMEVENTTARGET

    compzillaControl ();
    virtual ~compzillaControl ();

private:
    compzillaWindow *FindWindow (Window win);

    void AddWindow (Window win);
    void DestroyWindow (Window win);
    void ForgetWindow (Window win);
    void MapWindow (Window win);
    void UnmapWindow (Window win);
    void PropertyChanged (Window win, Atom prop);
    void WindowDamaged (Window win, XRectangle *rect);

    GdkWindow *GetNativeWindow(nsIDOMWindow *window);
    nsCOMPtr<nsIWidget> GetNativeWidget(nsIDOMWindow *window);

    bool InitXAtoms ();
    bool InitXExtensions ();
    bool InitOutputWindow ();
    bool InitManagerWindow ();
    bool InitWindowState ();

    void ShowOutputWindow ();
    void HideOutputWindow ();

    GdkFilterReturn Filter (GdkXEvent *xevent, GdkEvent *event);

    static GdkFilterReturn gdk_filter_func (GdkXEvent *xevent, 
                                            GdkEvent *event, 
                                            gpointer data);
    static int ErrorHandler (Display *, XErrorEvent *);
    static int ClearErrors (Display *dpy);
    static int sErrorCnt;

    GdkDisplay *mDisplay;
    Display *mXDisplay;

    GdkWindow *mRoot;
    GdkWindow *mMainwin;
    Window mMainwinParent;
    Window mOverlay;

    Window mManagerWindow;
    bool mIsWindowManager;
    bool mIsCompositor;

    nsCOMPtr<nsIDOMWindow> mDOMWindow;
    nsCOMPtr<compzillaIWindowManager> mWM;
    nsClassHashtable<nsUint32HashKey, compzillaWindow> mWindowMap;

    compzillaEventManager mWindowCreateEvMgr;
    compzillaEventManager mWindowDestroyEvMgr;

    static int composite_event, composite_error;
    static int xevie_event, xevie_error;
    static int damage_event, damage_error;
    static int xfixes_event, xfixes_error;
    static int shape_event, shape_error;
    static int render_event, render_error;

    XAtoms atoms;
};
