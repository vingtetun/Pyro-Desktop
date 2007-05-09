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
    void PropertyChanged (Window win, Atom prop, bool deleted);
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
    nsClassHashtable<nsUint32HashKey, compzillaWindow> mWindowMap;

    compzillaEventManager mWindowCreateEvMgr;
    compzillaEventManager mWindowDestroyEvMgr;

    static int composite_event, composite_error;
    static int xevie_event, xevie_error;
    static int damage_event, damage_error;
    static int xfixes_event, xfixes_error;
    static int shape_event, shape_error;
    static int render_event, render_error;
};
