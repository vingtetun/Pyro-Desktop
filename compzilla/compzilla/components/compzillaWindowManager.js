/* -*- mode: javascript; c-basic-offset: 4; indent-tabs-mode: t; -*- */

const CLSID        = Components.ID('{38054e08-223b-4eea-adcf-442c58945704}');
const CONTRACTID   = "@pyrodesktop.org/compzillaWindowManager";
const nsISupports  = Components.interfaces.nsISupports;
const nsIComponentRegistrar        = Components.interfaces.nsIComponentRegistrar;
const compzillaIWindowManager      = Components.interfaces.compzillaIWindowManager;

var ResizeHandle = {
    none: -1,

    north: 0x001,
    south: 0x002,
    east:  0x004,
    west:  0x008,

    northEast: 0x005,
    northWest: 0x009,
    southEast: 0x006,
    southWest: 0x00a
};

var CompzillaState = {
    debugLog: null,
    dragWindow: null,
    mousePosition: { x: -1, y: -1 },
    resizeHandle: ResizeHandle.None,
    wm: null,
    svc: null,
    windowStack: null,
    desktop: null
};

var Atoms = null;

function Debug (str)
{
    if (CompzillaState.debugLog) {
	CompzillaState.debugLog.insertBefore (CompzillaState.wm.document.createElementNS ("http://www.w3.org/1999/xhtml", "html:br"),
					      CompzillaState.debugLog.firstChild);
	CompzillaState.debugLog.insertBefore (CompzillaState.wm.document.createTextNode (str),
					      CompzillaState.debugLog.firstChild);
    }
}

