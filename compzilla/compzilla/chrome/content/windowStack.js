function Debug (str)
{
  // hack.  compzillaWindowManager.js installs its Debug function on
  // the document.body so we can get access to it here.
  windowStack.parentNode.Debug (str);
}

var windowStack = document.getElementById ('windowStack');

windowStack.showingDesktop = false;
windowStack.layers = new Array ();

for (var el = windowStack.firstChild; el != null; el = el.nextSibling) {
  if (el.id == "desktop-layer")         windowStack.layers[0] = el;
  else if (el.id == "below-layer")      windowStack.layers[1] = el;
  else if (el.id == "normal-layer")     windowStack.layers[2] = el;
  else if (el.id == "dock-layer")       windowStack.layers[3] = el;
  else if (el.id == "fullscreen-layer") windowStack.layers[4] = el;
  else if (el.id == "expose-layer")     windowStack.layers[5] = el;
  else if (el.id == "picker-layer")     windowStack.layers[6] = el;
  else if (el.id == "overlay-layer")    windowStack.layers[7] = el;
  else if (el.id == "debug-layer")      windowStack.layers[8] = el;
}


// add a resize event handler on the parentNode so we can resize the window stack
windowStack.parentNode.onresize = function (e) {
  windowStack.updateSize ();
}

windowStack.updateSize = function () {
  /* make sure the window stack takes up 100% of the size available to it */
  windowStack.style.width = windowStack.parentNode.clientWidth;
  windowStack.style.height = windowStack.parentNode.clientHeight;
}

windowStack.determineLayer = function (w) {
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
    return windowStack.layers[8];
  if (w._net_wm_window_type == Atoms._NET_WM_WINDOW_TYPE_DESKTOP())
    return windowStack.layers[0];
  else if (w._net_wm_window_type == Atoms._NET_WM_WINDOW_TYPE_DOCK())
    return windowStack.layers[3];
  /* XXX we need cases 1 and 4 here */
  else
    return windowStack.layers[2];
}


windowStack.stackWindow = function (w) {
  Debug ("w.className == " + w.className);
  var l = windowStack.determineLayer (w);
  Debug ("adding window '" + w.content.id + "' to layer '" + l.id + "'");
  l.appendChild (w);
  w.layer = l;
  windowStack.restackLayer (l);
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


windowStack.restackLayer = function (l) {
  var z = 0;

  for (var el = l.firstChild; el != null; el = el.nextSibling)
    el.zIndex = z++;
}


windowStack.moveAbove = function(w, above) {
  if (above.layer == undefined)
    windowStack.stackWindow (above, windowStack.determineLayer (above));

  if (w.layer == undefined)
    windowStack.stackWindow (w, windowStack.determineLayer (w));

  if (above.layer != w.layer)
    return; // XXX

  above.layer.insertBefore (w, above.nextSibling);
  windowStack.restackLayer (above.layer);
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

  windowStack.restackLayer (l);
}


windowStack.moveToTop = function(w) {
  if (w.layer == undefined)
    return;

  w.layer.insertBefore (w, null);
  windowStack.restackLayer (w.layer);
}

windowStack.toggleDesktop = function () {
  if (windowStack.showingDesktop) {
    // hide everything above the desktop layer
    for (var i = 1; i < windowStack.layers.length; i ++) {
      windowStack.layers[i].style.display = "block";
    }
  }
  else {
    // hide everything above the desktop layer
    for (var i = 1; i < windowStack.layers.length; i ++) {
      windowStack.layers[i].style.display = "none";
    }
  }

  windowStack.showingDesktop = !windowStack.showingDesktop;
}
