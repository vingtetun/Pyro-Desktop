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
    void DestroyWindow (nsRefPtr<compzillaWindow> win, Window xwin);
    void RootClientMessaged (Atom type, int format, long *data/*[5]*/);

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
    Window GetEventXWindow (XEvent *x11_event);

    GdkFilterReturn Filter (GdkXEvent *xevent, GdkEvent *event);

    static GdkFilterReturn gdk_filter_func (GdkXEvent *xevent, 
                                            GdkEvent *event, 
                                            gpointer data);
    static int ErrorHandler (Display *, XErrorEvent *);
    static int ClearErrors (Display *dpy);
    static int sErrorCnt;

    static PLDHashOperator CallWindowCreateCb (const PRUint32& key, 
                                               nsRefPtr<compzillaWindow>& win, 
                                               void *userdata);

    Display *mXDisplay;
    Window mXRoot;

    Window mMainwin;
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
