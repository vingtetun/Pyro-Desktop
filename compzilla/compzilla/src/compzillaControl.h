/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#include <nsCOMPtr.h>
#define MOZILLA_INTERNAL_API
#include <nsClassHashtable.h>
#undef MOZILLA_INTERNAL_API
#include "nsIWidget.h"

#include "compzillaIControl.h"
#include "compzillaWindow.h"

extern "C" {
#include <gdk/gdkwindow.h>
#include <X11/Xlib.h>
}


class compzillaControl
    : public compzillaIControl
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_COMPZILLAICONTROL

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

    int		composite_event, composite_error;
    int		xevie_event, xevie_error;
    int		damage_event, damage_error;
    int		xfixes_event, xfixes_error;
    int		shape_event, shape_error;
    int		render_event, render_error;
};
