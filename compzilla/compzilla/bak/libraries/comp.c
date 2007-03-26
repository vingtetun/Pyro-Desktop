#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xdamage.h>
#include <X11/extensions/Xrender.h>

typedef struct _ignore {
    struct _ignore	*next;
    unsigned long	sequence;
} ignore;

typedef struct _win {
    struct _win		*next;
    Window		id;
#if HAS_NAME_WINDOW_PIXMAP
    Pixmap		pixmap;
#endif
    XWindowAttributes	a;
#if CAN_DO_USABLE
    Bool		usable;		    /* mapped and all damaged at one point */
    XRectangle		damage_bounds;	    /* bounds of damage */
#endif
    int			mode;
    int			damaged;
    Damage		damage;
    Picture		picture;
    XserverRegion	extents;
    unsigned long	damage_sequence;    /* sequence when damage was created */
} win;

ignore		*ignore_head, **ignore_tail = &ignore_head;

int xfixes_event, xfixes_error;
int damage_event, damage_error;
int composite_event, composite_error;
int render_event, render_error;
Bool		synchronize;
int		composite_opcode;

XserverRegion	allDamage;
win             *list;
Display		*dpy;
int		scr;
Window		root;

void
discard_ignore (Display *dpy, unsigned long sequence)
{
    while (ignore_head)
    {
	if ((long) (sequence - ignore_head->sequence) > 0)
	{
	    ignore  *next = ignore_head->next;
	    free (ignore_head);
	    ignore_head = next;
	    if (!ignore_head)
		ignore_tail = &ignore_head;
	}
	else
	    break;
    }
}

void
set_ignore (Display *dpy, unsigned long sequence)
{
    ignore  *i = malloc (sizeof (ignore));
    if (!i)
	return;
    i->sequence = sequence;
    i->next = 0;
    *ignore_tail = i;
    ignore_tail = &i->next;
}

int
should_ignore (Display *dpy, unsigned long sequence)
{
    discard_ignore (dpy, sequence);
    return ignore_head && ignore_head->sequence == sequence;
}

static void
add_damage (Display *dpy, XserverRegion damage)
{
    if (allDamage)
    {
	XFixesUnionRegion (dpy, allDamage, allDamage, damage);
	XFixesDestroyRegion (dpy, damage);
    }
    else
	allDamage = damage;
}

static void
determine_mode(Display *dpy, win *w)
{
    int mode;
    XRenderPictFormat *format;
    unsigned int default_opacity;

    /* if trans prop == -1 fall back on  previous tests*/

    if (w->a.class == InputOnly)
    {
	format = 0;
    }
    else
    {
	format = XRenderFindVisualFormat (dpy, w->a.visual);
    }

    if (w->extents)
    {
	XserverRegion damage;
	damage = XFixesCreateRegion (dpy, 0, 0);
	XFixesCopyRegion (dpy, damage, w->extents);
	add_damage (dpy, damage);
    }
}

static win *
find_win (Display *dpy, Window id)
{
    win	*w;

    for (w = list; w; w = w->next)
	if (w->id == id)
	    return w;
    return 0;
}

static void
map_win (Display *dpy, Window id, unsigned long sequence)
{
    win		*w = find_win (dpy, id);
    Drawable	back;

    if (!w)
	return;

    w->a.map_state = IsViewable;
    
#if CAN_DO_USABLE
    w->damage_bounds.x = w->damage_bounds.y = 0;
    w->damage_bounds.width = w->damage_bounds.height = 0;
#endif
    w->damaged = 0;
}

static void
add_win (Display *dpy, Window id)
{
    win				*new = malloc (sizeof (win));
    win				**p;
    
    if (!new)
	return;
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
}

