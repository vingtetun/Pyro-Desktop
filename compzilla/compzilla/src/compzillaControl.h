/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#include "nsCOMPtr.h"

#define MOZILLA_INTERNAL_API
#include "nsClassHashtable.h"
#undef MOZILLA_INTERNAL_API

#include <gdk/gdkwindow.h>

#include <X11/Xlib.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xdamage.h>
#include <X11/extensions/Xrender.h>

#include "compzillaIControl.h"


#if COMPOSITE_MAJOR > 0 || COMPOSITE_MINOR >= 2
#define HAS_NAME_WINDOW_PIXMAP 1
#endif


class CompositedWindow
{
public:
    CompositedWindow(Display *display, Window window);
    ~CompositedWindow();
    
    nsCOMPtr<nsISupports> mContent;
    Window mWindow;
    Display *mDisplay;
    XWindowAttributes mAttr;
    Damage mDamage;
    Picture mPicture;
    Pixmap mPixmap;
    Region mClip;
    CompositedWindow *mFrame;
};


class compzillaControl
    : public compzillaIControl
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_COMPZILLAICONTROL

    compzillaControl();
    virtual ~compzillaControl();

private:
    CompositedWindow *FindWindow (Window win);
    CompositedWindow *FindToplevelWindow (Window win);

    void AddWindow (Window win);
    void DestroyWindow (Window win);
    void ForgetWindow (Window win);
    void MapWindow (Window win);
    void UnmapWindow (Window win);

    GdkWindow *GetNativeWindow(nsIDOMWindow *window);

    void Render (nsCOMPtr<nsISupports> content, 
                 int x, int y, 
                 int width, int height, 
                 const char *dataBuf, 
                 int len);

    void ShowOutputWindow();
    void HideOutputWindow();
    void PaintScreen();
    void GetDamage(Region region);

    GdkFilterReturn Filter (GdkXEvent *xevent, GdkEvent *event);
    static GdkFilterReturn gdk_filter_func (GdkXEvent *xevent, GdkEvent *event, gpointer data);

    static PLDHashOperator FindWindowForFrame (const PRUint32& key,
                                               CompositedWindow *entry, 
                                               void *userData);

    static PLDHashOperator AddWindowDamage (const PRUint32& key,
                                            CompositedWindow *entry, 
                                            void *userData);

    static int ErrorHandler (Display *, XErrorEvent *);

    GdkDisplay *mDisplay;
    Display *mXDisplay;

    GdkWindow *mRoot;
    GdkWindow *mMainwin;
    Window mMainwinParent;
    Window mOverlay;

    static Region sEmptyRegion;

    nsCOMPtr<nsIDOMWindow> mDOMWindow;
    nsCOMPtr<compzillaIWindowManager> mWM;
    nsClassHashtable<nsUint32HashKey, CompositedWindow> mWindowMap;

    int		xfixes_event, xfixes_error;
    int		damage_event, damage_error;
    int		composite_event, composite_error;
    int		render_event, render_error;
    int		composite_opcode;

#if HAS_NAME_WINDOW_PIXMAP
    bool	hasNamePixmap;
#endif
};
