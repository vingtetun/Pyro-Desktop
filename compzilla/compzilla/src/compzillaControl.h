/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
#include "compzillaIControl.h"
#include "nsCOMPtr.h"

#include <gdk/gdkwindow.h>

#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xdamage.h>
#include <X11/extensions/Xrender.h>

#if COMPOSITE_MAJOR > 0 || COMPOSITE_MINOR >= 2
#define HAS_NAME_WINDOW_PIXMAP 1
#endif

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

    GdkFilterReturn Filter (GdkXEvent *xevent, GdkEvent *event);
    static GdkFilterReturn gdk_filter_func (GdkXEvent *xevent, GdkEvent *event, gpointer data);

    GdkWindow *root;

    int		xfixes_event, xfixes_error;
    int		damage_event, damage_error;
    int		composite_event, composite_error;
    int		render_event, render_error;
    int		composite_opcode;

#if HAS_NAME_WINDOW_PIXMAP
    bool	hasNamePixmap;
#endif
};
