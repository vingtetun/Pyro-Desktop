/* -*- mode: javascript; c-basic-offset: 4; indent-tabs-mode: t; -*- */

/*
 *  we install a few public methods on the 'windowStack' element in start.xul
 *
 *  stackWindow (w) - this is the entry point to use when initially
 *                    adding a window to the stack.
 *
 *  replaceWindow (w1, w2) - replaces window @w1 with @w2.  if @w1 is
 *                           not presently in the stack, this has the
 *                           same behavior as stackWindow(@w2).
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
 *  toggleDesktop () - toggles all windows above the desktop-layer.
 */


var windowStack = document.getElementById ("windowStack");


windowStack.stackWindow = function (w) {
    var l = _determineLayer (w);
    Debug ("windowStack",
	   "adding window '" + w.id + "' to layer '" + l.name + "'");

    w.layer = l;
    w.style.zIndex = ++l.highZIndex; // Stack on top of the layer by default
    windowStack.appendChild (w);

    _maybeRestackLayer (l);
}


windowStack.replaceWindow = function (w1, w2) {
    if (w1 == w2)
	return;

    if (w1.layer == undefined) {
	Debug ("windowStack",
	       "w1 not in a layer.  just adding w2");
	windowStack.stackWindow (w2);
    }
    else {
	w2.style.zIndex = w1.style.zIndex;
	try {
	    windowStack.replaceChild (w2, w1);
	} catch (ex) { 
	    /* 
	     * swallow any exception raised here.  it should only raise one if
	     * w1.layer doesn't contain w1, which shouldn't ever happen. 
	     */ 
	}
	w2.layer = w1.layer;
	w1.layer = undefined;
    }
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

    if (w.layer.minZIndex != w.style.zIndex) {
	// Not already the lowest window  
	// Assign an invalid zIndex, which will be reassigned
	// in restackLayer.
	w.style.zIndex = w.layer.minZIndex - 1;

	// Restack now to assign valid zIndex to w
	_restackLayer (w.layer);
    }
}


windowStack.moveToTop = function(w) {
    if (w.layer == undefined)
	return;

    Debug ("windowStack",
	   "windowStack.moveToTop: layer=" + w.layer.name + " zIndex=" + w.style.zIndex);

    if (w.layer.highZIndex != w.style.zIndex) {
	// Not already the top window
	w.style.zIndex = ++w.layer.highZIndex;

	// Restack if we've run out of valid zIndexes
	_maybeRestackLayer (w.layer);
    }
}


windowStack.toggleDesktop = function () {
    showingDesktop = !showingDesktop;

    Debug ("windowStack",
	   "toggling the desktop display");
    for (var el = windowStack.firstChild; el != null; el = el.nextSibling) {
	if (el.layer != desktopLayer) {
	    el.style.display = showingDesktop ? "none" : "block";
	}
    }
}


// Utility functions for extensions

windowStack.getLayer = function (name) {
    for (var idx = 0; idx < layers.length; idx++) {
	if (layers[idx].name == name) {
	    return layers[idx];
	}
    }
}


/* 
 * Layers are Objects with the following properties:
 *   name: a name for the layer
 *   minZIndex: the starting zIndex
 *   maxZIndex: the ending zIndex
 *   highZIndex: the highest zIndex of a window in that range
 */
windowStack.addLayer = function (name, minZIndex, maxZIndex) {
    layer = new Object ();
    layer.name = name;
    layer.minZIndex = minZIndex;
    layer.maxZIndex = maxZIndex;
    layer.highZIndex = minZIndex;

    // Ensure we're not overlapping existing layers
    for (var idx = 0; idx < layers.length; idx++) {
	l = layers[idx]
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

var showingDesktop = false;
var layers = new Array ();

desktopLayer    = windowStack.addLayer ("desktopLayer",        0,  1000);
belowLayer      = windowStack.addLayer ("belowLayer",       5000,  6000);
normalLayer     = windowStack.addLayer ("normalLayer",     10000, 11000);
dockLayer       = windowStack.addLayer ("dockLayer",       20000, 21000);
fullscreenLayer = windowStack.addLayer ("fullscreenLayer", 25000, 25001);
debugLayer      = windowStack.addLayer ("debugLayer",      60000, 61000);


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

    var z = l.minZIndex - 1;

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

    if (w.content != null && w.content.id == "debugContent") 
	// special case for the debug window, which sits above everything
	return debugLayer;
    if (w._net_wm_window_type == Atoms._NET_WM_WINDOW_TYPE_DESKTOP())
	return desktopLayer;
    else if (w._net_wm_window_type == Atoms._NET_WM_WINDOW_TYPE_DOCK())
	return dockLayer;
    /* XXX we need cases 1 and 4 here */
    else
	return normalLayer;
}
