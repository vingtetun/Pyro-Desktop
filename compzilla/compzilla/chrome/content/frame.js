/* -*- mode: javascript; c-basic-offset: 4; indent-tabs-mode: t; -*- */


var _PYRO_NAMESPACE = "http://www.pyrodesktop.org/compzilla";
var _focusedFrame;


var FrameMethods = {
    destroy: function () {
	Debug ("frame", "frame.destroy");

	if (this._observer) {
	    try {
		this.content.nativeWindow.removeObserver (this._observer);
	    } catch (e) {
		// Observer may have been automatically removed
	    }
	    this._observer = null;
	}

	if (this._content) {
	    if (this._content.ondestroy)
		this._content.ondestroy ();
            this._content = null;
	}

	if (_focusedFrame == this)
	    _focusedFrame = null;

	windowStack.removeWindow (this);
    },

    moveResize: function (x, y, width, height) {
        // Coordinates are relative to the frame

	if (this._moveResize (x, y, width, height)) {
	    this._updateContentSize ();
        }
    },

    moveResizeToContent: function (x, y, width, height) {
        // ev coords are relative to content, adjust for frame offsets

        Debug ("frame",
	       "moveResizeToContent: [x:" + x + ", y:" + y + 
	       ", width:" + width + ", height:" + height + "]");

	var wasVisible = this.visible;
	if (!wasVisible) {
	    this.style.display = "block";
	    this.style.visibility = "hidden";
	}

        var posframe = this.getPosition ();
        var pos = this._content.getPosition ();
        x -= posframe.left - pos.left;

	// FIXME: Hardcoded height fixes menu position.
        //y -= posframe.top - pos.top;
	y -= 30;

        width += this.offsetWidth - this._content.offsetWidth;
        height += this.offsetHeight - this._content.offsetHeight;

	// Constrain to size of workarea
	x = Math.max (x, workarea.bounds.left);
	y = Math.max (y, workarea.bounds.top);
	width = Math.min (width, workarea.bounds.width);
	height = Math.min (height, workarea.bounds.height);

        this.moveResize (x, y, width, height);

	if (!wasVisible) {
	    this.style.display = "none";
	    this.style.visibility = "visible";
	}
    },

    _moveResize: function (x, y, width, height) {
        // Coordinates are relative to the frame

        Debug ("frame",
	       "_moveResize: [x:" + x + ", y:" + y + 
	       ", width:" + width + ", height:" + height + "]");

        var changed = false;

	if (this.offsetLeft != x) {
	    this.style.left = x + "px";
            changed = true;
	}

	if (this.offsetTop != y) {
	    this.style.top = y + "px";
            changed = true;
	}

        if (this.offsetWidth != width) {
            this.style.width = width + "px";
            changed = true;
        }

        if (this.offsetHeight != height) {
            this.style.height = height + "px";
            changed = true;
        }

        return changed;
    },

    _ignoreConfigureCount: 0,

    _updateContentSize: function () {
	if (this._content.onmoveresize) {
	    if (!this.overrideRedirect) 
		this._ignoreConfigureCount++;
	    this._content.onmoveresize (this.overrideRedirect);
	}
    },

    _resetChromeless: function () {
        if (this.overrideRedirect ||
            this._content.wmWindowType == "dock" ||
            this._content.wmWindowType == "desktop" ||
            this._content.wmWindowType == "splash" ||
            this._content.wmWindowType == "toolbar") {
            this.chromeless = true;
        } else {
            this.chromeless = false;
        }
    },

    _updateStrutInfo: function () {
	workarea.UpdateStrutInfo (this, this._content.wmStruts);
    },

    _recomputeAllowedActions: function () {
	/* this only applies for native windows, and those windows
	   which have had mwm hints/wmWindowType set. */
	if (!this._content.wmWindowType)
	    return;

	// XXX first consider the mwm hints

	// next override using metacity's rules for assigning features
	// based on EWMH's window types.
        var isSpecial = 
            (this._content.wmWindowType == "desktop" ||
             this._content.wmWindowType == "dock" ||
             this._content.wmWindowType == "splash");
        var isNormal = 
            this._content.wmWindowType == "normal";

        this.allowClose = !isSpecial;
        this.allowShade = !isSpecial;
        this.allowMove = !isSpecial;
        this.allowResize = !isSpecial;

        this.allowMaximize = isNormal;
        this.allowMinimize = isNormal;
        this.allowFullscreen = isNormal;

	if (!this.allowResize) {
	    this.allowMaximize = false;
	    this.allowFullscreen = false;
	}

	// we don't allow maximized windows to be moved or resized
	if (this.windowState == "maximized") {
	    this.allowMove = false;
	    this.allowResize = false;
	}
    },

    show: function () {
	if (this.style.display == "block")
	    return;

	// this happens when we uniconify a window
	if (this.windowState != "normal")
	    this.restore();

	Debug ("frame", "frame.show");

	if (!this.overrideRedirect && this._content.onshow) {
	    this._content.onshow ();
	}

	this.style.display = "block";

	this._updateContentSize ();
    },

    hide: function () {
	if (this.style.display == "none")
	    return;

	Debug ("frame", "frame.hide");

	if (!this.overrideRedirect && this._content.onhide) {
	    this._content.onhide ();
	}

	this.style.display = "none";
    },

    minimize: function () {
	if (this.windowState == "minimized")
	    return;

	this.restoreBounds = { left: this.offsetLeft,
			       top: this.offsetTop,
			       width: this.offsetWidth,
			       height: this.offsetHeight };

	this.windowState = "minimized";

	MinimizeCompzillaFrame (this);

	if (this._content.onminimize)
	    this._content.onminimize ();
    },

    maximize: function () {
	if (this.windowState == "maximized")
	    return;

	this.restoreBounds = { left: this.offsetLeft,
			       top: this.offsetTop,
			       width: this.offsetWidth,
			       height: this.offsetHeight };

	MaximizeCompzillaFrame (this);

	if (this._content.onmaximize)
	    this._content.onmaximize ();
    },

    toggleMaximized: function () {
	if (this.windowState == "maximized")
	    this.restore ();
	else if (this.windowState == "normal")
	    this.maximize ();
    },

    restore: function () {
	if (this.windowState == "normal")
	    return;

	RestoreCompzillaFrame (this);
    },

    fullscreen: function () {
	if (this.windowState == "fullscreen")
	    return;

	this.restoreBounds = { left: this.offsetLeft,
			       top: this.offsetTop,
			       width: this.offsetWidth,
			       height: this.offsetHeight };

	this.windowState = "fullscreen";

	// XXX this should do more, like hide the chrome, or resize
	// the window such that the chrome is offscreen.
	this.moveResize (workarea.bounds.left,
			 workarea.bounds.top,
			 workarea.bounds.width,
			 workarea.bounds.height);

	if (this._content.onfullscreen)
	    this._content.onfullscreen ();
    },

    getPyroAttribute: function (name) {
        return this.getAttributeNS (_PYRO_NAMESPACE, name);
    },

    setPyroAttribute: function (name, value) {
        this.setAttributeNS (_PYRO_NAMESPACE, name, value);
    },

    mapPropertyToPyroAttribute: function (propname, attrname) {
	this.__defineGetter__ (propname, 
			       function () { 
				   return this.getPyroAttribute (attrname); 
			       });
	this.__defineSetter__ (propname, 
			       function (val) { 
				   this.setPyroAttribute (attrname, val); 
			       });
    }
};


