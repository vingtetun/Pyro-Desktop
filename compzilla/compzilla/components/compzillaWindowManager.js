/* -*- mode: javascript; c-basic-offset: 4; indent-tabs-mode: t; -*- */

const CLSID        = Components.ID('{38054e08-223b-4eea-adcf-442c58945704}');
const CONTRACTID   = "@beatniksoftware.com/compzillaWindowManager";
const nsISupports  = Components.interfaces.nsISupports;
const nsIComponentRegistrar        = Components.interfaces.nsIComponentRegistrar;
const compzillaIWindowManager      = Components.interfaces.compzillaIWindowManager;

const jsLoader = Components.classes["@mozilla.org/moz/jssubscript-loader;1"]
                           .getService(Components.interfaces.mozIJSSubScriptLoader);

/* load the various pieces we need */
// const scriptPrefix = "chrome://compzilla/content/windowmanager/";
// jsLoader.loadSubScript (scriptPrefix + "windowStack.js");
// jsLoader.loadSubScript (scriptPrefix + "windowFrame.js");

var Atoms = {
    cache : new Object (),
    Get : function (atom_name) {
	if (this.cache[atom_name] == null)
	    this.cache[atom_name] = CompzillaState.svc.InternAtom (atom_name);
	return this.cache[atom_name];
    },

    WM_NAME : function () { return this.Get ("WM_NAME"); },
    WM_NORMAL_HINTS : function () { return this.Get ("WM_NORMAL_HINTS"); },
    _NET_WM_NAME : function () { return this.Get ("_NET_WM_NAME"); },
    _NET_WM_WINDOW_TYPE : function () { return this.Get ("_NET_WM_WINDOW_TYPE"); },
    _NET_WM_WINDOW_TYPE_DOCK : function () { return this.Get ("_NET_WM_WINDOW_TYPE_DOCK"); },
    _NET_WM_WINDOW_TYPE_MENU : function () { return this.Get ("_NET_WM_WINDOW_TYPE_MENU"); },
    _NET_WM_WINDOW_TYPE_DESKTOP : function () { return this.Get ("_NET_WM_WINDOW_TYPE_DESKTOP"); },
    _NET_WM_WINDOW_TYPE_SPLASH : function () { return this.Get ("_NET_WM_WINDOW_TYPE_SPLASH"); },
};

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
    mousePosition: new Object (),
    resizeHandle: ResizeHandle.None,
    wm: null,
    svc: null,
    windowStack: null
};

function Debug (str)
{
    if (CompzillaState.debugLog) {
        CompzillaState.debugLog.innerHTML = str + "<br>" + 
	    CompzillaState.debugLog.innerHTML;
    }
}

function WindowStack ()
{
}

