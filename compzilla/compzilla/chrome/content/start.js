/* -*- mode: javascript; c-basic-offset: 4; indent-tabs-mode: t; -*- */

var svc = Components.classes['@pyrodesktop.org/compzillaService'].getService(
    Components.interfaces.compzillaIControl);
var _clientList = new Array ();


function compzillaLoad()
{
    var xulwin = $("#desktopWindow")[0];
    xulwin.width = screen.width;
    xulwin.height = screen.height;

    svc.addObserver ({
	windowCreate: function (win) {
	    // NOTE: Be careful to not do anything which will create a new
	    //       toplevel window, to avoid an infinite loop.

	    var content = CompzillaWindowContent (win);
	    var frame = CompzillaFrame (content);

	    windowStack.stackWindow (frame);

	    if (!frame.overrideRedirect) {
		_clientList.push (win.nativeWindowId);
		_updateClientListProperty ();
	    }
	},

	windowDestroy: function (win) {
	    var idx = _clientList.indexOf (win.nativeWindowId);
	    if (idx != -1) {
		_clientList.splice (idx, 1);
		_updateClientListProperty ();
	    }
	},

	rootClientMessageRecv: function (messageType, format, d1, d2, d3, d4) {
	    Debug ("Got root window clientmessage type:" + messageType + 
		   " [d1:"+d1 + ", d2:"+d2 + ", d3:"+d3 + ", d4:"+d4 + "]");

	    switch (messageType) {
	    case Atoms._NET_ACTIVE_WINDOW:
		/* d1 == 1 for normal apps, 2 for pagers and their ilk */
		/* d2 == timestamp */
		/* d3 == XID */
		break;

	    case Atoms._NET_CURRENT_DESKTOP:
		/* d1 == desktop index */
		/* d2 == timestamp */
		break;

	    case Atoms._NET_DESKTOP_VIEWPORT:
		/* d1 == new desktop vx */
		/* d2 == new desktop vy */
		break;

	    case Atoms._NET_DESKTOP_GEOMETRY:
		/* d1 == new desktop width */
		/* d2 == new desktop height */
		break;

	    case Atoms._NET_NUMBER_OF_DESKTOPS:
		/* d1 == new number of desktops */
		break;

	    case Atoms._NET_SHOWING_DESKTOP:
		windowStack.showingDesktop = (d1 == 1);
		break;
	    }
	},
    });

    // Register as the window manager and generate windowcreate events for
    // existing windows.
    svc.RegisterWindowManager (window);

    var supported = [ Atoms._NET_ACTIVE_WINDOW,
 		      Atoms._NET_CLIENT_LIST,
 		      Atoms._NET_CLIENT_LIST_STACKING,
		      Atoms._NET_CLOSE_WINDOW,
 		      Atoms._NET_WM_MOVERESIZE,
		      Atoms._NET_WM_NAME,
		      Atoms._NET_WM_STATE,
 		      Atoms._NET_WM_STATE_MAXIMIZED_HORZ,
 		      Atoms._NET_WM_STATE_MAXIMIZED_VERT,
 		      Atoms._NET_WM_STATE_MODAL,
		      Atoms._NET_WM_STATE_SHADED,
 		      Atoms._NET_WM_STATE_SKIP_PAGER,
 		      Atoms._NET_WM_STATE_SKIP_TASKBAR,
 		      Atoms._NET_WM_STRUT,
 		      Atoms._NET_WM_STRUT_PARTIAL,
 		      Atoms._NET_WM_WINDOW_TYPE,
 		      Atoms._NET_WM_WINDOW_TYPE_DESKTOP,
 		      Atoms._NET_WM_WINDOW_TYPE_DIALOG,
 		      Atoms._NET_WM_WINDOW_TYPE_DOCK,
 		      Atoms._NET_WM_WINDOW_TYPE_MENU,
 		      Atoms._NET_WM_WINDOW_TYPE_NORMAL,
 		      Atoms._NET_WM_WINDOW_TYPE_TOOLBAR,
 		      Atoms._NET_WM_WINDOW_TYPE_SPLASH,
 		      Atoms._NET_WM_WINDOW_TYPE_UTILITY,
 		      Atoms._NET_DESKTOP_GEOMETRY,
 		      Atoms._NET_NUMBER_OF_DESKTOPS,
 		      Atoms._NET_CURRENT_DESKTOP,
 		      Atoms._NET_DESKTOP_VIEWPORT,
 		      Atoms._NET_SHOWING_DESKTOP,
 		      Atoms._NET_WORKAREA,

		      // FIXME: Implement!
// 		      Atoms._NET_DESKTOP_LAYOUT,
// 		      Atoms._NET_DESKTOP_NAMES,
// 		      Atoms._NET_FRAME_EXTENTS,
// 		      Atoms._NET_REQUEST_FRAME_EXTENTS,
// 		      Atoms._NET_STARTUP_ID,
// 		      Atoms._NET_WM_ACTION_CHANGE_DESKTOP,
// 		      Atoms._NET_WM_ACTION_CLOSE,
// 		      Atoms._NET_WM_ACTION_FULLSCREEN,
// 		      Atoms._NET_WM_ACTION_MAXIMIZE_HORZ,
// 		      Atoms._NET_WM_ACTION_MAXIMIZE_VERT,
// 		      Atoms._NET_WM_ACTION_MINIMIZE,
// 		      Atoms._NET_WM_ACTION_MOVE,
// 		      Atoms._NET_WM_ACTION_RESIZE,
// 		      Atoms._NET_WM_ACTION_SHADE,
// 		      Atoms._NET_WM_ACTION_STICK,
// 		      Atoms._NET_WM_ALLOWED_ACTIONS,
// 		      Atoms._NET_WM_DESKTOP,
// 		      Atoms._NET_WM_ICON,
// 		      Atoms._NET_WM_ICON_GEOMETRY,
// 		      Atoms._NET_WM_ICON_NAME,
// 		      Atoms._NET_WM_PID,
// 		      Atoms._NET_WM_PING,
// 		      Atoms._NET_WM_STATE_ABOVE,
// 		      Atoms._NET_WM_STATE_BELOW,
// 		      Atoms._NET_WM_STATE_DEMANDS_ATTENTION,
// 		      Atoms._NET_WM_STATE_FULLSCREEN,
// 		      Atoms._NET_WM_STATE_HIDDEN,
// 		      Atoms._NET_WM_USER_TIME,
		      ];

    svc.SetRootWindowProperty (Atoms._NET_SUPPORTED, 
			       Atoms.XA_ATOM , 
			       supported.length, 
			       supported);

    svc.SetRootWindowProperty (Atoms._NET_SHOWING_DESKTOP,
			       Atoms.XA_CARDINAL,
			       1,
			       [ windowStack.showingDesktop ]);

    svc.SetRootWindowProperty (Atoms._NET_NUMBER_OF_DESKTOPS,
			       Atoms.XA_CARDINAL,
			       1,
			       [ 1 ]);

    svc.SetRootWindowProperty (Atoms._NET_DESKTOP_GEOMETRY,
			       Atoms.XA_CARDINAL,
			       2,
			       [ screen.width, screen.height ]);

    svc.SetRootWindowProperty (Atoms._NET_DESKTOP_VIEWPORT,
			       Atoms.XA_CARDINAL,
			       2,
			       [0, 0]);

    svc.SetRootWindowProperty (Atoms._NET_CURRENT_DESKTOP,
			       Atoms.XA_CARDINAL,
			       1,
			       [ 0 ]);

    svc.DeleteRootWindowProperty (Atoms._NET_VIRTUAL_ROOTS);
}


function _updateClientListProperty ()
{
    svc.SetRootWindowProperty (Atoms._NET_CLIENT_LIST, 
			       Atoms.XA_WINDOW, 
			       _clientList.length, 
			       _clientList);
    svc.SetRootWindowProperty (Atoms._NET_CLIENT_LIST_STACKING, 
			       Atoms.XA_WINDOW, 
			       _clientList.length, 
			       _clientList);
}
