/* -*- mode: javascript; c-basic-offset: 4; indent-tabs-mode: t; -*- */

const compzillaIControl = Components.interfaces.compzillaIControl;

function compzillaLoad()
{
    var xulwin = document.getElementById ("desktopWindow");
    xulwin.width = screen.width;
    xulwin.height = screen.height;

    svccls = Components.classes['@pyrodesktop.org/compzillaService'];
    svc = svccls.getService (compzillaIControl);

    svc.addEventListener (
	"windowcreate", 
	{
	    handleEvent: function (ev) {
		// NOTE: Be careful to not do anything which will create a new
		//       toplevel window, to avoid an infinite loop.

		var content = CompzillaWindowContent (ev.window);
		var frame = CompzillaFrame (content);
		frame.overrideRedirect = ev.overrideRedirect;

		windowStack.stackWindow (frame);

		// ev is a compzillaIWindowConfigureEvent
		frame.moveResize (ev.x, ev.y, ev.width, ev.height);
		if (ev.mapped) {
		    frame.show ();
		}

		if (!ev.overrideRedirect) {
		    _clientList.push (ev.window.nativeWindowId);
		    _updateClientListProperty ();
		}
	    }
        },
	true);

    svc.addEventListener (
	"windowdestroy",
	{
	    handleEvent: function (ev) {
		// NOTE: Be careful to not do anything which will create a new
		//       toplevel window, to avoid an infinite loop.

		var idx = _clientList.indexOf (ev.window.nativeWindowId);
		if (idx != -1) {
		    _clientList.splice (idx, 1);
		    _updateClientListProperty ();
		}
	    }
	},
	true);

    // Register as the window manager and generate windowcreate events for
    // existing windows.
    svc.RegisterWindowManager (window);

    // we don't really support these...

//     var supported = [ Atoms._NET_WM_NAME,
// 		      Atoms._NET_CLOSE_WINDOW,
// 		      Atoms._NET_WM_STATE,
// 		      Atoms._NET_WM_STATE_SHADED,
// 		      Atoms._NET_WM_STATE_MAXIMIZED_VERT,
// 		      Atoms._NET_WM_STATE_MAXIMIZED_HORZ,
// 		      Atoms._NET_WM_DESKTOP,
// 		      Atoms._NET_NUMBER_OF_DESKTOPS,
// 		      Atoms._NET_CURRENT_DESKTOP,
// 		      Atoms._NET_WM_WINDOW_TYPE,
// 		      Atoms._NET_WM_WINDOW_TYPE_DESKTOP,
// 		      Atoms._NET_WM_WINDOW_TYPE_DOCK,
// 		      Atoms._NET_WM_WINDOW_TYPE_TOOLBAR,
// 		      Atoms._NET_WM_WINDOW_TYPE_MENU,
// 		      Atoms._NET_WM_WINDOW_TYPE_DIALOG,
// 		      Atoms._NET_WM_WINDOW_TYPE_NORMAL,
// 		      Atoms._NET_WM_STATE_MODAL,
// 		      Atoms._NET_CLIENT_LIST,
// 		      Atoms._NET_CLIENT_LIST_STACKING,
// 		      Atoms._NET_WM_STATE_SKIP_TASKBAR,
// 		      Atoms._NET_WM_STATE_SKIP_PAGER,
// 		      Atoms._NET_WM_ICON_NAME,
// 		      Atoms._NET_WM_ICON,
// 		      Atoms._NET_WM_ICON_GEOMETRY,
// 		      Atoms._NET_WM_MOVERESIZE,
// 		      Atoms._NET_ACTIVE_WINDOW,
// 		      Atoms._NET_WM_STRUT,
// 		      Atoms._NET_WM_STATE_HIDDEN,
// 		      Atoms._NET_WM_WINDOW_TYPE_UTILITY,
// 		      Atoms._NET_WM_WINDOW_TYPE_SPLASH,
// 		      Atoms._NET_WM_STATE_FULLSCREEN,
// 		      Atoms._NET_WM_PING,
// 		      Atoms._NET_WM_PID,
// 		      Atoms._NET_WORKAREA,
// 		      Atoms._NET_SHOWING_DESKTOP,
// 		      Atoms._NET_DESKTOP_LAYOUT,
// 		      Atoms._NET_DESKTOP_NAMES,
// 		      Atoms._NET_WM_ALLOWED_ACTIONS,
// 		      Atoms._NET_WM_ACTION_MOVE,
// 		      Atoms._NET_WM_ACTION_RESIZE,
// 		      Atoms._NET_WM_ACTION_SHADE,
// 		      Atoms._NET_WM_ACTION_STICK,
// 		      Atoms._NET_WM_ACTION_MAXIMIZE_HORZ,
// 		      Atoms._NET_WM_ACTION_MAXIMIZE_VERT,
// 		      Atoms._NET_WM_ACTION_CHANGE_DESKTOP,
// 		      Atoms._NET_WM_ACTION_CLOSE,
// 		      Atoms._NET_WM_STATE_ABOVE,
// 		      Atoms._NET_WM_STATE_BELOW,
// 		      Atoms._NET_STARTUP_ID,
// 		      Atoms._NET_WM_STRUT_PARTIAL,
// 		      Atoms._NET_WM_ACTION_FULLSCREEN,
// 		      Atoms._NET_WM_ACTION_MINIMIZE,
// 		      Atoms._NET_FRAME_EXTENTS,
// 		      Atoms._NET_REQUEST_FRAME_EXTENTS,
// 		      Atoms._NET_WM_USER_TIME,
// 		      Atoms._NET_WM_STATE_DEMANDS_ATTENTION,
// 		      Atoms._NET_DESKTOP_GEOMETRY,
// 		      Atoms._NET_DESKTOP_VIEWPORT ];

//     svc.SetRootWindowArrayProperty (Atoms._NET_SUPPORTED, Atoms.XA_ATOM , supported.length, supported);
}

var _clientList = new Array ();

function _updateClientListProperty ()
{
    svc.SetRootWindowArrayProperty (Atoms._NET_CLIENT_LIST, Atoms.XA_WINDOW, _clientList.length, _clientList);
    svc.SetRootWindowArrayProperty (Atoms._NET_CLIENT_LIST_STACKING, Atoms.XA_WINDOW, _clientList.length, _clientList);
}
