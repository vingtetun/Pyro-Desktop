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
    Debug ("adding window '" + w.id + "' to layer '" + l[0] + "'");

    w.layer = l;
    w.style.zIndex = ++l[3]; // Stack on top of the layer by default
    windowStack.appendChild (w);

    _maybeRestackLayer (l);
}


windowStack.replaceWindow = function (w1, w2) {
    if (w1 == w2)
	return;

    if (w1.layer == undefined) {
	Debug ("w1 not in a layer.  just adding w2");
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

    if (w.layer[1] != w.style.zIndex && 
	w.layer[3] != w.style.zIndex) {
	// Not the lowest window, or the only window.  
	// Assign an invalid zIndex, which will be reassigned
	// in restackLayer.
	w.style.zIndex = w.layer[1] - 1;

	// Restack now to assign valid zIndex to w
	_restackLayer (w.layer);
    }
}


windowStack.moveToTop = function(w) {
    if (w.layer == undefined)
	return;

    Debug ("windowStack.moveToTop: layer=" + w.layer + " zIndex=" + w.style.zIndex);

    if (w.layer[3] != w.style.zIndex) {
	// Not already the top window
	w.style.zIndex = ++w.layer[3];

	// Restack if we've run out of valid zIndexes
	_maybeRestackLayer (w.layer);
    }
}


windowStack.toggleDesktop = function () {
    showingDesktop = !showingDesktop;

    Debug ("toggling the desktop display");
    for (var el = windowStack.firstChild; el != null; el = el.nextSibling) {
	if (el.layer != layers[0]) {
	    el.style.display = showingDesktop ? "none" : "block";
	}
    }
}


// initialization

var showingDesktop = false;

/*
 * layers is an array of arrays, each containing 
 *   0: a name for the layer
 *   1: the starting zIndex
 *   2: the ending zIndex
 *   3: the highest zIndex of a window in that range
 */
var layers = new Array ();
layers[0] = ["desktopLayer",        0,  1000,     0];
layers[1] = ["belowLayer",       5000,  6000,  5000];
layers[2] = ["normalLayer",     10000, 11000, 10000];
layers[3] = ["dockLayer",       20000, 21000, 20000];
layers[4] = ["fullscreenLayer", 25000, 25001, 25000];
layers[5] = ["debugLayer",      60000, 61000, 60000];


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
    var lower = l[1]; // Lower zIndex bound
    var upper = l[2]; // Upper zIndex bound

    if (l[3] <= lower) {
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

    var z = lower - 1;

    for (var idx = 0; idx < windows.length; idx++) {
	windows[idx].style.zIndex = ++z;
    }

    l[3] = z;
}


function _maybeRestackLayer (l) {
    if (l[3] >= l[2]) {
	Debug ("Maximum zIndex for layer '" + l[0] + "' hit, restacking");
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

    if (w.getContent() != null && w.getContent().id == "debugContent") 
	// special case for the debug window, which sits above everything
	return layers[5];
    if (w._net_wm_window_type == Atoms._NET_WM_WINDOW_TYPE_DESKTOP())
	return layers[0];
    else if (w._net_wm_window_type == Atoms._NET_WM_WINDOW_TYPE_DOCK())
	return layers[3];
    /* XXX we need cases 1 and 4 here */
    else
	return layers[2];
}