function _addFrameMethods (frame)
{
    jQuery.extend (frame, FrameMethods);

    // now add our public properties

    // These map directly to pyro:attributes on the frame
    frame.mapPropertyToPyroAttribute ("allowResize", "allow-resize");
    frame.mapPropertyToPyroAttribute ("allowMaximize", "allow-maximize");
    frame.mapPropertyToPyroAttribute ("allowMinimize", "allow-minimize");
    frame.mapPropertyToPyroAttribute ("allowClose", "allow-close");
    frame.mapPropertyToPyroAttribute ("allowMove", "allow-move");
    frame.mapPropertyToPyroAttribute ("allowShade", "allow-shade");
    frame.mapPropertyToPyroAttribute ("inactive", "inactive");
    frame.mapPropertyToPyroAttribute ("moving", "moving");
    frame.mapPropertyToPyroAttribute ("windowState", "window-state");

    /* XXX we should rethink the name of this */
    frame.mapPropertyToPyroAttribute ("wmClass", "wm-class");


    frame.addProperty ("visible",
		       /* getter */
		       function () {
			   return this.style.display == "block";
                       },
		       /* setter */
		       function (val) {
			   if (val != this.visible) {
			       val ? this.show() : this.hide ();
			   }
		       });

    frame.addProperty ("chromeless",
		       /* getter */
		       function () {
			   $(this).is ("chromeless");
                       },
		       /* setter */
		       function (flag) {
			   if (flag)
			       $(this).addClass ("chromeless");
			   else
			       $(this).removeClass ("chromeless");
		       });

    frame.addProperty ("content",
		       /* getter */
		       function () {
			   return this._content;
		       }
		       /* no setter */
		       );

    frame.addProperty ("overrideRedirect",
		       /* getter */
		       function () {
                           return this._overrideRedirect;
		       },
		       /* setter */
		       function (val) {
			   if (this._overrideRedirect != val) {
			       this._overrideRedirect = val;
			       this._resetChromeless ();
			   }
		       });

    frame.addProperty ("title",
		       /* getter */
		       function () {
			   return this._title ? this._title.value : null;
		       },
		       /* setter */
		       function (t) {
			   if (!this._title)
			       return;

			   if (!t || t == "")
			       t = "[Unknown]";

			   if (t == this._title.getAttributeNS (_PYRO_NAMESPACE, "caption"))
			       return;

			   Debug ("setting caption of " + this._title + " to " + t);

			   this._title.setAttributeNS (_PYRO_NAMESPACE, "caption", t);
			   for (var el = this._title.firstChild; el; el = el.nextSibling) {
			       if (el.className == "windowTitleSpan")
				   el.innerHTML = t;
			   }
		       });
}