WindowStack.prototype = {
    /* from EWMH:
       from bottom to top:
       * 0: windows of type _NET_WM_WINDOW_TYPE_DESKTOP
       * 1: windows having state _NET_WM_STATE_BELOW
       * 2: windows not belonging in any other layer
       * 3: windows of type _NET_WM_TYPE_DOCK (unless they have state _NET_WM_TYPE_BELOW) and
       *    windows having state _NET_WM_STATE_ABOVE
       * 4: focused windows having state _NET_WM_STATE_FULLSCREEN
       */

    layers: null,

    init : function () {
	this.layers = new Array ();
	for (var i = 0; i < 5; i ++)
	    this.layers[i] = new Array();
    },

    determineLayer : function (w) {
	if (w._net_wm_window_type == Atoms._NET_WM_WINDOW_TYPE_DESKTOP())
	    return 0;
	else if (w._net_wm_window_type == Atoms._NET_WM_WINDOW_TYPE_DOCK())
	    return 3;
	/* XXX we need cases 1 and 4 here */
	else
	    return 2;
    },

    addWindow : function (w) {
	this.removeWindow (w);
	this.addWindowToLayer (w, this.determineLayer (w));
    },

    replaceWindow : function (w1, w2) {
	if (w1.layer == undefined) {
	    this.addWindow (w2);
	}
	else {
	    w2.layer = w1.layer;
	    var idx = this.layers[w1.layer].indexOf (w1);
	    this.layers[w1.layer][idx] = w2;
	    w1.layer = undefined;
	}
    },

    addWindowToLayer : function (w, l) {
	//Debug ("adding window " + w.xid + " to layer " + l);
	this.layers[l].push (w);
	w.layer = l;
	this.restackFromLayer (l);
    },

    restackAll: function (windows) {
	init ();

	for (var i = 0; i < windows.length; i ++) {
	    var w = windows[i];
	    var layer = this.determineLayer (w);

	    addWindowToLayer (w, layer);
	}

	restackFromLayer (0);
    },

    restackWindow : function (w) {
	var before, after, start;

	if (w.layer == undefined)
	    before = -1;
	else
	    before = w.layer;

	this.removeWindow (w);

	after = this.determineLayer (w);
	this.addWindowToLayer (w, after);

	if (before = -1)
	    start = after;
	else if (before < after)
	    start = before;
	else
	    start = after;

	this.restackFromLayer (start);
    },

    restackFromLayer : function (l) {
	//Debug ("restack from layer " + l);

	var z = -1;
	if (l == 0) {
	    z = 0;
	}
	else {
	    for (var i = l-1; i >=0; i --) {
		if (this.layers[i] != null && this.layers[i].length > 0) {
		    z = this.layers [i][this.layers[i].length - 1].style.zIndex;
		    break;
		}
	    }
	    if (z == -1)
		z = 0;
	}

	for (var i = l; i < this.layers.length; i ++) {
	    for (var j = 0; j < this.layers[i].length; j ++) {
		//Debug ("setting window zIndex to " + this.layers[i][j].style.zIndex);
		this.layers[i][j].style.zIndex = z++;
	    }
	}
    },

    moveAbove: function(w, above) {
	if (above.layer == undefined)
	    this.addWindowToLayer (above, this.determineLayer (above));

	if (w.layer == undefined)
	    this.addWindowToLayer (w, this.determineLayer (w));

	if (above.layer != w.layer)
	    return; // XXX

	var above_index = this.layers[above.layer].indexOf (above);

	this.removeWindow (w);
	w.layer = above.layer; /* removeWindow clears this */
	this.layers[above.layer].splice (above_index+1, 1, w);
	this.restackFromLayer (above.layer);
    },

    removeWindow: function(w) {
	if (w.layer == undefined)
	    return;

	//Debug ("removing window from layer " + w.layer);
	var w_idx = this.layers[w.layer].indexOf (w);

	for (var i = w_idx; i < this.layers[w.layer].length-1; i ++) {
	    this.layers[w.layer][i] = this.layers[w.layer][i+1];
	}

	this.layers[w.layer].pop();

	this.restackFromLayer (w.layer);
	w.layer = undefined;
    },

    moveToBottom: function(w) {
	if (w.layer == undefined)
	    return;

	var l = w.layer;

	this.removeWindow(w);

	w.layer = l; /* removeWindow clears this */

	this.layers[l].splice(0, 1, w);

	this.restackFromLayer (l);
    },

    moveToTop: function(w) {
	if (w.layer == undefined)
	    return;

	var l = w.layer;

	this.removeWindow (w);
	this.addWindowToLayer (w, l);

	w.content.focus();
    },
};


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

	var content = this.document.createElement ("canvas");

	window.AddContentNode (content);

	content.tabIndex = "1";

	/* start out without a frame, we'll attach it in a sec */
	content.chrome = content;

	/* also start with the canvas hidden, we'll show it if we need to below */
	content.style.visibility = "hidden";

	content.xid = xid;
	this.document.body.appendChild (content);

	if (override) {
	    content.className = "override_content";
	}
	else {
	    content.className = "content";
	    this.CreateWMChrome (content);
	}

	var min_x = this.BorderSize;
	var min_y = this.BorderSize + this.TitleBarHeight + this.TitleContentGap;
	this.MoveElementTo (content.chrome,
			    min_x > x ? min_x : x,
			    min_y > y ? min_y : y);
	this.ResizeElementTo (content.chrome, width, height);

	if (mapped)
	    content.style.visibility = "visible";

	content.focus();
    },

    WindowDestroyed : function(content) {
	var chrome_root = content.chrome;
	CompzillaState.windowStack.removeWindow (chrome_root);
	this.document.body.removeChild (chrome_root);
    },

    WindowMapped : function(content) {
	var chrome_root = content.chrome;
	CompzillaState.windowStack.addWindow (content.chrome);
	content.chrome.style.visibility = "visible";
	content.style.visibility = "visible";
	//Debug ("window + " + content.chrome.xid + " mapped");
    },

    WindowUnmapped : function(content) {
	var chrome_root = content.chrome;
	CompzillaState.windowStack.removeWindow (chrome_root);
	content.chrome.style.visibility = "hidden";
	content.style.visibility = "hidden";
	//Debug ("window + " + content.chrome.xid + " unmapped");
    },

    WindowConfigured : function(content, x, y, width, height, border, above) {
	var chrome_root = content.chrome;

	Debug ("window " + content.xid + " configured to " + width + ", " + height);

	chrome_root.style.left = x;
	chrome_root.style.top = y;

	var need_relayout = false;

	if (chrome_root.offsetWidth != width) {
	    if (chrome_root == content)
		chrome_root.width = width;
	    else {
		chrome_root.style.width = width;
		need_relayout = true;
	    }
	}
	if (chrome_root.offsetHeight != height) {
	    if (chrome_root == content)
		chrome_root.height = height;
	    else {
		chrome_root.style.height = height;
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
	    if (chrome_root != content)
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
		this.DestroyWMChrome (content);
		content.className = "override_content";
	    }
	    else {
		content.className = "content";
		this.CreateWMChrome (content);
	    }

	    CompzillaState.windowStack.restackWindow (content.chrome);
	}
	else if (prop == Atoms.WM_NORMAL_HINTS()) {
	    Debug ("sizeHints.width = " + bag.getPropertyAsInt32 ("sizeHints.width"));
	    Debug ("sizeHints.height = " + bag.getPropertyAsInt32 ("sizeHints.height"));
	}
    },

    SetDocument : function(doc) {
	CompzillaState.wm = this;
	this.document = doc;

	CompzillaState.windowStack = new WindowStack ();
	CompzillaState.windowStack.init ();

	cls = Components.classes['@beatniksoftware.com/compzillaService'];
	CompzillaState.svc = cls.getService(Components.interfaces.compzillaIControl);

	// add a debug window
	this.CreateDebugWindow ();
    
	// create the overlay window
	CompzillaState.overlay = this.document.getElementById ("overlay");
	Debug ("overlay = " + (CompzillaState.overlay == null ? "null!" : "not null!"));
	this.document.onmouseup = this.DocumentMouseUp;
	this.document.onmousemove = this.DocumentMouseMove;

	this.document.addEventListener("keypress", this.DocumentKeyPressListener, true);
    },

    // ****************************************************
    // private stuff.  not accessible through the
    // compzillaIWindowManager interface

    document: null,

    DocumentKeyPressListener : {
	handleEvent : function (event) {
	    var state = CompzillaState;

	    if (event.keyCode == event.DOM_VK_F12 && event.ctrlKey) {
		// toggle the overlay
		var overlay = CompzillaState.overlay;

		if (overlay == null)
		    return;

		if (overlay.style.visibility != "visible") {
		    Debug ("overlay should be visible");

		    overlay.style.width = overlay.parentNode.clientWidth;
		    overlay.style.height = overlay.parentNode.clientHeight;
		    overlay.style.x = 0;
		    overlay.style.y = 0;

		    Debug ("overlay should be " + overlay.parentNode.style.width + " pixels wide");

		    overlay.style.visibility = "visible";
		}
		else {
		    Debug ("overlay should be hidden");
		    overlay.style.visibility = "hidden";
		}

		event.stopPropagation();
	    }
	}
    },

    DocumentMouseUp : function (event) {
	var state = CompzillaState;
	if (state.originalOpacity != undefined) {
	    state.dragWindow.style.opacity = state.originalOpacity;
	    state.originalOpacity = undefined;
	}
	state.dragWindow = null;
	state.resizeWindow = null;
	state.resizeHandle = null;
    },

    DocumentMouseMove : function (event) {
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
    },

    // ****************************************************
    // window manager frame related stuff
    //

    CreateDebugWindow : function () {
	var debugContent = this.document.createElement ("div");
	var debugClear = this.document.createElement ("button");

	debugContent.className = "debugContent";

	CompzillaState.debugLog = this.document.createElement ("div");
	CompzillaState.debugLog.className = "debugLog";

	debugClear.value = "Clear";
	debugClear.onclick = function (event) { CompzillaState.debugLog.innerHTML = ""; };

	debugContent.appendChild (CompzillaState.debugLog);
	debugContent.appendChild (debugClear);

	var debugChrome = this.CreateWMChrome (debugContent);
	this.MoveElementTo (debugChrome, 200, 50);
	this.ResizeElementTo (debugChrome, 400, 300);
	debugChrome.titlespan.innerHTML = "debug window";
	debugChrome.style.zIndex = 10000; // always on top
    },

    CreateWMChrome : function (content) {
	if (content.chrome != null && content.chrome != content)
	    return;

	Debug ("CreateWMChrome for " + content.xid);

	var root = this.document.createElement ("div");
	var titlebar = this.document.createElement ("div");
	var title = this.document.createElement ("span");

	title.innerHTML = "window title here";

	titlebar.appendChild (title);
	root.appendChild (titlebar);

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

	this.MoveElementTo (root, content.offsetLeft, content.offsetTop);
	this.ResizeElementTo (root, content.offsetWidth, content.offsetHeight); // XXX do we want this here?

	/* XXX this causes a crash for me */
	// CompzillaState.windowStack.replaceWindow (content, root);

	this.document.body.appendChild (root);

	this.HookupChromeEvents (root);
	this.LayoutChrome (root);

	return root;
    },

    DestroyWMChrome : function (content) {
	if (content.chrome != null && content.chrome == content)
	    return;

	Debug ("DestroyWMChrome for " + content.xid);

	var old_chrome = content.chrome;
	content.chrome = content;
     
	content.style.visibility = old_chrome.style.visibility;

	this.document.body.appendChild (content); // this both removes from chrome and appends to document

	this.MoveElementTo (content, old_chrome.offsetLeft, old_chrome.offsetTop);
	this.ResizeElementTo (content, old_chrome.offsetWidth, old_chrome.offsetHeight); // XXX do we want this here?

	this.document.body.removeChild (old_chrome);

	CompzillaState.windowStack.replaceWindow (old_chrome, content);
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
	element.style.left = x + "px";
	element.style.top = y + "px";

	if (element.xid != undefined) {
	    CompzillaState.svc.ConfigureWindow (element.xid,
						x, y,
						element.offsetWidth, element.offsetHeight,
						0);
	}
    },

    ResizeElementTo: function(element, w, h) {
	if (element.chrome == element) {
	    element.width = w + "px";
	    element.height = h + "px";
	}
	else {
	    element.style.width = w + "px";
	    element.style.height = h + "px";

	    this.LayoutChrome (element);
	}

	if (element.xid != undefined) {
	    CompzillaState.svc.ConfigureWindow (element.xid,
						element.offsetLeft, element.offsetTop,
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
	element.style.left = element.offsetLeft + dx;
	element.style.top = element.offsetTop + dy;

	var x, y;

	if (element.content == element) {
	    if (dw != 0)
		element.width = (element.offsetWidth + dw) + "px";
	    if (dh != 0)
		element.height = (element.offsetHeight + dh) + "px";

	    x = element.offsetLeft;
	    y = element.offsetTop;
	}
	else {
	    if (dw != 0)
		element.style.width = (element.offsetWidth + dw) + "px";
	    if (dh != 0)
		element.style.height = (element.offsetHeight + dh) + "px";

	    this.LayoutChrome (element)

		x = element.offsetLeft + element.content.offsetLeft;
	    y = element.offsetTop + element.content.offsetTop;
	}

	if (element.xid != undefined) {
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
	    return ResizeHandle.None
		},

    LayoutChrome: function(element) {
	var bs = this.BorderSize;
	var tbs = this.TitleBarHeight;
	var tcg = this.TitleContentGap;

	element.titlebar.style.left = bs;
	element.titlebar.style.top = bs;
	element.titlebar.style.width = element.offsetWidth - 2 * bs;
	element.titlebar.style.height = tbs;

	element.content.style.left = bs;
	element.content.style.top = element.titlebar.offsetTop + element.titlebar.offsetHeight + tcg;

	if (element.content.className == "debugContent") {
	    element.content.style.width = element.offsetWidth - 2 * bs;
	    element.content.style.height = element.offsetHeight - element.content.offsetTop - bs;
	}
	else {
	    element.content.width = element.offsetWidth - 2 * bs;
	    element.content.height = element.offsetHeight - element.content.offsetTop - bs;
	}
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
