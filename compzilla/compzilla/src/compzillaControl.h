/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#include "nsCOMPtr.h"
#include "nsClassHashtable.h"
#include "nsHashKeys.h"

#include <gdk/gdkwindow.h>

#include <X11/Xlib.h>
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
    CompositedWindow();
    ~CompositedWindow();
    
    nsCOMPtr<nsISupports> mContent;
    GdkWindow *mNativeWindow;
    Damage mDamage;
    Picture mPicture;
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
    void AddWindow (Window id, Window prev);

    GdkWindow *GetNativeWindow(nsIDOMWindow *window);

    GdkFilterReturn Filter (GdkXEvent *xevent, GdkEvent *event);
    static GdkFilterReturn gdk_filter_func (GdkXEvent *xevent, GdkEvent *event, gpointer data);

    GdkWindow *root;
    GdkWindow *mainwin;
    nsCOMPtr<compzillaIWindowManager> wm;
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