function CompzillaFrame (content)
{
    var frame = $("#windowFrame").clone()[0];

    frame._contentBox = $("#windowContentBox", frame)[0];
    frame._contentBox.appendChild (content);
    frame._content = content;

    frame._icon = $("#windowIcon", frame)[0];

    frame._titleBox = $("#windowTitleBox", frame)[0];
    frame._title = $("#windowTitle", frame)[0];

    // Add our methods
    _addUtilMethods (frame);
    _addFrameMethods (frame);
    
    if (content.nativeWindow) {
        frame.id = "XID:" + content.nativeWindow.nativeWindowId;

	frame._observer = _observeNativeWindow (frame);

	frame._updateStrutInfo ();

	frame._resetChromeless ();
        frame._recomputeAllowedActions ();

	frame.title = content.wmName;
    }

    if (frame._title) {
	_connectFrameDragListeners (frame);
    }

    _connectFrameFocusListeners (frame);

    return frame;
}


function _connectFrameFocusListeners (frame)
{
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
                        _focusedFrame.inactive = true;
			if (_focusedFrame._content.onlostfocus)
			    _focusedFrame._content.onlostfocus ();
                    }

                    // Send FocusIn
                    frame._content.focus ();
                    frame.inactive = false;

                    _focusedFrame = frame;
		    if (_focusedFrame._content.ongotfocus)
			_focusedFrame._content.ongotfocus ();
                }
            }
        },
	true);
}


function _connectFrameDragListeners (frame)
{
    var frameDragPosition = new Object ();

    var frameDragMouseMoveListener = {
        handleEvent: function (ev) {
	    // figure out the deltas
	    var dx = ev.clientX - frameDragPosition.x;
	    var dy = ev.clientY - frameDragPosition.y;

            // Don't do anything if no delta.  
            // Toshok was seeing this Minefield, but not with FF2.
	    if (dx == 0 && dy == 0) {
		return;
	    }

	    frameDragPosition.x = ev.clientX;
	    frameDragPosition.y = ev.clientY;

            var rect = { x:      frame.offsetLeft,
			 y:      frame.offsetTop,
			 width:  frame.offsetWidth,
			 height: frame.offsetHeight };

	    if (frameDragPosition.operation == "move") {
                frame.moving = true;

                rect.x += dx;
                rect.y += dy;
	    } else {
		// calculate y/height
		switch (frameDragPosition.operation) {
		case "nw":
		case "ne":
		case "n":
		    rect.height -= dy;
		    rect.y += dy;
		    break;
		case "sw":
		case "se":
		case "s":
		    rect.height += dy;
		    break;
		}

		// calculate x/width
		switch (frameDragPosition.operation) {
		case "nw":
		case "sw":
		case "w":
		    rect.width -= dx;
		    rect.x += dx;
		    break;
		case "ne":
		case "se":
		case "e":
		    rect.width += dx;
		    break;
		}
	    }

	    frame._ignoreConfigureCount++;
	    frame.moveResize (rect.x, rect.y, rect.width, rect.height);

	    ev.preventDefault ();
	    ev.stopPropagation ();
	}
    };

    var frameDragMouseUpListener = {
	handleEvent: function (ev) {
	    frame.moving = false;

	    // clear the event handlers we add in the title mousedown handler below
	    document.removeEventListener ("mousemove", frameDragMouseMoveListener, true);
	    document.removeEventListener ("mouseup", frameDragMouseUpListener, true);

	    ev.preventDefault ();
	    ev.stopPropagation ();
	}
    };

    frame.onmousedown = function (ev) {
        var op = ev.target.getAttributeNS (_PYRO_NAMESPACE, "resize");
        if (!op)
            return;

        if (op == "move" && !frame.allowMove)
            return;
        else if (!frame.allowResize)
            return;

        frameDragPosition.operation = op;
	frameDragPosition.x = ev.clientX;
	frameDragPosition.y = ev.clientY;

	document.addEventListener ("mousemove", frameDragMouseMoveListener, true);
	document.addEventListener ("mouseup", frameDragMouseUpListener, true);
	ev.stopPropagation ();
    };
}


