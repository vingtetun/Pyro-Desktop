/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#include <nsError.h>
#include <nsCOMPtr.h>
#include <nsCOMArray.h>
#include <nsRefPtrHashtable.h>
#include <nsIWidget.h>

#include "compzillaIControl.h"
#include "compzillaWindow.h"

extern "C" {
#include <gdk/gdkwindow.h>
#include <X11/Xlib.h>
}


#define NEW_ERROR(_id) NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_GENERAL,  _id)
#define NS_ERROR_NO_COMPOSITE_EXTENSTION  NEW_ERROR(1001)
#define NS_ERROR_NO_DAMAGE_EXTENSTION     NEW_ERROR(1002)
#define NS_ERROR_NO_XFIXES_EXTENSTION     NEW_ERROR(1003)
#define NS_ERROR_NO_SHAPE_EXTENSTION      NEW_ERROR(1004)


class compzillaControl
    : public compzillaIControl
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_COMPZILLAICONTROL

    compzillaControl ();
    virtual ~compzillaControl ();

private:
    already_AddRefed<compzillaWindow> FindWindow (Window win);

    void AddWindow (Window win);
    void DestroyWindow (Window win);
    void MapWindow (Window win, bool override_redirect);
    void UnmapWindow (Window win);
    void ConfigureWindow (bool isNotify,
                          Window win,
                          PRInt32 x, PRInt32 y,
                          PRInt32 width, PRInt32 height,
                          PRInt32 border,
                          Window aboveWin,
                          bool override_redirect);
    void PropertyChanged (Window win, Atom prop, bool deleted);
    void DamageWindow (Window win, XRectangle *rect);
    void ClientMessaged (Window win, Atom type, int format, long *data/*[5]*/);

    GdkWindow *GetNativeWindow(nsIDOMWindow *window);
    nsresult GetNativeWidget(nsIDOMWindow *window, nsIWidget **widget);

    nsresult InitXAtoms ();
    nsresult InitXExtensions ();
    nsresult InitOverlay ();
    nsresult InitManagerWindow ();
    nsresult InitWindowState ();

    bool ReplaceSelectionOwner (Window newOwner, Atom atom);

    void ShowOverlay (bool show);
    void EnableOverlayInput (bool receiveInput);

    void PrintEvent (XEvent *x11_event);
    GdkFilterReturn Filter (GdkXEvent *xevent, GdkEvent *event);

    static GdkFilterReturn gdk_filter_func (GdkXEvent *xevent, 
                                            GdkEvent *event, 
                                            gpointer data);
    static int ErrorHandler (Display *, XErrorEvent *);
    static int ClearErrors (Display *dpy);
    static int sErrorCnt;

    Display *mXDisplay;
    Window mXRoot;

    GdkWindow *mMainwin;
    Window mMainwinParent;
    Window mOverlay;

    Window mManagerWindow;
    bool mIsWindowManager;
    bool mIsCompositor;

    nsCOMPtr<nsIDOMWindow> mDOMWindow;
    nsRefPtrHashtable<nsUint32HashKey, compzillaWindow> mWindowMap;
    nsCOMArray<compzillaIControlObserver> mObservers;

    static int composite_event, composite_error;
    static int damage_event, damage_error;
    static int xfixes_event, xfixes_error;
    static int shape_event, shape_error;
};
