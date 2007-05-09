/* -*- mode: javascript; c-basic-offset: 4; indent-tabs-mode: t; -*- */


function CompzillaFrame (content) {
    frame = document.getElementById ("windowFrame").cloneNode (true);

    frame._content = content;
    frame.getContent = function () { return frame._content; }
    frame.appendChild (content);

    frame.setTitle = function (title) {
	//frame.getElement()
    }

    frame.moveResize = function (width, height, x, y) {
	if (content.offsetWidth != width) {
	    content.style.width = width + "px";
	}
	if (content.offsetHeight != height) {
	    content.style.height = height + "px";
	}
	if (frame.offsetLeft != x) {
	    frame.style.left = x + "px";
	}
	if (frame.offsetTop != y) {
	    frame.style.top = y + "px";
	}
    }

    frame.show = function () {
	frame.style.visibility = "visible";
	content.style.visibility = "visible";
    }

    frame.hide = function () {
	frame.style.visibility = "hidden";
	content.style.visibility = "hidden";
    }

    if (content.getNativeWin) {
	nativewin = content.getNativeWin ();
	nativewin.addEventListener (
	    "destroy", 
	    { 
		frame: frame, 
	        handleEvent: function (ev) {
		    windowDestroy (this.frame);
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
		    // Nothing yet.
		}
	    },
	    true);
    }

    return frame;
}

