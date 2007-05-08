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
 *  toggleDesktop () - toggles all layers above the desktop-layer.
 */

// HACK:
// compzillaWindowManager.js installs two things on it's body tag (id = "desktop"):
//
// 1. the Debug function that allows us to spew to the debug window
//    from here
// 2. the Atoms object which allows us to share the interned js cache.
//
// These are accessed from here by "windowStack.parentNode.Debug" and
// "windowStack.parentNode.Atoms".

var windowStack = document.getElementById ('windowStack');

windowStack.stackWindow = function (w) {
  var l = determineLayer (w);
  Debug ("adding window '" + w.content.id + "' to layer '" + l.id + "'");
  l.appendChild (w);
  w.layer = l;
  restackLayer (l);
}


windowStack.replaceWindow = function (w1, w2) {
  if (w1.layer == undefined) {
    Debug ("w1 not in a layer.  just adding w2");
    windowStack.stackWindow (w2);
  }
  else {
    w2.zIndex= w1.nzIndex;
    try {
      w1.layer.replaceChild (w2, w1);
    } catch (ex) { /* swallow any exception raised here.  it should only raise one if w1.layer doesn't contain w1, which shouldn't ever happen. */ }
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

  above.layer.insertBefore (w, above.nextSibling);
  restackLayer (above.layer);
}


windowStack.removeWindow = function(w) {
  if (w.layer == undefined)
    return;

  try {
    w.layer.removeChild (w);
  } catch (ex) { /* swallow any exception raised here */ }
  w.layer = undefined;
}


windowStack.moveToBottom = function(w) {
  if (w.layer == undefined)
    return;

  if (w.layer.firstChild != w)
    w.layer.insertBefore (w, w.layer.firstChild);

  restackLayer (l);
}


windowStack.moveToTop = function(w) {
  if (w.layer == undefined)
    return;

  w.layer.insertBefore (w, null);
  restackLayer (w.layer);
}


windowStack.toggleDesktop = function () {
  if (showingDesktop) {
    // hide everything above the desktop layer
    for (var i = 1; i < layers.length; i ++) {
      layers[i].style.display = "block";
    }
  }
  else {
    // hide everything above the desktop layer
    for (var i = 1; i < layers.length; i ++) {
      layers[i].style.display = "none";
    }
  }

  showingDesktop = !windowStack.showingDesktop;
}


/// initialization

var showingDesktop = false;
var layers = new Array ();

for (var el = windowStack.firstChild; el != null; el = el.nextSibling) {
  if (el.id == "desktop-layer")         layers[0] = el;
  else if (el.id == "below-layer")      layers[1] = el;
  else if (el.id == "normal-layer")     layers[2] = el;
  else if (el.id == "dock-layer")       layers[3] = el;
  else if (el.id == "fullscreen-layer") layers[4] = el;
  else if (el.id == "expose-layer")     layers[5] = el;
  else if (el.id == "picker-layer")     layers[6] = el;
  else if (el.id == "overlay-layer")    layers[7] = el;
  else if (el.id == "debug-layer")      layers[8] = el;
}

// private methods

// add a resize event handler on the parentNode so we can resize the
// window stack in response to the document being layed out.
windowStack.parentNode.onresize = function (e) {
  updateSize ();
}


function updateSize () {
  /* make sure the window stack takes up 100% of the size available to it */
  windowStack.style.width = windowStack.parentNode.clientWidth;
  windowStack.style.height = windowStack.parentNode.clientHeight;
}


function restackLayer (l) {
  var z = 0;

  for (var el = l.firstChild; el != null; el = el.nextSibling)
    el.zIndex = z++;
}


function determineLayer (w) {
  /* from EWMH:
     from bottom to top:
     * 0: windows of type _NET_WM_WINDOW_TYPE_DESKTOP
     * 1: windows having state _NET_WM_STATE_BELOW
     * 2: windows not belonging in any other layer
     * 3: windows of type _NET_WM_TYPE_DOCK (unless they have state _NET_WM_TYPE_BELOW) and
     *    windows having state _NET_WM_STATE_ABOVE
     * 4: focused windows having state _NET_WM_STATE_FULLSCREEN
     */
  if (w.content != null && w.content.id == "debugContent") // special case for the debug window, which sits above everything
    return layers[8];
  if (w._net_wm_window_type == windowStack.parentNode.Atoms._NET_WM_WINDOW_TYPE_DESKTOP())
    return layers[0];
  else if (w._net_wm_window_type == windowStack.parentNode.Atoms._NET_WM_WINDOW_TYPE_DOCK())
    return layers[3];
  /* XXX we need cases 1 and 4 here */
  else
    return layers[2];
}


function Debug (str)
{
  windowStack.parentNode.Debug (str);
}
