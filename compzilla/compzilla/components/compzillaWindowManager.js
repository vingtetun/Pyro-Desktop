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
    _NET_WM_NAME : function () { return this.Get ("_NET_WM_NAME"); },
    _NET_WM_WINDOW_TYPE : function () { return this.Get ("_NET_WM_WINDOW_TYPE"); },
    _NET_WM_WINDOW_TYPE_DOCK : function () { return this.Get ("_NET_WM_WINDOW_TYPE_DOCK"); },
    _NET_WM_WINDOW_TYPE_MENU : function () { return this.Get ("_NET_WM_WINDOW_TYPE_MENU"); },
    _NET_WM_WINDOW_TYPE_DESKTOP : function () { return this.Get ("_NET_WM_WINDOW_TYPE_DESKTOP"); },
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
  debugContent: null,
  dragWindow: null,
  mousePosition: new Object (),
  resizeHandle: ResizeHandle.None,
  wm: null,
  svc: null
};

function Debug (str)
{
//  CompzillaState.debugContent.innerHTML += str + "<br>";
}

var WindowStack = {
   /* from EWMH:
      from bottom to top:
       * windows of type _NET_WM_TYPE_DESKTOP
       * windows having state _NET_WM_STATE_BELOW
       * windows not belonging in any other layer
       * windows of type _NET_WM_TYPE_DOCK (unless they have state _NET_WM_TYPE_BELOW) and
         windows having state _NET_WM_STATE_ABOVE
       * focused windows having state _NET_WM_STATE_FULLSCREEN
   */

   layers: new Array (),

   moveAbove: function(w, above) {
      var above_layer = this.layers.indexOf (above);

      if (above_layer == -1) {
	 // badness, but in the interest of robustness, put the window on top
	 this.moveToTop (w);
      }
      else {
	 this.removeWindow (w);
	 this.layers.splice (above_layer+1, 1, w);
	 for (var i = above_layer+1; i < this.layers.length; i ++) {
	    this.layers[i].style.zIndex = i;
	 }
      }
   },

   removeWindow: function(w) {
      var w_idx = this.layers.indexOf (w);
      if (w_idx == -1)
	 return;
      else {
	 for (var i = w_idx; i < this.layers.length-1; i ++) {
	    this.layers[i] = this.layers[i+1];
	    this.layers[i].style.zIndex = i;
	 }
	 this.layers.pop();
      }
   },

   moveToBottom: function(w) {
      this.removeWindow(w);
      this.layers.splice(0, 1, w);
      for (var i = 0; i < layers.length; i ++) {
	 layers[i].style.zIndex = i;
      }
   },

   moveToTop: function(w) {
      this.removeWindow (w);
      w.style.zIndex = this.layers.push (w);
   },
};