function CompzillaWindowManager() {}
CompzillaWindowManager.prototype = {
    /* nsISupports */
    QueryInterface : function handler_QI(iid) {
	if (iid.equals(nsISupports))
	    return this;
	if (iid.equals(compzillaIWindowManager))
	    return this;

	throw Components.results.NS_ERROR_NO_INTERFACE;
    },

    /* compzillaIWindowManager methods */
    WindowCreated : function(window, xid, override, x, y, width, height, mapped) {

	Debug ("creating window " + xid + ", " + (mapped ? "mapped" : "unmapped") + " " + x + "," + y + " / " + width + "x" + height);

	var content = this.document.createElementNS ("http://www.w3.org/1999/xhtml",
						     "html:canvas");

	content.tabIndex = "1";

	/* start with the canvas hidden, we'll show it if we need to below */
	content.style.visibility = "hidden";

	content.id = "X window " + xid;
	content.xid = xid;

	if (override) {
	    content.className = "override-content";
	    this.CreateOverrideWMChrome (content);
	    Debug ("Created override wm chrome.  chrome = content? " + (content == content.chrome));
	}
	else {
	    content.className = "content";
	    this.CreateWMChrome (content);
	    Debug ("Created normal wm chrome.  chrome = content? " + (content == content.chrome));
	}

	window.AddContentNode (content);

	Debug ("chrome classname = " + content.chrome.className);

	var min_x = this.BorderSize;
	var min_y = this.BorderSize + this.TitleBarHeight + this.TitleContentGap;

	this.MoveAndResizeElementTo (content.chrome,
				     min_x > x ? min_x : x,
				     min_y > y ? min_y : y,
				     width, height);

	if (mapped) {
	    CompzillaState.windowStack.stackWindow (content.chrome);
	    content.style.visibility = "visible";
	    content.chrome.style.visibility = "visible";
	    content.focus();
	}
    },

    WindowDestroyed : function(content) {
	var chrome_root = content.chrome;
	CompzillaState.windowStack.removeWindow (chrome_root);
    },

    WindowMapped : function(content) {
	Debug ("window + " + content.chrome.xid + " mapped");
	var chrome_root = content.chrome;
	Debug ("chrome_root == content? " + (chrome_root == content));
	CompzillaState.windowStack.stackWindow (chrome_root);
	chrome_root.style.visibility = "visible";
	content.style.visibility = "visible";

	content.focus();
    },

    WindowUnmapped : function(content) {
	Debug ("window + " + content.chrome.xid + " unmapped");
	var chrome_root = content.chrome;
	CompzillaState.windowStack.removeWindow (chrome_root);
	chrome_root.style.visibility = "hidden";
	content.style.visibility = "hidden";
    },

    WindowConfigured : function(content, x, y, width, height, border, above) {
	var chrome_root = content.chrome;

	Debug ("window " + content.xid + " configured to " + width + ", " + height);

	//	chrome_root.style.left = x;
	//	chrome_root.style.top = y;

	var need_relayout = false;

	if (chrome_root.offsetWidth != width) {
	    if (chrome_root == content)
		chrome_root.width = width + "px";
	    else {
		chrome_root.style.width = width + "px";
		need_relayout = true;
	    }
	}
	if (chrome_root.offsetHeight != height) {
	    if (chrome_root == content)
		chrome_root.height = height + "px";
	    else {
		chrome_root.style.height = height + "px";
		need_relayout = true;
	    }
	}

	CompzillaState.svc.SendConfigureNotify (content.xid, x, y, width, height, border);

	if (need_relayout && chrome_root != content)
	    this.LayoutChrome (chrome_root);

	if (above != null)
	    CompzillaState.windowStack.moveAbove (chrome_root, above.chrome);
    },

    PropertyChanged: function(content, prop, bag) {
	var chrome_root = content.chrome;

	if (prop == Atoms.WM_NAME() ||
	    prop == Atoms._NET_WM_NAME()) {
	    if (chrome_root.titlespan != null)
		chrome_root.titlespan.innerHTML = bag.getPropertyAsAString (".text");
	}
	else if (prop == Atoms._NET_WM_WINDOW_TYPE()) {
	    Debug ("window " + content.xid + " type set");
	    // XXX _NET_WM_WINDOW_TYPE is actually an array of atoms, not just 1.
	    var type = bag.getPropertyAsUint32 (".atom");

	    content._net_wm_window_type = type;

	    if (type == Atoms._NET_WM_WINDOW_TYPE_DOCK() ||
		type == Atoms._NET_WM_WINDOW_TYPE_DESKTOP() ||
		type == Atoms._NET_WM_WINDOW_TYPE_SPLASH()) {
		if (chrome_root.className == "window") {
		    content.className = "override-content";
		    this.DestroyWMChrome (content);
		    this.CreateOverrideWMChrome (content);
		}
	    }
	    else {
		if (chrome_root.className == "override-window") {
		    content.className = "content";
		    this.DestroyWMChrome (content);
		    this.CreateWMChrome (content);
		}
	    }

	    // need to use content.chrome here because we might have changed it above
	    CompzillaState.windowStack.stackWindow (content.chrome);
	}
	else if (prop == Atoms.WM_NORMAL_HINTS()) {
	    Debug ("sizeHints.width = " + bag.getPropertyAsInt32 ("sizeHints.width"));
	    Debug ("sizeHints.height = " + bag.getPropertyAsInt32 ("sizeHints.height"));
	}
    },

    SetDocument : function(doc) {
	CompzillaState.wm = this;
	this.document = doc;

	CompzillaState.desktop = this.document.getElementById ("desktop");

	CompzillaState.desktop.Debug = Debug;
	Atoms = CompzillaState.desktop.Atoms;

	CompzillaState.windowStack = this.document.getElementById ("compzillaWindowStack");

	cls = Components.classes['@pyrodesktop.org/compzillaService'];
	CompzillaState.svc = cls.getService(Components.interfaces.compzillaIControl);

	// create the overlay window
	CompzillaState.overlay = this.document.getElementById ("overlay-layer");
	Debug ("overlay = " + (CompzillaState.overlay == null ? "null!" : "not null!"));

 	this.document.addEventListener ("mouseup", this.DocumentMouseUpListener, true);
 	this.document.addEventListener ("mousemove", this.DocumentMouseMoveListener, true);
 	this.document.addEventListener ("keypress", this.DocumentKeyPressListener, true);
    },

    // ****************************************************
    // private stuff.  not accessible through the
    // compzillaIWindowManager interface

    document: null,

    DocumentKeyPressListener : {
	handleEvent : function (event) {
	    var state = CompzillaState;

	    Debug ("keypress, charcode=" + event.charCode + ", keycode=" + event.keyCode + ", ctrl=" + event.ctrlKey + ", alt=" + event.altKey + ", shift=" + event.shiftKey);  
	    if (event.keyCode == event.DOM_VK_F10 && event.ctrlKey) {
		CompzillaState.wm.ToggleDebugWindow ();
	    }
	    else if (event.keyCode == event.DOM_VK_F11 && event.ctrlKey) {
		// toggle the overlay
		var overlay = CompzillaState.overlay;

		if (overlay == null)
		    return;

		overlay.style.visibility = (overlay.style.visibility == "visible") ? "hidden" : "visible";

		event.stopPropagation();
	    }
	    else if (event.charCode == 100 /* 'd' */ && event.altKey) {
		CompzillaState.windowStack.toggleDesktop ();
	    }
	}
    },
    
    DocumentMouseUpListener : {
	handleEvent : function (event) {
	    var state = CompzillaState;
	    if (state.originalOpacity != undefined) {
		state.dragWindow.style.opacity = state.originalOpacity;
		state.originalOpacity = undefined;
	    }
	    state.dragWindow = null;
	    state.resizeWindow = null;
	    state.resizeHandle = ResizeHandle.None;
	}
    },

    DocumentMouseMoveListener : {
	handleEvent : function (event) {
	    var state = CompzillaState;
	    if (state.dragWindow != null) {

		if (state.originalOpacity == undefined) {
		    state.originalOpacity = state.dragWindow.style.opacity
			state.dragWindow.style.opacity = "0.8";
		}

		// figure out the deltas
		var dx = event.clientX - state.mousePosition.x;
		var dy = event.clientY - state.mousePosition.y;

		state.mousePosition.x = event.clientX;
		state.mousePosition.y = event.clientY;

		state.wm.MoveElementBy (state.dragWindow, dx, dy);

		event.stopPropagation ();
	    }
	    else if (state.resizeWindow != null && state.resizeHandle != ResizeHandle.none) {
		// figure out the deltas
		var dx = event.clientX - state.mousePosition.x;
		var dy = event.clientY - state.mousePosition.y;

		state.mousePosition.x = event.clientX;
		state.mousePosition.y = event.clientY;

		switch (state.resizeHandle) {
		case ResizeHandle.north:
		    state.wm.MoveAndResizeElementBy (state.resizeWindow,
						     0, dy,
						     0, -dy);
		    break;
		case ResizeHandle.south:
		    state.wm.MoveAndResizeElementBy (state.resizeWindow,
						     0, 0,
						     0, dy);
		    break;
		case ResizeHandle.east:
		    state.wm.MoveAndResizeElementBy (state.resizeWindow,
						     0, 0,
						     dx, 0);
		    break;
		case ResizeHandle.west:
		    state.wm.MoveAndResizeElementBy (state.resizeWindow,
						     dx, 0,
						     -dx, 0);
		    break;
		case ResizeHandle.northWest:
		    state.wm.MoveAndResizeElementBy (state.resizeWindow,
						     dx, dy,
						     -dx, -dy);
		    break;
		case ResizeHandle.northEast:
		    state.wm.MoveAndResizeElementBy (state.resizeWindow,
						     0, dy,
						     dx, -dy);
		    break;
		case ResizeHandle.southWest:
		    state.wm.MoveAndResizeElementBy (state.resizeWindow,
						     dx, 0,
						     -dx, dy);
		    break;
		case ResizeHandle.southEast:
		    state.wm.MoveAndResizeElementBy (state.resizeWindow,
						     0, 0,
						     dx, dy);
		    break;
		}
		event.stopPropagation ();
	    }
	}
    },

    // ****************************************************
    // window manager frame related stuff
    //

    ToggleDebugWindow : function () {

	if (CompzillaState.debugLog == null) {

	    var debugContent = this.document.getElementById ("debugContent");
	    if (debugContent == null)
		throw "could not find debugContent node in start.xul";

	    var debugLog;
	    var debugClear;

	    for (var el = debugContent.firstChild; el != null; el = el.nextSibling) {
		if (el.id == "debugLog")
		    debugLog = el;
		else if (el.id == "debugClear")
		    debugClear = el;
	    }

	    if (debugLog == null || debugClear == null)
		throw "invalid debugContent structure.  you must define a node with id='debugLog' and a node with id='debugClear'";

	    debugLog.content = debugContent;
	    CompzillaState.debugLog = debugLog;

	    debugClear.onclick = function (event) { CompzillaState.debugLog.innerHTML = ""; };

	    // this reparents the debugContent into debugChrome
	    var debugChrome = this.CreateWMChrome (debugContent);

	    this.MoveAndResizeElementTo (debugChrome,
					 200, 50,
					 300, 400);

	    debugChrome.titlespan.innerHTML = "debug window";
	    debugChrome.style.zIndex = 10000; // always on top
	}

	var content = CompzillaState.debugLog.content;
	if (content.style.visibility == "visible") {
	    content.chrome.style.visibility = "hidden";
	    content.style.visibility = "hidden";
	}
	else {
	    content.chrome.style.visibility = "visible";
	    content.style.visibility = "visible";
	}
    },

    CreateWMChrome : function (content) {
	if (content.chrome != null)
	    return;

	Debug ("CreateWMChrome for '" + content.id + "'");

	var root = this.document.createElementNS ("http://www.w3.org/1999/xhtml",
						  "html:div");
	var titlebar = this.document.createElementNS ("http://www.w3.org/1999/xhtml",
						      "html:div");
	var title = this.document.createElementNS ("http://www.w3.org/1999/xhtml",
						   "html:span");

	title.appendChild (this.document.createTextNode ("window title here"));

	titlebar.appendChild (title);
	root.appendChild (titlebar);

	// make sure we get button clicks in the window
	root.setAttribute ("mousethrough", "never");

	root.className = "window";
	titlebar.className = "titlebar";
	title.className = "windowtitle";
     
	// a couple of convenience refs
	root.titlespan = title;
	root.titlebar = titlebar;
	root.content = content;
	root.xid = content.xid;

	root.appendChild (content); // this both removes from document and appends to root

	// back reference so we can find the chrome in all the other methods
	content.chrome = root;

	root.style.visibility = content.style.visibility;
	//content.style.visibility = "visible";

	CompzillaState.windowStack.replaceWindow (content, root);

	this.HookupChromeEvents (root);


	// XXX this is wrong.  we should be resizing the chrome to be
	// width + 2*bordersize, and height + 2*bordersize +
	// titlebarsize, etc.
	//
	// also, ResizeElementTo calls ConfigureWindow on native
	// windows, which we don't want (i don't think..)  we need a
	// call that does an initial layout from the content
	// width/height without causing any X traffic.

	this.MoveAndResizeElementTo (root,
				     content.offsetLeft, content.offsetTop,
				     content.offsetWidth, content.offsetHeight);

	return root;
    },

    CreateOverrideWMChrome : function (content) {
	if (content.chrome != null)
	    return;

	Debug ("CreateWMChrome for '" + content.id + "'");

	var root = this.document.createElementNS ("http://www.w3.org/1999/xhtml",
						  "html:div");

	// make sure we get button clicks in the window
	root.setAttribute ("mousethrough", "never");

	root.className = "override-window";
     
	// a couple of convenience refs
	root.content = content;
	root.xid = content.xid;

	root.appendChild (content); // this both removes from document and appends to root

	// back reference so we can find the chrome in all the other methods
	content.chrome = root;

	root.style.visibility = content.style.visibility;
	//content.style.visibility = "visible";

	CompzillaState.windowStack.replaceWindow (content, root);

	// XXX this is wrong.  we should be resizing the chrome to be
	// width + 2*bordersize, and height + 2*bordersize +
	// titlebarsize, etc.
	//
	// also, ResizeElementTo calls ConfigureWindow on native
	// windows, which we don't want (i don't think..)  we need a
	// call that does an initial layout from the content
	// width/height without causing any X traffic.

	this.MoveAndResizeElementTo (root,
				     content.offsetLeft, content.offsetTop,
				     content.offsetWidth, content.offsetHeight);

	return root;
    },

    DestroyWMChrome : function (content) {
	if (content.chrome == null)
	    return;

	Debug ("DestroyWMChrome for " + content.xid);

	var old_chrome = content.chrome;
	content.chrome = null;
     
	content.style.visibility = old_chrome.style.visibility;

	CompzillaState.windowStack.replaceWindow (old_chrome, content);

	this.MoveAndResizeElementTo (content,
				     old_chrome.offsetLeft, old_chrome.offsetTop,
				     old_chrome.offsetWidth, old_chrome.offsetHeight); // XXX do we want to resize here?
    },

    BorderSize: 3,
    TitleContentGap: 3,
    TitleBarHeight: 15,
    CornerSize: 25,

    HookupChromeEvents: function(element) {
	element.addEventListener("mousedown", {
	                         handleEvent: function (event) {
				     var state = CompzillaState;
				     state.windowStack.moveToTop (element);
				     // don't stop propagation of the
				     // event, so it'll still make it to
				     // the canvas/X window
				 } },
				 true);

	element.titlebar.onmousedown = function (event) {
	    var state = CompzillaState;
	    state.dragWindow = element;
	    state.mousePosition.x = event.clientX;
	    state.mousePosition.y = event.clientY;
	    event.stopPropagation (); // need this or the window's mousedown handler below gets called
	};

	element.onmousedown = function (event) {
	    var state = CompzillaState;
	    state.mousePosition.x = event.clientX;
	    state.mousePosition.y = event.clientY;

	    var handle = state.wm.InsideResizeHandle (element);
	    if (handle != ResizeHandle.None) {
		state.resizeWindow = element;
		state.resizeHandle = handle;
	    }
	};
    },

    MoveElementTo: function(element, x, y) {
	this.MoveAndResizeElementTo (element,
				     x, y,
				     element.offsetWidth, element.offsetHeight);
    },

    ResizeElementTo: function(element, w, h) {
	this.MoveAndResizeElementTo (element,
				     element.offsetLeft, element.offsetTop,
				     w, h);
    },

    MoveAndResizeElementTo: function (element, x, y, w, h) {
	if (x != element.offsetLeft)
	    element.style.left = x + "px";
	if (y != element.offsetTop)
	    element.style.top = y + "px";

	var need_relayout = false;
	if (w != element.offsetWidth || h != element.offsetHeight)
	    need_relayout = true;

	if (w != element.offsetWidth)
	    element.style.width = w + "px";
	if (h != element.offsetHeight)
	    element.style.height = h + "px";

	this.LayoutChrome (element);

	if (element.xid != undefined && w > 0 && h > 0) {
	    Debug ("Configuring X window " + element.xid);
	    CompzillaState.svc.ConfigureWindow (element.xid,
						x, y,
						w, h,
						0);
	}
    },

    ResizeElementBy: function(element, dw, dh) {
	this.MoveAndResizeElementBy (element, 0, 0, dw, dh);
    },

    MoveElementBy: function(element, dx, dy) {
	this.MoveAndResizeElementBy (element, dx, dy, 0, 0);
    },

    MoveAndResizeElementBy: function (element, dx, dy, dw, dh) {
	element.style.left = (element.offsetLeft + dx) + "px";
	element.style.top = (element.offsetTop + dy) + "px";

	var x, y;

	var need_relayout = false;
	if (dw != 0 || dh != 0)
	    need_relayout = true;

	if (dw != 0)
	    element.style.width = (element.offsetWidth + dw) + "px";
	if (dh != 0)
	    element.style.height = (element.offsetHeight + dh) + "px";

	if (need_relayout)
	    this.LayoutChrome (element);

	x = element.offsetLeft + element.content.offsetLeft;
	y = element.offsetTop + element.content.offsetTop;

	if (element.xid != undefined && element.offsetWidth > 0 && element.offsetHeight > 0) {
	    CompzillaState.svc.ConfigureWindow (element.xid,
						x, y,
						element.offsetWidth, element.offsetHeight,
						0);
	}
    },

    InsideResizeHandle: function(element) {
	var state = CompzillaState;

	var bs = this.BorderSize;
	var cs = this.CornerSize;

	var mx = state.mousePosition.x - element.offsetLeft;
	var my = state.mousePosition.y - element.offsetTop;

	if (mx < bs) {
	    // left side
	    if (my < cs)
		return ResizeHandle.northWest;
	    else if (my > element.offsetHeight - cs)
		return ResizeHandle.southWest;
	    else
		return ResizeHandle.west;
	}
	else if (mx > element.offsetWidth - bs) {
	    // right side
	    if (my < cs)
		return ResizeHandle.northEast;
	    else if (my > element.offsetHeight - cs)
		return ResizeHandle.southEast;
	    else
		return ResizeHandle.east;
	}
	else if (my < bs) {
	    // top side
	    return ResizeHandle.north;
	}
	else if (my > element.offsetHeight - bs) {
	    // bottom side
	    return ResizeHandle.south;
	}
	else
	    return ResizeHandle.None;
    },

    LayoutChrome: function(element) {

	if (element.className == "window") {
	    var bs = this.BorderSize;
	    var tbs = this.TitleBarHeight;
	    var tcg = this.TitleContentGap;

	    element.titlebar.style.left = bs + "px";
	    element.titlebar.style.top = bs + "px";
	    element.titlebar.style.width = (element.offsetWidth - 2 * bs) + "px";
	    element.titlebar.style.height = tbs + "px";

	    element.content.style.left = bs + "px";
	    element.content.style.top = (element.titlebar.offsetTop + element.titlebar.offsetHeight + tcg) + "px";

	    if (element.content.className == "content"
		|| element.content.className == "override-content") {
		Debug ("laying out using content.width");
		element.content.width = (element.offsetWidth - 2 * bs) + "px";
		element.content.height = (element.offsetHeight - element.content.offsetTop - bs) + "px";
	    }
	    else {
		Debug ("laying out using content.style.width");
		element.content.style.width = (element.offsetWidth - 2 * bs) + "px";
		element.content.style.height = (element.offsetHeight - element.content.offsetTop - bs) + "px";
	    }
	}
	else if (element.className == "override-window") {
	    Debug ("override laying out using content.width");
	    element.content.width = element.offsetWidth + "px";
	    element.content.height = element.offsetHeight + "px";
	}
	else
	    throw "unknown chrome classname " + element.className;
    },
};


