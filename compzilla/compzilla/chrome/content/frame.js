/* -*- mode: javascript; c-basic-offset: 4; indent-tabs-mode: t; -*- */


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


var FrameMethods = {
    destroy: function () {
	Debug ("frame",
	       "frame.destroy");

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

	Debug ("frame", 
	       "frame.moveResize: x=" + x + ", y=" + y + ", w=" + width + ", h=" + height);

	if (this._moveResize (x, y, width, height)) {
	    if (this._content.onmoveresize)
		this._content.onmoveresize (this.overrideRedirect);
        }
    },


    _moveResize: function (x, y, width, height) {
        // Coordinates are relative to the frame

        /*
        Debug ("BEFORE _moveResize: " + 
               " this._content.offsetHeight=" + this._content.offsetHeight + 
               " this._content.offsetWidth=" + this._content.offsetWidth);
        */

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

        Debug ("frame", 
               "AFTER _moveResize: " + 
               " this._content.offsetHeight=" + this._content.offsetHeight + 
               ", this._content.offsetWidth=" + this._content.offsetWidth);

        return changed;
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
    },

    show: function () {
	if (this.style.display == "block")
	    return;

	Debug ("frame", "frame.show");

	if (this._content.onshow)
	    this._content.onshow();

	this.style.display = "block";
    },


    hide: function () {
	if (this.style.display == "none")
	    return;

	Debug ("frame", "frame.hide");

	this.style.display = "none";

	if (this._content.onhide)
	    this._content.onhide();
    },

    maximize: function () {

	// XXX more stuff here

	if (this._content.onmaximize)
	    this._content.onmaximize();
    },

    fullscreen: function () {

	// XXX more stuff here

	if (this._content.onfullscreen)
	    this._content.onfullscreen ();
    },

    getPyroAttribute: function (name) {
        return this.getAttributeNS ("http://www.pyrodesktop.org/compzilla", name);
    },


    setPyroAttribute: function (name, value) {
        this.setAttributeNS ("http://www.pyrodesktop.org/compzilla", name, value);
    },


    addProperty: function (name, getter, setter) {
	this.__defineGetter__ (name, getter);

	/* allow setter to be undefined for read-only properties */
	if (setter != undefined)
	    this.__defineSetter__ (name, setter);
    },

    mapPropertyToPyroAttribute: function (propname, attrname) {
	this.__defineGetter__ (propname, function () { return this.getPyroAttribute (attrname); });
	this.__defineSetter__ (propname, function (val) { this.setPyroAttribute (attrname, val); });
    }

};


function _addFrameMethods (frame)
{
    for (var m in FrameMethods) {
	frame[m] = FrameMethods[m];
    }

    // now add our public properties

    // These map directly to pyro:attributes on the frame
    frame.mapPropertyToPyroAttribute ("allowResize", "allow-resize");
    frame.mapPropertyToPyroAttribute ("allowMaximize", "allow-maximize");
    frame.mapPropertyToPyroAttribute ("allowMinimize", "allow-minimize");
    frame.mapPropertyToPyroAttribute ("allowMove", "allow-move");
    frame.mapPropertyToPyroAttribute ("allowShade", "allow-shade");
    frame.mapPropertyToPyroAttribute ("inactive", "inactive");
    frame.mapPropertyToPyroAttribute ("moving", "moving");
    frame.mapPropertyToPyroAttribute ("wmClass", "wm-class"); /* XXX we should rethink the name of this */


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
                           this._overrideRedirect = val;
                           this._resetChromeless ();
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

			   if (t == this._title.getAttributeNS ("http://www.pyrodesktop.org/compzilla", 
								"caption"))
			       return;

			   Debug ("setting caption of " + this._title + " to " + t);

			   this._title.setAttributeNS ("http://www.pyrodesktop.org/compzilla", 
						       "caption", 
						       t);
			   for (var el = this._title.firstChild; el; el = el.nextSibling) {
			       if (el.className == "windowTitleSpan")
				   el.innerHTML = t;
			   }
		       });
}


