/* -*- mode: javascript; c-basic-offset: 4; indent-tabs-mode: t; -*- */


Atoms = {
    svc: null,
    cache : new Object (),

    _intern : function (atom_name) {
        if (this.svc == null) {
            cls = Components.classes['@pyrodesktop.org/compzillaService'];
            this.svc = cls.getService(Components.interfaces.compzillaIControl);
        }

        if (this.cache[atom_name] == null)
            this.cache[atom_name] = this.svc.InternAtom (atom_name);

        return this.cache[atom_name];
    },

    /* constant atoms */
    get XA_ATOM () { return 4; },
    get XA_WINDOW () { return 33; },

    get WM_CLASS () { return this._intern ("WM_CLASS"); },
    get WM_NAME () { return this._intern ("WM_NAME"); },
    get WM_NORMAL_HINTS () { return this._intern ("WM_NORMAL_HINTS"); },
    get WM_TRANSIENT_FOR () { return this._intern ("WM_TRANSIENT_FOR"); },

    get _NET_ACTIVE_WINDOW () { return this._intern ("_NET_ACTIVE_WINDOW"); },
    get _NET_CLIENT_LIST () { return this._intern ("_NET_CLIENT_LIST"); },
    get _NET_CLIENT_LIST_STACKING () { return this._intern ("_NET_CLIENT_LIST_STACKING"); },
    get _NET_CLIENT_LIST_STACKING () { return this._intern ("_NET_CLIENT_LIST_STACKING"); },
    get _NET_CLOSE_WINDOW () { return this._intern ("_NET_CLOSE_WINDOW"); },
    get _NET_CURRENT_DESKTOP () { return this._intern ("_NET_CURRENT_DESKTOP"); },
    get _NET_DESKTOP_GEOMETRY () { return this._intern ("_NET_DESKTOP_GEOMETRY"); },
    get _NET_DESKTOP_LAYOUT () { return this._intern ("_NET_DESKTOP_LAYOUT"); },
    get _NET_DESKTOP_NAMES () { return this._intern ("_NET_DESKTOP_NAMES"); },
    get _NET_DESKTOP_VIEWPORT () { return this._intern ("_NET_DESKTOP_VIEWPORT"); },
    get _NET_FRAME_EXTENTS () { return this._intern ("_NET_FRAME_EXTENTS"); },
    get _NET_NUMBER_OF_DESKTOPS () { return this._intern ("_NET_NUMBER_OF_DESKTOPS"); },
    get _NET_REQUEST_FRAME_EXTENTS () { return this._intern ("_NET_REQUEST_FRAME_EXTENTS"); },
    get _NET_SHOWING_DESKTOP () { return this._intern ("_NET_SHOWING_DESKTOP"); },
    get _NET_STARTUP_ID () { return this._intern ("_NET_STARTUP_ID"); },
    get _NET_SUPPORTED () { return this._intern ("_NET_SUPPORTED"); },
    get _NET_WM_ACTION_CHANGE_DESKTOP () { return this._intern ("_NET_WM_ACTION_CHANGE_DESKTOP"); },
    get _NET_WM_ACTION_CLOSE () { return this._intern ("_NET_WM_ACTION_CLOSE"); },
    get _NET_WM_ACTION_FULLSCREEN () { return this._intern ("_NET_WM_ACTION_FULLSCREEN"); },
    get _NET_WM_ACTION_MAXIMIZE_HORZ () { return this._intern ("_NET_WM_ACTION_MAXIMIZE_HORZ"); },
    get _NET_WM_ACTION_MAXIMIZE_VERT () { return this._intern ("_NET_WM_ACTION_MAXIMIZE_VERT"); },
    get _NET_WM_ACTION_MINIMIZE () { return this._intern ("_NET_WM_ACTION_MINIMIZE"); },
    get _NET_WM_ACTION_MOVE () { return this._intern ("_NET_WM_ACTION_MOVE"); },
    get _NET_WM_ACTION_RESIZE () { return this._intern ("_NET_WM_ACTION_RESIZE"); },
    get _NET_WM_ACTION_SHADE () { return this._intern ("_NET_WM_ACTION_SHADE"); },
    get _NET_WM_ACTION_STICK () { return this._intern ("_NET_WM_ACTION_STICK"); },
    get _NET_WM_ALLOWED_ACTIONS () { return this._intern ("_NET_WM_ALLOWED_ACTIONS"); },
    get _NET_WM_DESKTOP () { return this._intern ("_NET_WM_DESKTOP"); },
    get _NET_WM_ICON_GEOMETRY () { return this._intern ("_NET_WM_ICON_GEOMETRY"); },
    get _NET_WM_ICON_NAME () { return this._intern ("_NET_WM_ICON_NAME"); },
    get _NET_WM_ICON () { return this._intern ("_NET_WM_ICON"); },
    get _NET_WM_MOVERESIZE () { return this._intern ("_NET_WM_MOVERESIZE"); },
    get _NET_WM_NAME () { return this._intern ("_NET_WM_NAME"); },
    get _NET_WM_PID () { return this._intern ("_NET_WM_PID"); },
    get _NET_WM_PING () { return this._intern ("_NET_WM_PING"); },
    get _NET_WM_STATE_ABOVE () { return this._intern ("_NET_WM_STATE_ABOVE"); },
    get _NET_WM_STATE_BELOW () { return this._intern ("_NET_WM_STATE_BELOW"); },
    get _NET_WM_STATE_DEMANDS_ATTENTION () { return this._intern ("_NET_WM_STATE_DEMANDS_ATTENTION"); },
    get _NET_WM_STATE_FULLSCREEN () { return this._intern ("_NET_WM_STATE_FULLSCREEN"); },
    get _NET_WM_STATE_HIDDEN () { return this._intern ("_NET_WM_STATE_HIDDEN"); },
    get _NET_WM_STATE_MAXIMIZED_HORZ () { return this._intern ("_NET_WM_STATE_MAXIMIZED_HORZ"); },
    get _NET_WM_STATE_MAXIMIZED_VERT () { return this._intern ("_NET_WM_STATE_MAXIMIZED_VERT"); },
    get _NET_WM_STATE_MODAL () { return this._intern ("_NET_WM_STATE_MODAL"); },
    get _NET_WM_STATE () { return this._intern ("_NET_WM_STATE"); },
    get _NET_WM_STATE_SHADED () { return this._intern ("_NET_WM_STATE_SHADED"); },
    get _NET_WM_STATE_SKIP_PAGER () { return this._intern ("_NET_WM_STATE_SKIP_PAGER"); },
    get _NET_WM_STATE_SKIP_TASKBAR () { return this._intern ("_NET_WM_STATE_SKIP_TASKBAR"); },
    get _NET_WM_STRUT_PARTIAL () { return this._intern ("_NET_WM_STRUT_PARTIAL"); },
    get _NET_WM_STRUT () { return this._intern ("_NET_WM_STRUT"); },
    get _NET_WM_USER_TIME () { return this._intern ("_NET_WM_USER_TIME"); },
    get _NET_WM_WINDOW_TYPE_DESKTOP () { return this._intern ("_NET_WM_WINDOW_TYPE_DESKTOP"); },
    get _NET_WM_WINDOW_TYPE_DIALOG () { return this._intern ("_NET_WM_WINDOW_TYPE_DIALOG"); },
    get _NET_WM_WINDOW_TYPE_DOCK () { return this._intern ("_NET_WM_WINDOW_TYPE_DOCK"); },
    get _NET_WM_WINDOW_TYPE_MENU () { return this._intern ("_NET_WM_WINDOW_TYPE_MENU"); },
    get _NET_WM_WINDOW_TYPE_NORMAL () { return this._intern ("_NET_WM_WINDOW_TYPE_NORMAL"); },
    get _NET_WM_WINDOW_TYPE_NORMAL () { return this._intern ("_NET_WM_WINDOW_TYPE_NORMAL"); },
    get _NET_WM_WINDOW_TYPE () { return this._intern ("_NET_WM_WINDOW_TYPE"); },
    get _NET_WM_WINDOW_TYPE_SPLASH () { return this._intern ("_NET_WM_WINDOW_TYPE_SPLASH"); },
    get _NET_WM_WINDOW_TYPE_SPLASH () { return this._intern ("_NET_WM_WINDOW_TYPE_SPLASH"); },
    get _NET_WM_WINDOW_TYPE_TOOLBAR () { return this._intern ("_NET_WM_WINDOW_TYPE_TOOLBAR"); },
    get _NET_WM_WINDOW_TYPE_UTILITY () { return this._intern ("_NET_WM_WINDOW_TYPE_UTILITY"); },
    get _NET_WORKAREA () { return this._intern ("_NET_WORKAREA"); },
}
