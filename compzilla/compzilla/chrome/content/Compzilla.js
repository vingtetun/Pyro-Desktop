
let Cc = Components.classes;
let Ci = Components.interfaces;
let Cr = Components.results;

const NS_ERROR_NO_COMPOSITE_EXTENSION = 0x807803e9;
const NS_ERROR_NO_DAMAGE_EXTENSION = 0x807803ea;
const NS_ERROR_NO_XFIXES_EXTENSION = 0x807803eb;
const NS_ERROR_NO_SHAPE_EXTENSION = 0x807803ec;

// NOTE: This needs to be global because setting parentContentListener does not
//       take a reference.
let _compzillaContentListener = {
  onStartURIOpen: function(aURI) {
	  return false;
  },

  doContent: function(aContentType, aIsContentPreferred, aRequest, aContentHandler) {
	  throw Cr.NS_ERROR_UNEXPECTED;
  },

  isPreferred: function(aContentType, aDesiredContentType) {
	  return false;
  },

  canHandleContent: function(aContentType, aIsContentPreferred, aDesiredContentType) {
	  return false;
  },

  loadCookie: null,
  parentContentListener: null,

  QueryInterface: function (aIID) {
	  if (aIID.equals(Ci.nsIURIContentListener) ||
	      aIID.equals(Ci.nsISupportsWeakReference) ||
	      aIID.equals(Ci.nsISupports))
	    return this;
	  Components.returnCode = Cr.NS_ERROR_NO_INTERFACE;
	  return null;
  }
};


