/* -*- mode: javascript; c-basic-offset: 4; indent-tabs-mode: t; -*- */


function CompzillaFrame (content) {
    var frame = document.getElementById ("windowFrame").cloneNode (true);
    frame.appendChild (content);

    frame._content = content;
    frame.getContent = function () { return frame._content; }

    frame.destroy = function () {
	if (content.destroy) {
	    content.destroy ();
	}
	windowStack.removeWindow (frame);
    }

    frame._title = frame.getElementsByTagName("label")[0];
    frame.setTitle = function (title) { frame._title.value = title; }

    frame.moveResize = function (width, height, x, y) {
	if (content.offsetWidth != width) {
	    content.style.width = width + "px";
	    frame.style.width = width + "px";
	}
	if (content.offsetHeight != height) {
	    content.style.height = height + "px";
	    frame.style.width = width + "px";
	}
	if (frame.offsetLeft != x) {
	    frame.style.left = x + "px";
	}
	if (frame.offsetTop != y) {
	    frame.style.top = y + "px";
	}
    };

    frame.show = function () {
	frame.style.visibility = "visible";
	content.style.visibility = "visible";
    };

    frame.hide = function () {
	frame.style.visibility = "hidden";
	content.style.visibility = "hidden";
    };

    if (content.getNativeWindow) {
	var nativewin = content.getNativeWindow ();

	nativewin.addEventListener (
	    "destroy", 
	    { 
		frame: frame, 
	        handleEvent: function (ev) {
		    frame.destroy ();
		}
	    },
	    true);
	nativewin.addEventListener (
	    "moveresize", 
	    { 
		frame: frame, 
	        handleEvent: function (ev) {
		    frame.moveResize (ev.width, ev.height, ev.x, ev.y);
		}
	    },
	    true);
	nativewin.addEventListener (
	    "show", 
	    { 
		frame: frame, 
	        handleEvent: function (ev) {
		    frame.show ();
		}
	    },
	    true);
	nativewin.addEventListener (
	    "hide", 
	    { 
		frame: frame, 
	        handleEvent: function (ev) {
		    frame.hide ();
		}
	    },
	    true);
	nativewin.addEventListener (
	    "propertychange", 
	    { 
		frame: frame, 
	        handleEvent: function (ev) {
		    if (ev.atom == Atoms.WM_NAME () ||
			ev.atom == Atoms._NET_WM_NAME ()) {
			frame.setTitle (ev.value.getPropertyAsAString (".text"));
		    }
		}
	    },
	    true);
    }

    return frame;
}

