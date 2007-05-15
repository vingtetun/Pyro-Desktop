/* -*- mode: javascript; c-basic-offset: 4; indent-tabs-mode: t; -*- */

cls = Components.classes['@pyrodesktop.org/compzillaService'];
svc = cls.getService(Components.interfaces.compzillaIControl);

var _focusedFrame;

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


var FrameMethods = {
    getContent: function () {
	return this._content;
    },


    destroy: function () {
	Debug ("frame.destroy");

	if (this._content && this._content.destroy) {
	    this._content.destroy ();
	}
	windowStack.removeWindow (this);
    },


    setTitle: function (title) {
	if (this._title)
	    this._title.value = title;
    },


    getTitle: function () {
	return this._title ? this._title.value : null;
    },


    getWMClass: function () {
	return this.getAttributeNS ("http://www.pyrodesktop.org/compzilla",
				    "wm-class");
    },


    setWMClass: function (wmclass) {
	return this.setAttributeNS ("http://www.pyrodesktop.org/compzilla",
				    "wm-class", wmclass);
    },


    getAllowedActions: function () {
	return this.getAttributeNS ("http://www.pyrodesktop.org/compzilla",
				    "allowed-actions");
    },


    setAllowedActions: function (actions) {
	return this.setAttributeNS ("http://www.pyrodesktop.org/compzilla",
				    "allowed-actions", actions);
    },

    moveResize: function (x, y, width, height) {
	Debug ("frame.moveResize: w=" + width + ", h=" + height);

	this._moveResize (x, y, width, height);

	if (this._content.getNativeWindow) {
	    svc.ConfigureWindow (this._content.getNativeWindow().nativeWindowId,
				 x + this._contentBox.offsetLeft,
				 y + this._contentBox.offsetTop,
				 this._contentBox.offsetWidth,
				 this._contentBox.offsetHeight,
				 0);
	}
    },


    _moveResize: function (x, y, width, height) {
	if (this.offsetWidth != width) {
	    this._contentBox.style.width = width + "px";
	    if (this._titleBox)
		this._titleBox.style.width = width + "px";
	    this._content.width = width;
	}
	if (this.offsetHeight != height) {
	    this._contentBox.style.height = height + "px";
	    this._content.height = height;
	}
	if (this.offsetLeft != x) {
	    this.style.left = x + "px";
	}
	if (this.offsetTop != y) {
	    this.style.top = y + "px";
	}
    },


    _recomputeAllowedActions: function () {
	/* this only applies for native windows, and those windows
	   which have had mwm hints/_net_wm_window_type set. */
	if (/* XXX mwm hints? */ this._net_wm_window_type == undefined)
	    return;

	var max_action = "maximize ";
	var min_action = "minimize ";
	var close_action = "close ";
	var resize_action = "resize ";
	var fullscreen_action = "fullscreen ";
	var move_action = "move ";
	var shade_action = "shade ";

	// XXX first consider the mwm hints

	// next override using metacity's rules for assigning features
	// based on EWMH's window types.
	if (this._net_wm_window_type == Atoms._NET_WM_WINDOW_TYPE_DESKTOP () ||
	    this._net_wm_window_type == Atoms._NET_WM_WINDOW_TYPE_DOCK () ||
	    this._net_wm_window_type == Atoms._NET_WM_WINDOW_TYPE_SPLASH ()) {
	    close_action = "";
	    shade_action = "";
	    move_action = "";
	    resize_action = "";
	}


	if (this._net_wm_window_type != Atoms._NET_WM_WINDOW_TYPE_NORMAL ()) {
	    max_action = "";
	    min_action = "";
	    fullscreen_action = "";
	}

	if (!resize_action) {
	    max_action = "";
	    fullscreen_action = "";
	}
    },


    show: function () {
	Debug ("frame.show");

	this.style.display = "block";
    },


    hide: function () {
	Debug ("frame.hide");

	this.style.display = "none";
    },

};

function _compzillaFrameCommon (content, templateId)
{
    var frame = document.getElementById (templateId).cloneNode (true);

    frame._content = content;

    frame._titleBox = getDescendentById (frame, "windowTitleBox");

    frame._contentBox = getDescendentById (frame, "windowContentBox");
    frame._contentBox.appendChild (content);

    frame._title = getDescendentById (frame, "windowTitle");

    for (var m in FrameMethods) {
	frame[m] = FrameMethods[m];
    }

    if (content.getNativeWindow) {
	_connectNativeWindowListeners (frame, content);
    }

    if (frame._title) {
	_connectFrameDragListeners (frame);
    }

    // click to raise/focus
    frame.addEventListener (
        "mousedown", 
        {
            handleEvent: function (event) {
		windowStack.moveToTop (frame);

		// XXX this should live in some sort of focus handler, not here.
		if (frame.className.match ("Focused") == null) {

		    if (_focusedFrame != null) {
			_focusedFrame.className =
			    _focusedFrame.className.slice (
					   0, 
					   _focusedFrame.className.lastIndexOf ("Focused"));
		    }
			
		    frame.className += "Focused";
		    _focusedFrame = frame;
		}
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
    };

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
    };

    frame._title.onmousedown = function (ev) {
	frameDragPosition.x = ev.clientX;
	frameDragPosition.y = ev.clientY;
	document.addEventListener ("mousemove", frameDragMouseMoveListener, true);
	document.addEventListener ("mouseup", frameDragMouseUpListener, true);
	ev.stopPropagation ();
    };
}

function _copyPyroFrameAttributes (from, to)
{
    to.setWMClass (from.getWMClass ());
    to.setAllowedActions (from.getAllowedActions ());
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
			Debug ("destroy.handleEvent (" + content + ")");
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

			svc.SendConfigureNotify (nativewin.nativeWindowId, 
						 ev.x, ev.y, 
						 ev.width, ev.height, 
						 ev.borderWidth);

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
			    Debug ("window " + frame.id + " type set");
			    // XXX _NET_WM_WINDOW_TYPE is actually an array of atoms, not just 1.
			    var type = ev.value.getPropertyAsUint32 (".atom");

			    var new_frame = frame;

			    if (type == Atoms._NET_WM_WINDOW_TYPE_DOCK() ||
				type == Atoms._NET_WM_WINDOW_TYPE_DESKTOP() ||
				type == Atoms._NET_WM_WINDOW_TYPE_SPLASH() ||
				type == Atoms._NET_WM_WINDOW_TYPE_TOOLBAR()) {
				if (frame.className == "windowFrame") {
				    new_frame = CompzillaDockFrame (frame.getContent());
				}
			    }
			    else {
				if (frame.className == "dockWindowFrame") {
				    new_frame = CompzillaFrame (frame.getContent());
				}
			    }

			    new_frame._net_wm_window_type = type;

			    _copyPyroFrameAttributes (frame, new_frame);

			    new_frame._recomputeAllowedActions ();

			    windowStack.replaceWindow (frame, new_frame);

			    return;
			}

			if (ev.atom == Atoms.WM_CLASS()) {
			    frame.setWMClass (ev.value.getProperty (".instanceName") +
					      " " +
					      ev.value.getProperty (".className"));

			    Debug ("frame wm-class = `" +  frame.getWMClass ());
			    return;
			}
		    }
		},
	    true);
}
