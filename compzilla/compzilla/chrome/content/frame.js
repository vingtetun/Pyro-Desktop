/* -*- mode: javascript; c-basic-offset: 4; indent-tabs-mode: t; -*- */

function getDescendentById (root, id)
{
    if (root.id == id)
	return root;

    for (var el = root.firstChild; el != null; el = el.nextSibling) {
	var rv = getDescendentById (el, id);
	if (rv != null)
	    return rv;
    }

    return null;
}

function _CompzillaFrameCommon (content, override)
{
    var frame = document.getElementById (override ? "dockFrame" : "windowFrame").cloneNode (true);

    frame._content = content;
    frame.getContent = function () { return frame._content; };

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
    frame.setTitle = function (title) { if (frame._title) frame._title.value = title; }
    frame.getTitle = function () { return frame._title ? frame._title.value : null; }

    frame.moveResize = function (width, height, x, y) {
	Debug ("frame.moveResize: w=" + width + ", h=" + height);

	if (contentBox.style.width != width) {
	    contentBox.style.width = width + "px";
	    content.width = width;
	}
	if (contentBox.style.height != height) {
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
	connectNativeWindowListeners (frame, content);
    }

    return frame;
}

function CompzillaFrame (content)
{
    return _CompzillaFrameCommon (content, false);
}

function CompzillaDockFrame (content)
{
    return _CompzillaFrameCommon (content, true);
}

function connectNativeWindowListeners (frame, content) 
{
    var nativewin = content.getNativeWindow ();

    nativewin.addEventListener (
        "destroy", 
        { 
	    handleEvent: function (ev) {
		Debug ("destroy.handleEvent");
		frame.destroy ();
	    }
	},
	true);
    nativewin.addEventListener (
        "moveresize", 
        { 
	    handleEvent: function (ev) {
		Debug ("moveresize.handleEvent");
		frame.moveResize (ev.width, ev.height, ev.x, ev.y);
	    }
	},
	true);
    nativewin.addEventListener (
	"show", 
	{ 
	    handleEvent: function (ev) {
		Debug ("show.handleEvent");
		frame.show ();
	    }
	},
	true);
    nativewin.addEventListener (
	"hide", 
        { 
	    handleEvent: function (ev) {
		Debug ("hide.handleEvent");
		frame.hide ();
	    }
	},
	true);
    nativewin.addEventListener (
	"propertychange", 
	{ 
	    handleEvent: function (ev) {
		Debug ("propertychange.handleEvent: ev.atom=" + ev.atom);

		if (ev.atom == Atoms.WM_NAME () ||
		    ev.atom == Atoms._NET_WM_NAME ()) {
		    name = ev.value.getProperty(".text");

		    Debug ("propertychange: setting title =" + name);
		    frame.setTitle (name);
		}
	    }
	},
	true);    
}