function CompzillaFrame (content)
{
    var frame = document.getElementById ("windowFrame").cloneNode (true);

    frame._contentBox = getDescendentById (frame, "windowContentBox");
    frame._contentBox.appendChild (content);
    frame._content = content;

    frame._titleBox = getDescendentById (frame, "windowTitleBox");
    frame._title = getDescendentById (frame, "windowTitle");

    // Add our methods
    _addFrameMethods (frame);
    
    if (content.nativeWindow) {
        frame.id = "XID:" + content.nativeWindow.nativeWindowId;

	_connectNativeWindowListeners (frame);
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
                    }

                    // Send FocusIn
                    frame._content.focus ();
                    frame.inactive = false;

                    _focusedFrame = frame;
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

            var rect = new Object();
            rect.x = frame.offsetLeft;
            rect.y = frame.offsetTop;
            rect.width = frame.offsetWidth;
            rect.height = frame.offsetHeight;

	    if (frameDragPosition.operation == "move-resize") {
                frame.moving = true;

                rect.x += dx;
                rect.y += dy;
	    } else {
		// calculate y/height
		switch (frameDragPosition.operation) {
		case "nw-resize":
		case "ne-resize":
		case "n-resize":
		    rect.height -= dy;
		    rect.y += dy;
		    break;
		case "sw-resize":
		case "se-resize":
		case "s-resize":
		    rect.height += dy;
		    break;
		}

		// calculate x/width
		switch (frameDragPosition.operation) {
		case "nw-resize":
		case "sw-resize":
		case "w-resize":
		    rect.width -= dx;
		    rect.x += dx;
		    break;
		case "ne-resize":
		case "se-resize":
		case "e-resize":
		    rect.width += dx;
		    break;
		}
	    }

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
        var op = ev.target.getAttributeNS ("http://www.pyrodesktop.org/compzilla", "resize");
        if (!op)
            return;

        if (op == "move" && !frame.allowMove)
            return;
        else if (!frame.allowResize)
            return;

        frameDragPosition.operation = op + "-resize";
	frameDragPosition.x = ev.clientX;
	frameDragPosition.y = ev.clientY;

	document.addEventListener ("mousemove", frameDragMouseMoveListener, true);
	document.addEventListener ("mouseup", frameDragMouseUpListener, true);
	ev.stopPropagation ();
    };
}


/* XXX this method should either be moved to content.js, or content.js
   should also listen for events it cares about (like
   propertychange) */

function _connectNativeWindowListeners (frame)
{
    var nativewin = frame._content.nativeWindow;

    nativewin.addEventListener (
	"destroy",
        {
	    handleEvent: function (ev) {
		Debug ("frame", "destroy.handleEvent (" + content + ")");
		frame.destroy ();
	    }
	},
	true);

    frame.addEventListener (
	"configure",
	{
	    handleEvent: function (ev) {
		Debug ("frame", "configure.handleEvent");

		// This is the only way we're notified of override changes
		frame.overrideRedirect = ev.overrideRedirect;

		// ev coords are relative to content, adjust for frame offsets
		frame.moveResize (
				  ev.x - frame._content.offsetLeft,
				  ev.y - frame._content.offsetTop,
				  ev.width - frame._content.width + frame.offsetWidth,
				  ev.height - frame._content.height + frame.offsetHeight);

		// XXX handle stacking requests here too
	    }
	},
	true);

    nativewin.addEventListener (
	"map",
	{
	    handleEvent: function (ev) {
		Debug ("frame", "map.handleEvent");
		frame.show ();
	    }
	},
	true);

    nativewin.addEventListener (
	"unmap",
	{
	    handleEvent: function (ev) {
		Debug ("frame", "unmap.handleEvent");
		frame.hide ();
	    }
	},
	true);

    nativewin.addEventListener (
	"propertychange",
	{
	    handleEvent: function (ev) {
		Debug ("frame", "propertychange.handleEvent: ev.atom=" + ev.atom);

		// the property has changed (or been deleted).  nuke it from our cache
		frame._content.XProps.Invalidate (ev.atom);

		switch (ev.atom) {
		case Atoms.XA_WM_NAME:
		case Atoms._NET_WM_NAME:
		    frame.title = frame._content.wmName;

		    Debug ("frame", "propertychange: setting title:" + frame.title);

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
	    }
	},
	true);
};

