/* -*- mode: javascript; c-basic-offset: 4; indent-tabs-mode: t; -*- */

cls = Components.classes['@pyrodesktop.org/compzillaService'];
svc = cls.getService(Components.interfaces.compzillaIControl);

function getDescendentById (root, id)
{
    if (root.id == id)
	return root;

    for (var el = root.firstChild; el != null; el = el.nextSibling) {
	var rv = getDescendentById (el, id);
	if (rv != null) {
	    return rv;
	}
    }

    return null;
}


/* probably a rather more useful function */
function addCachedEventListener (o, eventid, cachedListener, newListener, usecapture)
{
    if (cachedListener != undefined)
	o.removeEventListener (eventid, cachedListener, usecapture);
    o.addEventListener (eventid, newListener, usecapture);
    return newListener;
}


function _compzillaFrameCommon (content, templateId)
{
    var frame = document.getElementById (templateId).cloneNode (true);

    frame._content = content;
    frame.getContent = function () { return frame._content; };

    titleBox = getDescendentById (frame, "windowTitleBox");

    contentBox = getDescendentById (frame, "windowContentBox");
    contentBox.appendChild (content);

    frame.destroy = function () {
	Debug ("frame.destroy");

	if (content.destroy) {
	    content.destroy ();
	}
	windowStack.removeWindow (frame);
    };

    frame._title = getDescendentById (frame, "windowTitle");
    frame.setTitle = function (title) { if (frame._title) frame._title.value = title; };
    frame.getTitle = function () { return frame._title ? frame._title.value : null; };

    frame.moveResize = function (x, y, width, height) {
	//Debug ("frame.moveResize: w=" + width + ", h=" + height);

	frame._moveResize (x, y, width, height);

	if (content.getNativeWindow) {
	    svc.ConfigureWindow (content.getNativeWindow().nativeWindowId,
				 x + contentBox.offsetLeft,
				 y + contentBox.offsetTop,
				 contentBox.offsetWidth,
				 contentBox.offsetHeight,
				 0);
	}
    };

    frame._moveResize = function (x, y, width, height) {
	if (frame.offsetWidth != width) {
	    contentBox.style.width = width + "px";
	    if (titleBox)
		titleBox.style.width = width + "px";
	    content.width = width;
	}
	if (frame.offsetHeight != height) {
	    contentBox.style.height = height + "px";
	    content.height = height;
	}
	if (frame.offsetLeft != x) {
	    frame.style.left = x + "px";
	}
	if (frame.offsetTop != y) {
	    frame.style.top = y + "px";
	}
    };

    frame.show = function () {
	Debug ("frame.show");

	frame.style.visibility = "visible";
	content.style.visibility = "visible";
    };

    frame.hide = function () {
	Debug ("frame.hide");

	frame.style.visibility = "hidden";
	content.style.visibility = "hidden";
    };

    if (content.getNativeWindow) {
	_connectNativeWindowListeners (frame, content);
    }

    if (frame._title) {
	_connectFrameDragListeners (frame);
    }

    // click to raise
    frame.addEventListener (
        "mousedown", 
        {
            handleEvent: function (event) {
		windowStack.moveToTop (frame);
            }
        },
	true);

    return frame;
}


function CompzillaFrame (content)
{
    return _compzillaFrameCommon (content, "windowFrame");
}


function CompzillaDockFrame (content)
{
    return _compzillaFrameCommon (content, "dockFrame");
}


