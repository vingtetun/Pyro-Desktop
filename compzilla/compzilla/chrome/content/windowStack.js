/* -*- mode: javascript; c-basic-offset: 4; indent-tabs-mode: t; -*- */

/*
 *  we install a few public methods on the 'windowStack' element in start.xul
 *
 *  stackWindow (w) - this is the entry point to use when initially
 *                    adding a window to the stack.
 *
 *  removeWindow (w) - removes @w from the window stack.
 *
 *  moveAbove (w, above) - moves @w above @above in the stacking order.
 *                         If they're in 2 different layers, nothing
 *                         happens.  If @w or @above are not presently
 *                         in the window stack, they are added before
 *                         the moveAbove operation is performed.
 *
 *  moveToBottom (w) - moves @w to the bottom of its layer.  If @w is
 *                     not in the window stack, no action is taken.
 *
 *  moveToTop (w) - moves @w to the top of its layer.  If @w is not in
 *                  the window stack, no action is taken.
 *
 *  toggleDesktop () - toggles the visibility of windows above the
 *                     desktop layer.
 *
 *  showingDesktop - property.  true if the all windows above the
 *                   desktop are hidden.
 *
 */


var windowStack = $("#windowStack")[0];

_addUtilMethods (windowStack);

windowStack.stackWindow = function (w) {
    var l = _determineLayer (w);
    Debug ("windowStack",
	   "adding window '" + w.id + "' to layer '" + l.name + "'");

    w.layer = l;
    w.style.zIndex = ++l.highZIndex; // Stack on top of the layer by default
    windowStack.appendChild (w);

    _maybeRestackLayer (l);
}


windowStack.moveAbove = function(w, above) {
    if (above.layer == undefined)
	windowStack.stackWindow (above);

    if (w.layer == undefined)
	windowStack.stackWindow (w);

    if (above.layer != w.layer)
	return; // XXX

    w.style.zIndex = above.style.zIndex;

    // This will remove and re-add the element, causing
    // interactive drags to break (with FF2).  Use with caution.
    // (To avoid this we could assign with only even zIndexes,
    // then assign the above window's to above.style.zIndex, then
    // call restoreWindow.)
    windowStack.insertAfter (w, above);

    if (w.content && w.content.onmoveabove)
	w.content.onmoveabove (above);
}


windowStack.removeWindow = function(w) {
    if (w.layer == undefined)
	return;

    try {
	windowStack.removeChild (w);
    } catch (ex) {
	/* swallow any exception raised here */ 
    }
    w.layer = undefined;
}


windowStack.moveToBottom = function(w) {
    if (w.layer == undefined)
	return;

    if (w.layer.minZIndex == w.style.zIndex) {
	// already the lowest window  
	return;
    }

    w.style.zIndex = w.layer.lowZIndex - 1;

    // Restack if we've run out of valid zIndexes
    _maybeRestackLayer (w.layer);


    if (w.content && w.content.onmovetobottom)
	w.content.onmovetobottom ();
}


windowStack.moveToTop = function(w) {
    if (w.layer == undefined)
	return;

    if (w.layer.highZIndex == w.style.zIndex) {
	// already the top window
	return;
    }

    Debug ("windowStack",
	   "windowStack.moveToTop: layer=" + w.layer.name + " zIndex=" + w.style.zIndex);

    w.style.zIndex = ++w.layer.highZIndex;

    // Restack if we've run out of valid zIndexes
    _maybeRestackLayer (w.layer);

    if (w.content && w.content.onmovetotop)
	w.content.onmovetotop ();
}

windowStack.toggleDesktop = function () {
    windowStack.showingDesktop = !windowStack.showingDesktop;
}

windowStack._showingDesktop = false;

// our one property

windowStack.addProperty ("showingDesktop",
			 /* getter */
			 function () { return windowStack._showingDesktop; },
			 /* setter */
			 function (v) {
			     if (v == windowStack._showingDesktop)
				 return;

			     windowStack._showingDesktop = v;

			     for (var el = windowStack.firstChild; el != null; el = el.nextSibling) {
				 if (el.layer && el.layer.minZIndex > desktopLayer.maxZIndex
				     /* leave the debug windows there? */
				     && el.layer != debugLayer) {
				     el.style.display = windowStack._showingDesktop ? "none" : "block";
				 }
			     }

			     svc.SetRootWindowProperty (Atoms._NET_SHOWING_DESKTOP,
							Atoms.XA_CARDINAL,
							1,
							[ windowStack.showingDesktop ? 1 : 0 ]);
			 });


// Utility functions for extensions