function CompzillaWindowManager() {}
CompzillaWindowManager.prototype =
{
  /* nsISupports */
  QueryInterface : function handler_QI(iid) {
    if (iid.equals(nsISupports))
      return this;
    if (iid.equals(compzillaIWindowManager))
      return this;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  },

  document: null,

  /* compzillaIWindowManager methods */
  WindowCreated : function(xid, override, x, y, width, height) {
     Debug ("window " + xid + " created");

     var content = this.document.createElement ("canvas");

     content.className = "content";

     /* start out without a frame, we'll attach it in a sec */
     content.style.visibility = "hidden";
     content.chrome = content;
     content.xid = xid;
     this.document.body.appendChild (content)

     if (!override)
         this.CreateWMChrome (content, xid);

     this.MoveElementTo (content.chrome, x, y);
     this.ResizeElementTo (content.chrome, width, height);

     return content;
  },

  WindowDestroyed : function(content) {
     var chrome_root = content.chrome;
     WindowStack.removeWindow (chrome_root);
     /* XXX this generates a JS exception for me, for some reason - toshok */
     this.document.body.removeChild (chrome_root);
  },

  WindowMapped : function(content) {
     var chrome_root = content.chrome;
     WindowStack.moveToTop (chrome_root);
     content.chrome.style.visibility = "visible";
     Debug ("window + " + content.chrome.xid + " mapped");
  },

  WindowUnmapped : function(content) {
     var chrome_root = content.chrome;
     WindowStack.removeWindow (chrome_root);
     content.chrome.style.visibility = "hidden";
     Debug ("window + " + content.chrome.xid + " unmapped");
  },

  WindowConfigured : function(content, x, y, width, height, border, above) {
     var chrome_root = content.chrome;

     Debug ("window " + content.xid + " configured to " + width + ", " + height);

     if (chrome_root != content) {
	width += 2 * this.BorderSize + 2 * border;
	height += 2 * this.BorderSize + 2 * border + this.TitleBarHeight + this.TitleContentGap;
     }

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

     if (need_relayout && chrome_root != content)
	this.LayoutChrome (chrome_root);

     if (above == null)
	WindowStack.moveToTop (chrome_root);
     else
	WindowStack.moveAbove (chrome_root, above.chrome);
  },

  PropertyChanged: function(content, prop) {
     var chrome_root = content.chrome;

     if (prop == Atoms.WM_NAME()) {
	if (chrome_root != content)
		chrome_root.titlespan.innerHTML = CompzillaState.svc.GetStringProperty (chrome_root.xid, Atoms.WM_NAME());
     }
     else if (prop == Atoms._NET_WM_NAME()) {
	if (chrome_root != content)
		chrome_root.titlespan.innerHTML = CompzillaState.svc.GetStringProperty (chrome_root.xid, Atoms._NET_WM_NAME());
     }
     else if (prop == Atoms._NET_WM_WINDOW_TYPE()) {
	Debug ("window type set");
	// XXX _NET_WM_WINDOW_TYPE is actually an array of atoms, not just 1.
	var type = CompzillaState.svc.GetAtomProperty (chrome_root.xid, Atoms._NET_WM_WINDOW_TYPE());
	if (type == Atoms._NET_WM_WINDOW_TYPE_DOCK() ||
	    type == Atoms._NET_WM_WINDOW_TYPE_DESKTOP()) {
		this.DestroyWMChrome (content);
	}
	else {
		this.CreateWMChrome (content);
	}
     }
  },

  SetDocument : function(doc) {
     CompzillaState.wm = this;
     this.document = doc;

     cls = Components.classes['@beatniksoftware.com/compzillaService'];
     CompzillaState.svc = cls.getService(Components.interfaces.compzillaIControl);

     // add a debug window
     CompzillaState.debugContent = this.document.createElement ("div");
     CompzillaState.debugContent.className = "content";
     CompzillaState.debugContent.style.overflow = "scroll";

     var debugChrome = this.CreateWMChrome (CompzillaState.debugContent, 0);
     this.MoveElementTo (debugChrome, 500, 50);
     this.ResizeElementTo (debugChrome, 300, 500);
     debugChrome.titlespan.innerHTML = "debug window";
     
     this.document.onmouseup = function (event) {
	var state = CompzillaState;
	state.dragWindow = null;
	state.resizeWindow = null;
	state.resizeHandle = null;
     };

     this.document.onmousemove = function (event) {
	var state = CompzillaState;
	if (state.dragWindow != null) {
           // figure out the deltas
	   var dx = event.clientX - state.mousePosition.x;
	   var dy = event.clientY - state.mousePosition.y;

	   state.mousePosition.x = event.clientX;
	   state.mousePosition.y = event.clientY;

	   state.wm.MoveElementBy (state.dragWindow, dx, dy);

           //Debug ("mousemove on window titlebar at " + state.mousePosition.x + ", " + state.mousePosition.y);
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
	      state.wm.MoveElementBy (state.resizeWindow, 0, dy);
	      state.wm.ResizeElementBy (state.resizeWindow, 0, -dy);
	      break;
	   case ResizeHandle.south:
	      state.wm.ResizeElementBy (state.resizeWindow, 0, dy);
	      break;
	   case ResizeHandle.east:
	      state.wm.ResizeElementBy (state.resizeWindow, dx, 0);
	      break;
	   case ResizeHandle.west:
	      state.wm.MoveElementBy (state.resizeWindow, dx, 0);
	      state.wm.ResizeElementBy (state.resizeWindow, -dx, 0);
	      break;
	   case ResizeHandle.northWest:
	      state.wm.MoveElementBy (state.resizeWindow, dx, dy);
	      state.wm.ResizeElementBy (state.resizeWindow, -dx, -dy);
	      break;
	   case ResizeHandle.northEast:
	      state.wm.MoveElementBy (state.resizeWindow, 0, dy);
	      state.wm.ResizeElementBy (state.resizeWindow, dx, -dy);
	      break;
	   case ResizeHandle.southWest:
	      state.wm.MoveElementBy (state.resizeWindow, dx, 0);
	      state.wm.ResizeElementBy (state.resizeWindow, -dx, dy);
	      break;
	   case ResizeHandle.southEast:
	      state.wm.ResizeElementBy (state.resizeWindow, dx, dy);
	      break;
	   }
	   event.stopPropagation ();
	}
     };
   },