/* 
 * XXX this should either be moved to content.js, or content.js
 * should also listen for events it cares about (like
 * propertychange) 
 */

function _observeNativeWindow (frame)
{
    var observer = {
        destroy: function () {
	    Debug ("frame", "destroy.handleEvent (" + frame._content.nativeWindow + ")");
	    frame.destroy ();
	},

        configure: function (mapped,
			     overrideRedirect,
			     x,
			     y,
			     width,
			     height,
			     borderWidth,
			     aboveWindow) {
	    if (!overrideRedirect && frame._ignoreConfigureCount > 0) {
		frame._ignoreConfigureCount--;
		return;
	    }

	    Debug ("frame", "configure.handleEvent");

	    // track override changes
	    frame.overrideRedirect = overrideRedirect;

	    // This may not match the current state if the window was created
	    // before compzilla.
	    frame.visible = mapped;

	    frame.moveResizeToContent (x, y, width, height);

	    // XXX handle stacking requests here too
	},

        map: function (overrideRedirect) {
	    Debug ("frame", "map.handleEvent");

	    // track override changes
	    frame.overrideRedirect = overrideRedirect;

	    windowStack.moveToTop (frame);
	    frame.visible = true;
	},

        unmap: function () {
	    Debug ("frame", "unmap.handleEvent");
	    frame.visible = false;
	},

	propertyChange: function (atom, isDeleted) {
	    Debug ("frame", "propertychange.handleEvent: atom=" + atom);

	    switch (atom) {
	    case Atoms.XA_WM_NAME:
	    case Atoms._NET_WM_NAME:
		frame.title = frame._content.wmName;
		
		Debug ("frame", "propertychange: setting title:" + frame.title);
		break;

	    case Atoms.XA_WM_ICON_NAME:
	    case Atoms._NET_WM_ICON_NAME:
		frame.title = frame._content.wmIconName;

		Debug ("frame", "propertychange: new iconified title:" + frame.title);
		break;

	    case Atoms._NET_WM_ICON:
		frame._icon.src = frame._content.wmIcon;

		Debug ("frame", "propertychange: new icon!");
		break;

	    case Atoms._NET_WM_STRUT:
		frame._updateStrutInfo ();
		break;

	    case Atoms._NET_WM_WINDOW_TYPE:
		frame._resetChromeless ();
		frame._recomputeAllowedActions ();

		Debug ("frame", "propertychange: setting window type:" + 
		       frame._content.wmWindowType);
		break;

	    case Atoms.XA_WM_CLASS:
		frame.wmClass = frame._content.wmClass;

		Debug ("frame", "propertychange: setting wmClass: '" +  
		       frame.wmClass + "'");
		break;
	    }
	},

	clientMessageRecv: function (messageType, format, d1, d2, d3, d4) {
	    Debug ("frame", "clientmessage type:" + messageType + 
		   " [d1:"+d1 + ", d2:"+d2 + ", d3:"+d3 + ", d4:"+d4 + "]");

	    switch (messageType) {
	    case Atoms._NET_CLOSE_WINDOW:
		frame.close ();
		break;

	    case Atoms._NET_MOVERESIZE_WINDOW:
		/* d1 == gravity and flags */
		/* d2 == x */
		/* d3 == y */
		/* d4 == width */
		/* d5 == height */
		break;

	    case Atoms._NET_WM_MOVERESIZE:
		/* d1 = x_root */
		/* d2 = y_root */
		/* d3 = direction */
		/* d4 = button */
		/* d5 = source indication */
		break;

	    case Atoms._NET_RESTACK_WINDOW:
		/* d1 == source indication */
		/* d2 == sibling window */
		/* d3 == detail */
		break;

	    case Atoms._NET_REQUEST_FRAME_EXTENTS:
		/* The Window Manager MUST respond by estimating
		   the prospective frame extents and setting the
		   window's _NET_FRAME_EXTENTS property
		   accordingly. */
		break;

	    case Atoms._NET_WM_DESKTOP:
		/* A Client can request a change of desktop for a
		   non-withdrawn window by sending a
		   _NET_WM_DESKTOP client message to the root
		   window: */

		/* d1 == new_desktop */
		/* d2 == source indication */
		break;

	    case Atoms._NET_WM_STATE:
		/* d1 == the action, _NET_WM_STATE_{REMOVE,ADD,TOGGLE} */
		/* d2 == first property to alter */
		/* d3 == second property to alter */
		break;
	    }
	}
    };

    /*
     * observer.configure will be called immediately to inform
     * of the current state.
     */
    try {
	frame.content.nativeWindow.addObserver (observer);
	return observer;
    } catch (e) {
	Debug ("ERROR adding observer: " + e);
	return null;
    }
};