const Compzilla = {
  get _service() {
    delete this._service;
    return this._service = Cc["@pyrodesktop.org/compzillaService;1"].getService(Ci.compzillaIControl);
  },

  startup: function() {
    // FIXME: This doesn't work yet.
    //promptSetDefaultWindowManager ();

    // Resize toplevel window to screen dimensions
    var xulwin = $("#desktopWindow")[0];
    xulwin.width = screen.width;
    xulwin.height = screen.height;

    // Set defaults for the main docShell and contentListener
    this._initContent ();

    initDebugWindow ();

    let self = this;
    this._service.addObserver ({
	    windowCreate: function (win) {
	      // NOTE: Be careful to not do anything which will create a new
	      //       toplevel window, to avoid an infinite loop.
        let content = null;
	      try {
		      content = CompzillaWindowContent(win);
	      } catch (e) {
		      Debug ("Error creating content canvas: " + e);
	      }
		    if (!content)
		      return;

        let frame = null;
	      try {
		      frame = CompzillaFrame(content);
	      } catch (e) {
      		Debug ("Error creating content frame: " + e);
	      }
		    if (!frame)
		      return;

	      windowStack.stackWindow(frame);

	    if (!frame.overrideRedirect) {
  		  self._clientList.push(win.nativeWindowId);
		    self._updateClientListProperty ();
	      }
	    },

  	  windowDestroy: function (win) {
	      let idx = self._clientList.indexOf(win.nativeWindowId);
	      if (idx != -1) {
		      self._clientList.splice(idx, 1);
		      self._updateClientListProperty();
  	    }
	    },

	    rootClientMessageRecv: function (messageType, format, d1, d2, d3, d4) {
	      Debug ("Got root window clientmessage type:" + messageType + " [d1:"+d1 + ", d2:"+d2 + ", d3:"+d3 + ", d4:"+d4 + "]");

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
	    }
    });

    // Register as the window manager and generate windowcreate events for
    // existing windows.
    try {
	    this._service.RegisterWindowManager (window);
    } catch (e) {
      // FIXME: Use string bundles
    	let errXConfig = "\nPlease enable it for your X server, and try again.";
    	let errDetail;

	    switch (e.result) {
    	  case NS_ERROR_NO_COMPOSITE_EXTENSION:
	        errDetail = "The Composite extension version 0.3 or later is not available. " + errXConfig;
  	      break;
  	    case NS_ERROR_NO_DAMAGE_EXTENSION:
	        errDetail = "The Damage extension is not available. " + errXConfig;
    	    break;
	      case NS_ERROR_NO_XFIXES_EXTENSION:
	        errDetail = "The XFixes extension is not available. " + errXConfig;
	        break;
      	case NS_ERROR_NO_SHAPE_EXTENSION:
	        errDetail = "The XShape extension is not available. " + errXConfig;
    	    break;
      	default:
	        errDetail = e.message;
    	    break;
	    }
	    alert ("Cannot start Pyro: " + errDetail);
	    closeWindow (window);
      return;
    }

    let supported = [ Atoms._NET_ACTIVE_WINDOW,
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
      /*
        Atoms._NET_DESKTOP_LAYOUT,
        Atoms._NET_DESKTOP_NAMES,
        Atoms._NET_FRAME_EXTENTS,
        Atoms._NET_REQUEST_FRAME_EXTENTS,
        Atoms._NET_STARTUP_ID,
        Atoms._NET_WM_ACTION_CHANGE_DESKTOP,
        Atoms._NET_WM_ACTION_CLOSE,
        Atoms._NET_WM_ACTION_FULLSCREEN,
        Atoms._NET_WM_ACTION_MAXIMIZE_HORZ,
        Atoms._NET_WM_ACTION_MAXIMIZE_VERT,
        Atoms._NET_WM_ACTION_MINIMIZE,
        Atoms._NET_WM_ACTION_MOVE,
        Atoms._NET_WM_ACTION_RESIZE,
        Atoms._NET_WM_ACTION_SHADE,
        Atoms._NET_WM_ACTION_STICK,
        Atoms._NET_WM_ALLOWED_ACTIONS,
        Atoms._NET_WM_DESKTOP,
      */
        Atoms._NET_WM_ICON,
      //Atoms._NET_WM_ICON_GEOMETRY,
        Atoms._NET_WM_ICON_NAME,
      /* 
        Atoms._NET_WM_PID,
        Atoms._NET_WM_PING,
        Atoms._NET_WM_STATE_ABOVE,
        Atoms._NET_WM_STATE_BELOW,
        Atoms._NET_WM_STATE_DEMANDS_ATTENTION,
        Atoms._NET_WM_STATE_FULLSCREEN,
        Atoms._NET_WM_STATE_HIDDEN,
        Atoms._NET_WM_USER_TIME */
		];

    let service = this._service;
    service.SetRootWindowProperty (Atoms._NET_SUPPORTED, Atoms.XA_ATOM, supported.length, supported);
    service.SetRootWindowProperty (Atoms._NET_SHOWING_DESKTOP, Atoms.XA_CARDINAL, 1, [windowStack.showingDesktop]);
    service.SetRootWindowProperty (Atoms._NET_NUMBER_OF_DESKTOPS, Atoms.XA_CARDINAL, 1, [1]);
    service.SetRootWindowProperty (Atoms._NET_DESKTOP_GEOMETRY, Atoms.XA_CARDINAL, 2, [screen.width, screen.height]);
    service.SetRootWindowProperty (Atoms._NET_DESKTOP_VIEWPORT, Atoms.XA_CARDINAL, 2, [0, 0]);
    service.SetRootWindowProperty (Atoms._NET_CURRENT_DESKTOP, Atoms.XA_CARDINAL, 1, [0]);
    service.DeleteRootWindowProperty (Atoms._NET_VIRTUAL_ROOTS);
  },

  shutdown: function() {
  },

  _initContent: function() {
    let webNav = window.QueryInterface(Ci.nsIInterfaceRequestor)
                       .getInterface(Ci.nsIWebNavigation);

    let docShell = webNav.QueryInterface(Ci.nsIDocShell);
    docShell.useErrorPages = false;
    docShell.allowPlugins = true;
    docShell.allowJavascript = true;
    docShell.allowMetaRedirects = false;
    docShell.allowSubframes = false;
    docShell.allowAuth = false;

    // Deny all requests for new content by default, so the user's desktop won't
    // be replaced when clicking a link!
    let contentListener = webNav.QueryInterface(Ci.nsIInterfaceRequestor)
                                .getInterface(Ci.nsIURIContentListener);
    contentListener.parentContentListener = _compzillaContentListener;
  },

  _clientList: [],
  _updateClientListProperty: function() {
    this._service.SetRootWindowProperty (Atoms._NET_CLIENT_LIST, Atoms.XA_WINDOW, this._clientList.length, this._clientList);
    this._service.SetRootWindowProperty (Atoms._NET_CLIENT_LIST_STACKING, Atoms.XA_WINDOW, this._clientList.length, this._clientList);
  }
}
