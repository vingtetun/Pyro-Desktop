/* -*- mode: javascript; c-basic-offset: 4; indent-tabs-mode: t; -*- */


cls = Components.classes['@pyrodesktop.org/compzillaService'];
svc = cls.getService(Components.interfaces.compzillaIControl);

var _focusedFrame;


function CompzillaMakeFrame (content, typeAtom, isOverride)
{
    if (!isOverride && !typeAtom && content.nativewindow) {
        try {
            typeAtom = content.nativeWindow.GetAtomProperty (Atoms._NET_WM_WINDOW_TYPE());
        } catch (ex) {
            // _NET_WM_WINDOW_TYPE may not be set on new windows
        }
    }

    var className = _determineFrameClassName (typeAtom, isOverride, false);
    var frame = CompzillaFrame (content, className);

    frame._overrideRedirect = isOverride;
    frame._net_wm_window_type = typeAtom;

    if (content.nativeWindow) {
        frame.id = "XID:" + content.nativeWindow.nativeWindowId;
    }

    return frame;
}


function _determineFrameClassName (typeAtom, isOverride)
{
    if (isOverride ||
        typeAtom == Atoms._NET_WM_WINDOW_TYPE_DOCK() ||
        typeAtom == Atoms._NET_WM_WINDOW_TYPE_DESKTOP() ||
        typeAtom == Atoms._NET_WM_WINDOW_TYPE_SPLASH() ||
        typeAtom == Atoms._NET_WM_WINDOW_TYPE_TOOLBAR()) {
        return "dockFrame";
    } else {
        return "windowFrame";
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
    destroy: function () {
	Debug ("frame",
	       "frame.destroy");

	if (this._content && this._content.destroy) {
	    this._content.destroy ();
            this._content = null;
	}

	if (_focusedFrame == this)
	    _focusedFrame = null;

	windowStack.removeWindow (this);
    },

    moveResize: function (x, y, width, height) {
	Debug ("frame",
	       "frame.moveResize: w=" + width + ", h=" + height);

	this._moveResize (x, y, width, height);

	if (this._content.nativeWindow) {
	    svc.ConfigureWindow (this._content.nativeWindow.nativeWindowId,
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

	var max_action = true;
	var min_action = true;
	var close_action = true;
	var resize_action = true;
	var fullscreen_action = true;
	var move_action = true;
	var shade_action = true;
	var resize_action = true;

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

	Debug ("new actions = " + actionProp);

        this.allowedActions = actionProp;
    },


    show: function () {
	Debug ("frame", "frame.show");

	this.style.display = "block";
    },


    hide: function () {
	Debug ("frame", "frame.hide");

	this.style.display = "none";
    },


    setWindowState: function (state) {
	this.windowState += " " + state + " ";
    },


    clearWindowState: function (state) {
	this.windowState = this.windowState.replace (" " + state + " ", "", "g");
    },


    hasWindowState: function (state) {
	return this.windowState.indexOf (" " + state + " ") != -1;
    },


    addProperty: function (name, getter, setter) {
	this.__defineGetter__ (name, getter);

	/* allow setter to be undefined for read-only properties */
	if (setter != undefined)
	    this.__defineSetter__ (name, setter);
    }
};

function _addFrameMethods (frame)
{
    for (var m in FrameMethods) {
	frame[m] = FrameMethods[m];
    }

    // now add our public properties
    frame.addProperty ("title",
		       /* getter */
		       function () {
			   return this._title ? this._title.value : null;
		       },
		       /* setter */
		       function (t) {
			   if (this._title)
			       this._title.value = t;
		       });

    frame.addProperty ("wmClass",
		       /* getter */
		       function () {
			   return this.getAttributeNS (
				       "http://www.pyrodesktop.org/compzilla",
				       "wm-class");
		       },
		       /* setter */
		       function (wmclass) {
			   return this.setAttributeNS (
				       "http://www.pyrodesktop.org/compzilla",
				       "wm-class", wmclass);
		       });

    frame.addProperty ("windowState",
		       /* getter */
		       function () {
			   return this.getAttributeNS (
				       "http://www.pyrodesktop.org/compzilla",
				       "window-state");
		       },
		       /* setter */
		       function (wmclass) {
			   return this.setAttributeNS (
				       "http://www.pyrodesktop.org/compzilla",
				       "window-state", wmclass);
		       });

    frame.addProperty ("allowedActions",
		       /* getter */
		       function () {
			   return this.getAttributeNS (
				       "http://www.pyrodesktop.org/compzilla",
				       "allowed-actions");
		       },
		       /* setter */
		       function (actions) {
			   return this.setAttributeNS (
				       "http://www.pyrodesktop.org/compzilla",
				       "allowed-actions", actions);
		       });

    frame.addProperty ("content",
		       /* getter */
		       function () {
			   return this._content;
		       }
		       /* no setter */
		       );
}

function CompzillaFrame (content, className)
{
    var frame = document.getElementById ("windowFrame").cloneNode (true);
    frame.className = className;

    frame._content = content;

    frame._titleBox = getDescendentById (frame, "windowTitleBox");

    frame._contentBox = getDescendentById (frame, "windowContentBox");
    frame._contentBox.appendChild (content);

    frame._title = getDescendentById (frame, "windowTitle");

    // add our methods
    _addFrameMethods (frame);
    
    if (content.nativeWindow) {
	_connectNativeWindowListeners (frame, content.nativeWindow);
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
		if (_focusedFrame != frame) {
		    if (_focusedFrame) {
			// Send FocusOut
			_focusedFrame._content.blur();
			_focusedFrame.clearWindowState ("focused");
                     }

                     // Send FocusIn
                     frame._content.focus();
		     frame.setWindowState ("focused");

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
	    frame.setWindowState ("moving");

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
	    frame.clearWindowState ("moving");

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
			Debug ("frame", "destroy.handleEvent (" + content + ")");
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
			Debug ("frame", "configure.handleEvent");

			frame._moveResize (ev.x, ev.y, ev.width, ev.height);

			if (!ev.overrideRedirect) {
			    svc.ConfigureWindow (
                                frame._content.nativeWindow.nativeWindowId,
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
			Debug ("frame", "map.handleEvent");
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
			Debug ("frame", "unmap.handleEvent");
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
			Debug ("frame", "propertychange.handleEvent: ev.atom=" + ev.atom);

			if (ev.atom == Atoms.WM_NAME () ||
			    ev.atom == Atoms._NET_WM_NAME ()) {
			    name = ev.value.getProperty(".text");

			    Debug ("frame", "propertychange: setting title =" + name);
			    frame.title = name;
			    return;
			}

			if (ev.atom == Atoms._NET_WM_WINDOW_TYPE()) {
			    Debug ("frame", "window " + frame.id + " type set");

                            // XXX _NET_WM_WINDOW_TYPE is actually an array of atoms, not just 1
                            var typeAtom = ev.value.getPropertyAsUint32 (".atom");

                            frame.className = _determineFrameClassName (typeAtom,
                                                                        frame._overrideRedirect);

			    frame._net_wm_window_type = typeAtom;

			    frame._recomputeAllowedActions ();

			    return;
			}

			if (ev.atom == Atoms.WM_CLASS()) {
			    frame.wmClass = (ev.value.getProperty (".instanceName") +
					     " " +
					     ev.value.getProperty (".className"));
					     
			    Debug ("frame", "frame wm-class = `" +  frame.wmClass);
			    return;
			}
		    }
		},
	    true);
}