windowStack.getLayer = function (name) {
    for (var idx = 0; idx < layers.length; idx++) {
	if (layers[idx].name == name) {
	    return layers[idx];
	}
    }
    return null;
}


/* 
 * Layers are Objects with the following properties:
 *   name: a name for the layer
 *   minZIndex: the starting zIndex
 *   maxZIndex: the ending zIndex
 *   highZIndex: the highest zIndex of a window in that range
 */
windowStack.addLayer = function (name, minZIndex, maxZIndex) {
    var layer = new Object ();
    layer.name = name;
    layer.minZIndex = minZIndex;
    layer.maxZIndex = maxZIndex;
    layer.highZIndex = 
	layer.lowZIndex = Math.floor ((maxZIndex + maxZIndex) / 3);

    // Ensure we're not overlapping existing layers
    for (var idx = 0; idx < layers.length; idx++) {
	var l = layers[idx]
	if (l.name == name) {
	    throw "Layer name '" + name + "' already taken.";
	}
	if ((l.minZIndex <= minZIndex && l.maxZIndex >= minZIndex) || 
	    (l.minZIndex <= maxZIndex && l.maxZIndex >= maxZIndex)) {
	    throw "Cannot add layer which overlaps existing layer '" + l.name + "'.";
	}
    }

    layers.push (layer);
    return layer;
}


// initialization

var layers = new Array ();

var desktopLayer    = windowStack.addLayer ("desktopLayer",        0,  1000);
var belowLayer      = windowStack.addLayer ("belowLayer",       5000,  6000);
var normalLayer     = windowStack.addLayer ("normalLayer",     10000, 11000);
var dockLayer       = windowStack.addLayer ("dockLayer",       20000, 21000);
var fullscreenLayer = windowStack.addLayer ("fullscreenLayer", 25000, 25001);
var debugLayer      = windowStack.addLayer ("debugLayer",      60000, 61000);


// private methods

/*
 * Add a resize event handler on the parentNode so we can resize
 * the window stack in response to the document being layed out.
 */
windowStack.parentNode.onresize = function (e) {
    windowStack.style.width = windowStack.parentNode.clientWidth;
    windowStack.style.height = windowStack.parentNode.clientHeight;
}


function _sortByZIndex (w1, w2)
{
    return w1.style.zIndex - w2.style.zIndex;
}


function _restackLayer (l) {	
    if (l.highZIndex <= l.minZIndex) {
	return; // Zero or one window in layer
    }

    var windows = []
    for (var el = windowStack.firstChild; el != null; el = el.nextSibling) {
	if (el.layer == l) {
	    windows.push(el);
	}
    }

    // NOTE: It is important that this maintain document sorting
    //       order for children with the same zIndex, so we don't
    //       accidentally raise an older window above a newer one.
    windows.sort (_sortByZIndex);

    l.lowZIndex = (Math.floor ((l.maxZIndex + l.minZIndex) / 3) - 
		   Math.floor (windows.length / 3));

    var z = l.lowZIndex - 1;
    for (var idx = 0; idx < windows.length; idx++) {
	windows[idx].style.zIndex = ++z;
    }

    l.highZIndex = z;
}


function _maybeRestackLayer (l) {
    if (l.highZIndex >= l.maxZIndex) {
	Debug ("windowStack",
	       "Maximum zIndex for layer '" + l.name + "' hit, restacking");
	_restackLayer (l);
    }
    else if (l.lowZIndex <= l.minZIndex) {
	Debug ("windowStack",
	       "Minimum zIndex for layer '" + l.name + "' hit, restacking");
	_restackLayer (l);
    }
}


function _determineLayer (w) {
    /* 
     * from EWMH, bottom to top:
     *   0: windows of type _NET_WM_WINDOW_TYPE_DESKTOP
     *   1: windows having state _NET_WM_STATE_BELOW
     *   2: windows not belonging in any other layer
     *   3: windows of type _NET_WM_TYPE_DOCK (unless they have state _NET_WM_TYPE_BELOW) and
     *      windows having state _NET_WM_STATE_ABOVE
     *   4: focused windows having state _NET_WM_STATE_FULLSCREEN
     */

    // XXX we really need to think how we're going to assign layers to
    // different types of windows.  what about html content that needs
    // to go in the overlay?  are we going to require a special CSS
    // class for overlay widgets?

    if (w.content != null && (w.content.id == "debugContent" ||
			      w.content.id == "firebugContent")) 
	// special case for the debug window, which sits above everything
	return debugLayer;
    if (w.content.wmWindowType == "desktop")
	return desktopLayer;
    else if (w.content.wmWindowType == "dock")
	return dockLayer;
    /* XXX we need cases 1 and 4 here */
    else
	return normalLayer;
}