function _connectFrameDragListeners (frame)
{
    var frameDragPosition = new Object ();
    var frameDragMouseMoveListener = {
        handleEvent: function (ev) {
	    if (frame.originalOpacity == undefined) {
		frame.originalOpacity = frame.style.opacity;
		frame.style.opacity = "0.8";
	    }

	    // figure out the deltas
	    var dx = ev.clientX - frameDragPosition.x;
	    var dy = ev.clientY - frameDragPosition.y;

	    frameDragPosition.x = ev.clientX;
	    frameDragPosition.y = ev.clientY;

	    frame.moveResize (frame.offsetLeft + dx,
			      frame.offsetTop + dy,
			      frame.offsetWidth,
			      frame.offsetHeight); 

	    ev.stopPropagation ();
	}
    }

    var frameDragMouseUpListener = {
	handleEvent: function (ev) {
	    if (frame.originalOpacity != undefined) {
		frame.style.opacity = frame.originalOpacity;
		frame.originalOpacity = undefined;
	    }

	    // clear the event handlers we add in the title mousedown handler below
	    document.removeEventListener ("mousemove", frameDragMouseMoveListener, true);
	    document.removeEventListener ("mouseup", frameDragMouseUpListener, true);

	    ev.stopPropagation ();
	}
    }

    frame._title.onmousedown = function (ev) {
	frameDragPosition.x = ev.clientX;
	frameDragPosition.y = ev.clientY;
	document.addEventListener ("mousemove", frameDragMouseMoveListener, true);
	document.addEventListener ("mouseup", frameDragMouseUpListener, true);
	ev.stopPropagation ();
    }
}


function _connectNativeWindowListeners (frame, content) 
{
    var nativewin = content.getNativeWindow ();

    frame._nativeDestroyListener =
	addCachedEventListener (
	    nativewin,
	    "destroy",
	    frame._nativeDestroyListener,
		{
		    handleEvent: function (ev) {
			Debug ("destroy.handleEvent");
			frame.destroy ();
		    }
		},
	    true);

    frame._nativeConfigureListener =
	addCachedEventListener (
	    nativewin,
	    "configure",
	    frame._nativeConfigureListener,
		{
		    handleEvent: function (ev) {
			Debug ("configure.handleEvent");
			frame._moveResize (ev.x, ev.y, ev.width, ev.height);

			svc.SendConfigureNotify (nativewin.nativeWindowId, ev.x, ev.y, ev.width, ev.height, ev.borderWidth);

			// XXX handle stacking requests here too
		    }
		},
	    true);

    frame._nativeMapListener =
	addCachedEventListener (
	    nativewin,
	    "map",
	    frame._nativeMapListener,
		{
		    handleEvent: function (ev) {
			Debug ("map.handleEvent");
			frame.show ();
		    }
		},
	    true);

    frame._nativeUnmapListener =
	addCachedEventListener (
	    nativewin,
	    "unmap",
	    frame._nativeUnmapListener,
		{
		    handleEvent: function (ev) {
			Debug ("unmap.handleEvent");
			frame.hide ();
		    }
		},
	    true);

    frame._nativePropertyChangeListener =
	addCachedEventListener (
	    nativewin,
	    "propertychange",
	    frame._nativePropertyChangeListener,
		{
		    handleEvent: function (ev) {
			Debug ("propertychange.handleEvent: ev.atom=" + ev.atom);


			if (ev.atom == Atoms.WM_NAME () ||
			    ev.atom == Atoms._NET_WM_NAME ()) {
			    name = ev.value.getProperty(".text");

			    Debug ("propertychange: setting title =" + name);
			    frame.setTitle (name);
			    return;
			}



			if (ev.atom == Atoms._NET_WM_WINDOW_TYPE()) {
			    //Debug ("window " + content.xid + " type set");
			    // XXX _NET_WM_WINDOW_TYPE is actually an array of atoms, not just 1.
			    var type = ev.value.getPropertyAsUint32 (".atom");

			    frame._net_wm_window_type = type;

			    var new_frame = null;

			    if (type == Atoms._NET_WM_WINDOW_TYPE_DOCK() ||
				type == Atoms._NET_WM_WINDOW_TYPE_DESKTOP() ||
				type == Atoms._NET_WM_WINDOW_TYPE_SPLASH()) {
				if (frame.className == "windowFrame") {
				    new_frame = CompzillaDockFrame (frame.getContent());
				}
			    }
			    else {
				if (frame.className == "dockWindowFrame") {
				    new_frame = CompzillaFrame (frame.getContent());
				}
			    }

			    if (new_frame != null)
				windowStack.replaceWindow (frame, new_frame);

			    return;
			}
		    }
		},
	    true);
}
