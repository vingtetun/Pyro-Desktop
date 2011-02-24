/* -*- mode: javascript; c-basic-offset: 4; indent-tabs-mode: t; -*- */


var Atoms = {
    svc: Components.classes['@pyrodesktop.org/compzillaService;1'].getService(
	     Components.interfaces.compzillaIControl),

    Intern: function (atom_name) {
	var cache = null;
	this.__defineGetter__ (atom_name,
			       function () {
				   if (cache == null) {
				       cache = this.svc.InternAtom (atom_name);
				   }
				   return cache;
			       });
    },

    /* constant atoms */
    get XA_PRIMARY ()             { return 1; },
    get XA_SECONDARY ()           { return 2; },
    get XA_ARC ()                 { return 3; },
    get XA_ATOM ()                { return 4; },
    get XA_BITMAP ()              { return 5; },
    get XA_CARDINAL ()            { return 6; },
    get XA_COLORMAP ()            { return 7; },
    get XA_CURSOR ()              { return 8; },
    get XA_CUT_BUFFER0 ()         { return 9; },
    get XA_CUT_BUFFER1 ()         { return 10; },
    get XA_CUT_BUFFER2 ()         { return 11; },
    get XA_CUT_BUFFER3 ()         { return 12; },
    get XA_CUT_BUFFER4 ()         { return 13; },
    get XA_CUT_BUFFER5 ()         { return 14; },
    get XA_CUT_BUFFER6 ()         { return 15; },
    get XA_CUT_BUFFER7 ()         { return 16; },
    get XA_DRAWABLE ()            { return 17; },
    get XA_FONT ()                { return 18; },
    get XA_INTEGER ()             { return 19; },
    get XA_PIXMAP ()              { return 20; },
    get XA_POINT ()               { return 21; },
    get XA_RECTANGLE ()           { return 22; },
    get XA_RESOURCE_MANAGER ()    { return 23; },
    get XA_RGB_COLOR_MAP ()       { return 24; },
    get XA_RGB_BEST_MAP ()        { return 25; },
    get XA_RGB_BLUE_MAP ()        { return 26; },
    get XA_RGB_DEFAULT_MAP ()     { return 27; },
    get XA_RGB_GRAY_MAP ()        { return 28; },
    get XA_RGB_GREEN_MAP ()       { return 29; },
    get XA_RGB_RED_MAP ()         { return 30; },
    get XA_STRING ()              { return 31; },
    get XA_VISUALID ()            { return 32; },
    get XA_WINDOW ()              { return 33; },
    get XA_WM_COMMAND ()          { return 34; },
    get XA_WM_HINTS ()            { return 35; },
    get XA_WM_CLIENT_MACHINE ()   { return 36; },
    get XA_WM_ICON_NAME ()        { return 37; },
    get XA_WM_ICON_SIZE ()        { return 38; },
    get XA_WM_NAME ()             { return 39; },
    get XA_WM_NORMAL_HINTS ()     { return 40; },
    get XA_WM_SIZE_HINTS ()       { return 41; },
    get XA_WM_ZOOM_HINTS ()       { return 42; },
    get XA_MIN_SPACE ()           { return 43; },
    get XA_NORM_SPACE ()          { return 44; },
    get XA_MAX_SPACE ()           { return 45; },
    get XA_END_SPACE ()           { return 46; },
    get XA_SUPERSCRIPT_X ()       { return 47; },
    get XA_SUPERSCRIPT_Y ()       { return 48; },
    get XA_SUBSCRIPT_X ()         { return 49; },
    get XA_SUBSCRIPT_Y ()         { return 50; },
    get XA_UNDERLINE_POSITION ()  { return 51; },
    get XA_UNDERLINE_THICKNESS () { return 52; },
    get XA_STRIKEOUT_ASCENT ()    { return 53; },
    get XA_STRIKEOUT_DESCENT ()   { return 54; },
    get XA_ITALIC_ANGLE ()        { return 55; },
    get XA_X_HEIGHT ()            { return 56; },
    get XA_QUAD_WIDTH ()          { return 57; },
    get XA_WEIGHT ()              { return 58; },
    get XA_POINT_SIZE ()          { return 59; },
    get XA_RESOLUTION ()          { return 60; },
    get XA_COPYRIGHT ()           { return 61; },
    get XA_NOTICE ()              { return 62; },
    get XA_FONT_NAME ()           { return 63; },
    get XA_FAMILY_NAME ()         { return 64; },
    get XA_FULL_NAME ()           { return 65; },
    get XA_CAP_HEIGHT ()          { return 66; },
    get XA_WM_CLASS ()            { return 67; },
    get XA_WM_TRANSIENT_FOR ()    { return 68; },

    /* these aren't really atoms, they're #defines mentioned in EWMH-1.3 */
    get _NET_WM_MOVERESIZE_SIZE_TOPLEFT ()     { return 0; },
    get _NET_WM_MOVERESIZE_SIZE_TOP ()         { return 1; },
    get _NET_WM_MOVERESIZE_SIZE_TOPRIGHT ()    { return 2; },
    get _NET_WM_MOVERESIZE_SIZE_RIGHT ()       { return 3; },
    get _NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT () { return 4; },
    get _NET_WM_MOVERESIZE_SIZE_BOTTOM ()      { return 5; },
    get _NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT ()  { return 6; },
    get _NET_WM_MOVERESIZE_SIZE_LEFT ()        { return 7; },
    get _NET_WM_MOVERESIZE_MOVE ()             { return 8; },  /* movement only */
    get _NET_WM_MOVERESIZE_SIZE_KEYBOARD ()    { return 9; },  /* size via keyboard */
    get _NET_WM_MOVERESIZE_MOVE_KEYBOARD ()    { return 10; }, /* move via keyboard */
    get _NET_WM_MOVERESIZE_CANCEL ()           { return 11; }, /* cancel operation */

    /* state change operations */
    get _NET_WM_STATE_REMOVE ()                { return 0; },  /* remove/unset property */
    get _NET_WM_STATE_ADD ()                   { return 1; },  /* add/set property */
    get _NET_WM_STATE_TOGGLE ()                { return 2; },  /* toggle property  */
};