// ****************************************************
// private methods.  these aren't callable through the
// compzillaIWindowManager interface

  CreateWMChrome : function (content, xid) {
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
     
     // back reference so we can find the chrome in all the other methods
     content.chrome = root;

     root.style.visibility = content.style.visibility;
     content.style.visibility = "visible";

     // a couple of convenience refs
     root.titlespan = title;
     root.titlebar = titlebar;
     root.content = content;
     root.xid = content.xid;

     try {
	     content = this.document.body.removeChild (content);
     } catch (e) { }
	
     root.appendChild (content);
     content.chrome = root;
     this.MoveElementTo (root, content.offsetLeft, content.offsetTop);
     this.ResizeElementTo (root, content.offsetWidth, content.offsetHeight); // XXX do we want this here?

     // XXX fix the stacking order

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

     // XXX fix the stacking order
  },

  BorderSize: 3,
  TitleContentGap: 3,
  TitleBarHeight: 15,
  CornerSize: 25,

  HookupChromeEvents: function(element) {
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
  },

  MoveElementBy: function(element, dx, dy) {
     element.style.left = element.offsetLeft + dx;
     element.style.top = element.offsetTop + dy;
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
  },

  ResizeElementBy: function(element, dw, dh) {
     if (element.chrome == element) {
	element.width = (element.offsetWidth + dw) + "px";
	element.height = (element.offsetHeight + dh) + "px";
     }
     else {
	element.style.width = (element.offsetWidth + dw) + "px";
	element.style.height = (element.offsetHeight + dh) + "px";

	this.LayoutChrome (element)
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
      element.content.width = element.offsetWidth - 2 * bs;
      element.content.height = element.offsetHeight - element.content.offsetTop - bs;
  },
};


var CompzillaWindowManagerModule =
{
  registerSelf : function(compMgr, fileSpec, location, type)
  {
     debug ("registerSelf");
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
     debug ("unregisterSelf");
     compMgr = compMgr.QueryInterface(nsIComponentRegistrar);

     compMgr.unregisterFactoryLocation(CLSID, fileSpec);
  },


  getClassObject: function (compMgr, cid, iid) 
  {
     debug ("getClassObject");
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
         debug("CI: " + iid + "\n");
         if (outer != null)
            throw Components.results.NS_ERROR_NO_AGGREGATION;

         return (new CompzillaWindowManager()).QueryInterface(iid);
      }
  }
};


function NSGetModule(compMgr, fileSpec) {
  return CompzillaWindowManagerModule;
}