static int
error (Display *dpy, XErrorEvent *ev)
{
    int	    o;
    char    *name = 0;
    
    if (should_ignore (dpy, ev->serial))
	return 0;
    
    if (ev->request_code == composite_opcode &&
	ev->minor_code == X_CompositeRedirectSubwindows)
    {
	fprintf (stderr, "Another composite manager is already running\n");
	exit (1);
    }
    
    o = ev->error_code - xfixes_error;
    switch (o) {
    case BadRegion: name = "BadRegion";	break;
    default: break;
    }
    o = ev->error_code - damage_error;
    switch (o) {
    case BadDamage: name = "BadDamage";	break;
    default: break;
    }
    o = ev->error_code - render_error;
    switch (o) {
    case BadPictFormat: name ="BadPictFormat"; break;
    case BadPicture: name ="BadPicture"; break;
    case BadPictOp: name ="BadPictOp"; break;
    case BadGlyphSet: name ="BadGlyphSet"; break;
    case BadGlyph: name ="BadGlyph"; break;
    default: break;
    }

    printf ("error %d request %d minor %d serial %d\n",
	    ev->error_code, ev->request_code, ev->minor_code, ev->serial);

/*    abort ();	    this is just annoying to most people */
    return 0;
}

int
main (int argc, char **argv)
{
    XEvent	    ev;
    XRenderPictureAttributes	pa;

    dpy = XOpenDisplay (NULL);
    if (!dpy)
    {
	fprintf (stderr, "Can't open display\n");
	exit (1);
    }
    //XSetErrorHandler (error);
    //if (synchronize)
	XSynchronize (dpy, 1);
    scr = DefaultScreen (dpy);
    root = RootWindow (dpy, scr);

    if (!XRenderQueryExtension (dpy, &render_event, &render_error))
    {
	fprintf (stderr, "No render extension\n");
	exit (1);
    }

    if (!XQueryExtension (dpy, COMPOSITE_NAME, &composite_opcode,
			  &composite_event, &composite_error))
    {
	fprintf (stderr, "No composite extension\n");
	exit (1);
    }

    if (!XDamageQueryExtension (dpy, &damage_event, &damage_error))
    {
	fprintf (stderr, "No damage extension\n");
	exit (1);
    }

    if (!XFixesQueryExtension (dpy, &xfixes_event, &xfixes_error))
    {
	fprintf (stderr, "No XFixes extension\n");
	exit (1);
    }

    XGrabServer (dpy);

    Window w = (Window)strtol (argv[1], NULL, 16);

    XCompositeRedirectWindow (dpy, w, CompositeRedirectManual);

    printf ("0x%x\n", w);

    pa.subwindow_mode = IncludeInferiors; // Don't clip child widgets

    Picture picture = XRenderCreatePicture( dpy, w,
					    XRenderFindVisualFormat (dpy,
								     DefaultVisual (dpy, scr)),
					    CPSubwindowMode, &pa );

    Pixmap pixmap = XCompositeNameWindowPixmap (dpy, w);

    printf ("made it past NameWindowPixmap!\n");

    add_win (dpy, w);
    
    Window window = XCreateSimpleWindow (dpy, root, 0, 0, 500, 500, 0, 
					 BlackPixel (dpy, scr), WhitePixel (dpy, scr));

    XMapWindow (dpy, window);

    pa.subwindow_mode = IncludeInferiors; // Don't clip child widgets

    Picture p = XRenderCreatePicture( dpy, pixmap,
				      XRenderFindVisualFormat (dpy,
							       DefaultVisual (dpy, scr)),
				      CPSubwindowMode, &pa );

    XGCValues gcvalues;
    GC gc = XCreateGC (dpy, window, 0, &gcvalues);

    XUngrabServer (dpy);

    while (1) {
      XNextEvent (dpy, &ev);

#if false
      if (ev.type == GraphicsExpose)
	continue;
#endif

      printf ("%d\n", ev.type);

      if (ev.type == damage_event + XDamageNotify) {
	printf ("DAMAGE\n");
      }

      XRenderComposite (dpy,
 			PictOpSrc,
			picture,
			None,
			p,
			0, 0,
			0, 0,
			0, 0,
			500, 500);

      XCopyArea (dpy, pixmap, window, gc, 0, 0, 500, 500, 0, 0);
    }
}