var CompzillaWindowManagerModule = {
    registerSelf : function(compMgr, fileSpec, location, type)
    {
	compMgr = compMgr.QueryInterface(nsIComponentRegistrar);

	compMgr.registerFactoryLocation(CLSID,
					"Compzilla Window Manager Service",
					CONTRACTID,
					fileSpec,
					location,
					type);
    },

    unregisterSelf : function(compMgr, fileSpec, location)
    {
	compMgr = compMgr.QueryInterface(nsIComponentRegistrar);

	compMgr.unregisterFactoryLocation(CLSID, fileSpec);
    },

    getClassObject: function (compMgr, cid, iid) 
    {
	if (!cid.equals(CLSID))
	    throw Components.results.NS_ERROR_NO_INTERFACE;

	if (!iid.equals(Components.interfaces.nsIFactory))
	    throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

	return this.myFactory;
    },

    myFactory: {
	/*
	 * Construct an instance of the interface specified by iid, possibly
	 * aggregating it with the provided outer.  (If you don't know what
	 * aggregation is all about, you don't need to.  It reduces even the
	 * mightiest of XPCOM warriors to snivelling cowards.)
	 */

	createInstance: function (outer, iid) {
	    if (outer != null)
		throw Components.results.NS_ERROR_NO_AGGREGATION;

	    return (new CompzillaWindowManager()).QueryInterface(iid);
	}
    }
};


function NSGetModule(compMgr, fileSpec) {
    return CompzillaWindowManagerModule;
}