Atoms.Intern ("_NET_ACTIVE_WINDOW");
Atoms.Intern ("_NET_CLIENT_LIST");
Atoms.Intern ("_NET_CLIENT_LIST_STACKING");
Atoms.Intern ("_NET_CLOSE_WINDOW");
Atoms.Intern ("_NET_CURRENT_DESKTOP");
Atoms.Intern ("_NET_DESKTOP_GEOMETRY");
Atoms.Intern ("_NET_DESKTOP_LAYOUT");
Atoms.Intern ("_NET_DESKTOP_NAMES");
Atoms.Intern ("_NET_DESKTOP_VIEWPORT");
Atoms.Intern ("_NET_FRAME_EXTENTS");
Atoms.Intern ("_NET_MOVERESIZE_WINDOW");
Atoms.Intern ("_NET_NUMBER_OF_DESKTOPS");
Atoms.Intern ("_NET_REQUEST_FRAME_EXTENTS");
Atoms.Intern ("_NET_RESTACK_WINDOW");
Atoms.Intern ("_NET_SHOWING_DESKTOP");
Atoms.Intern ("_NET_STARTUP_ID");
Atoms.Intern ("_NET_SUPPORTED");
Atoms.Intern ("_NET_VIRTUAL_ROOTS");
Atoms.Intern ("_NET_WORKAREA");

Atoms.Intern ("_NET_WM_ACTION_CHANGE_DESKTOP");
Atoms.Intern ("_NET_WM_ACTION_CLOSE");
Atoms.Intern ("_NET_WM_ACTION_FULLSCREEN");
Atoms.Intern ("_NET_WM_ACTION_MAXIMIZE_HORZ");
Atoms.Intern ("_NET_WM_ACTION_MAXIMIZE_VERT");
Atoms.Intern ("_NET_WM_ACTION_MINIMIZE");
Atoms.Intern ("_NET_WM_ACTION_MOVE");
Atoms.Intern ("_NET_WM_ACTION_RESIZE");
Atoms.Intern ("_NET_WM_ACTION_SHADE");
Atoms.Intern ("_NET_WM_ACTION_STICK");
Atoms.Intern ("_NET_WM_ALLOWED_ACTIONS");

Atoms.Intern ("_NET_WM_DESKTOP");
Atoms.Intern ("_NET_WM_ICON_GEOMETRY");
Atoms.Intern ("_NET_WM_ICON_NAME");
Atoms.Intern ("_NET_WM_ICON");
Atoms.Intern ("_NET_WM_MOVERESIZE");
Atoms.Intern ("_NET_WM_NAME");
Atoms.Intern ("_NET_WM_PID");
Atoms.Intern ("_NET_WM_PING");

Atoms.Intern ("_NET_WM_STATE");
Atoms.Intern ("_NET_WM_STATE_ABOVE");
Atoms.Intern ("_NET_WM_STATE_BELOW");
Atoms.Intern ("_NET_WM_STATE_DEMANDS_ATTENTION");
Atoms.Intern ("_NET_WM_STATE_FULLSCREEN");
Atoms.Intern ("_NET_WM_STATE_HIDDEN");
Atoms.Intern ("_NET_WM_STATE_MAXIMIZED_HORZ");
Atoms.Intern ("_NET_WM_STATE_MAXIMIZED_VERT");
Atoms.Intern ("_NET_WM_STATE_MODAL");
Atoms.Intern ("_NET_WM_STATE_SHADED");
Atoms.Intern ("_NET_WM_STATE_SKIP_PAGER");
Atoms.Intern ("_NET_WM_STATE_SKIP_TASKBAR");

Atoms.Intern ("_NET_WM_STRUT_PARTIAL");
Atoms.Intern ("_NET_WM_STRUT");

Atoms.Intern ("_NET_WM_USER_TIME");

Atoms.Intern ("_NET_WM_WINDOW_TYPE");
Atoms.Intern ("_NET_WM_WINDOW_TYPE_DESKTOP");
Atoms.Intern ("_NET_WM_WINDOW_TYPE_DIALOG");
Atoms.Intern ("_NET_WM_WINDOW_TYPE_DOCK");
Atoms.Intern ("_NET_WM_WINDOW_TYPE_MENU");
Atoms.Intern ("_NET_WM_WINDOW_TYPE_NORMAL");
Atoms.Intern ("_NET_WM_WINDOW_TYPE_SPLASH");
Atoms.Intern ("_NET_WM_WINDOW_TYPE_TOOLBAR");
Atoms.Intern ("_NET_WM_WINDOW_TYPE_UTILITY");
Atoms.Intern ("_NET_WM_WINDOW_TYPE_DROPDOWN_MENU");
Atoms.Intern ("_NET_WM_WINDOW_TYPE_POPUP_MENU");
Atoms.Intern ("_NET_WM_WINDOW_TYPE_TOOLTIP");
Atoms.Intern ("_NET_WM_WINDOW_TYPE_NOTIFICATION");
Atoms.Intern ("_NET_WM_WINDOW_TYPE_COMBO");
Atoms.Intern ("_NET_WM_WINDOW_TYPE_DND");
