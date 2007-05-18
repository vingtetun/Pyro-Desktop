/* -*- mode: javascript; c-basic-offset: 4; indent-tabs-mode: t; -*- */


cls = Components.classes['@pyrodesktop.org/compzillaService'];
svc = cls.getService(Components.interfaces.compzillaIControl);

var _focusedFrame;


function CompzillaMakeFrame (content, typeAtom, isOverride)
{
    if (!isOverride && !typeAtom && content.getNativeWindow) {
        try {
            typeAtom = content.getNativeWindow().GetAtomProperty (Atoms._NET_WM_WINDOW_TYPE());
        } catch (ex) {
            // _NET_WM_WINDOW_TYPE may not be set on new windows
        }
    }

    var className = _determineFrameClassName (typeAtom, isOverride, false);
    var frame = CompzillaFrame (content, className);

    frame._overrideRedirect = isOverride;
    frame._net_wm_window_type = typeAtom;

    if (content.getNativeWindow) {
        frame.id = "XID:" + content.getNativeWindow().nativeWindowId;
    }

    return frame;
}


function _determineFrameClassName (typeAtom, isOverride, focused)
{
    if (isOverride ||
        typeAtom == Atoms._NET_WM_WINDOW_TYPE_DOCK() ||
        typeAtom == Atoms._NET_WM_WINDOW_TYPE_DESKTOP() ||
        typeAtom == Atoms._NET_WM_WINDOW_TYPE_SPLASH() ||
        typeAtom == Atoms._NET_WM_WINDOW_TYPE_TOOLBAR()) {
        return focused ? "dockFrameFocused" : "dockFrame";
    } else {
        return focused ? "windowFrameFocused" : "windowFrame";
    }
}


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
            this._content = null;
	}

	if (_focusedFrame == this)
	    _focusedFrame = null;

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
        Debug ("XXXXXXX setAllowedActions: " + actions);
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

        Debug ("XXXXXXXX _recomputeAllowedActions!");

	var max = true;
	var min = true;
	var close = true;
	var resize = true;
	var fullscreen = true;
	var move = true;
	var shade = true;

	// XXX first consider the mwm hints

	// next override using metacity's rules for assigning features
	// based on EWMH's window types.
	if (this._net_wm_window_type == Atoms._NET_WM_WINDOW_TYPE_DESKTOP () ||
	    this._net_wm_window_type == Atoms._NET_WM_WINDOW_TYPE_DOCK () ||
	    this._net_wm_window_type == Atoms._NET_WM_WINDOW_TYPE_SPLASH ()) {
	    close_action = false;
	    shade_action = false;
	    move_action = false;
	    resize_action = false;
	}

	if (this._net_wm_window_type != Atoms._NET_WM_WINDOW_TYPE_NORMAL ()) {
	    max_action = false;
	    min_action = false;
	    fullscreen_action = false;
	}

	if (!resize_action) {
	    max_action = false;
	    fullscreen_action = false;
	}

        var actionProp = "";
        actionProp += max_action ? "maximize " : "";
        actionProp += min_action ? "minimize " : "";
        actionProp += close_action ? "close " : "";
        actionProp += resize_action ? "resize " : "";
        actionProp += fullscreen_action ? "fullscreen " : "";
        actionProp += move_action ? "move " : "";
        actionProp += shade_action ? "shade " : "";

        this.setAllowedActions (actionProp);
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


function CompzillaFrame (content, className)
{
    var frame = document.getElementById ("windowFrame").cloneNode (true);
    frame.className = className;

    frame._content = content;

    frame._titleBox = getDescendentById (frame, "windowTitleBox");

    frame._contentBox = getDescendentById (frame, "windowContentBox");
    frame._contentBox.appendChild (content);

    frame._title = getDescendentById (frame, "windowTitle");

    for (var m in FrameMethods) {
	frame[m] = FrameMethods[m];
    }

    if (content.getNativeWindow) {
	_connectNativeWindowListeners (frame, content.getNativeWindow());
        frame._recomputeAllowedActions ();
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
                if (frame != _focusedFrame) {
                    if (_focusedFrame) {
                        // Send FocusOut
                        _focusedFrame._content.blur ();
                        _focusedFrame.className = 
                            _determineFrameClassName (_focusedFrame._net_wm_window_type,
                                                      _focusedFrame._overrideRedirect,
                                                      false);
                    }

                    // Send FocusIn
                    frame._content.focus ();
                    frame.className = _determineFrameClassName (frame._net_wm_window_type,
                                                                frame._overrideRedirect,
                                                                true);

                    Debug ("XXX Setting classname = " + frame.className);

                    _focusedFrame = frame;
                }
            }
        },
	true);

    return frame;
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


function _connectNativeWindowListeners (frame, nativewin) 
{
    frame._nativeDestroyListener =
	addCachedEventListener (
	    nativewin,
	    "destroy",
	    frame._nativeDestroyListener,
		{
		    handleEvent: function (ev) {
			Debug ("destroy.handleEvent id='" + frame.id + "'");
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

			if (!ev.overrideRedirect) {
			    svc.ConfigureWindow (
                                frame._content.getNativeWindow().nativeWindowId,
                                ev.x, ev.y, ev.width, ev.height,
                                ev.borderWidth);
                        }

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

                            // XXX _NET_WM_WINDOW_TYPE is actually an array of atoms, not just 1
                            var typeAtom = ev.value.getPropertyAsUint32 (".atom");

                            frame.className = _determineFrameClassName (typeAtom,
                                                                        frame._overrideRedirect,
                                                                        frame == _focusedFrame);

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
