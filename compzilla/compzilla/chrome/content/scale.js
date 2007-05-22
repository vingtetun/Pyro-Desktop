/* -*- mode: javascript; c-basic-offset: 4; indent-tabs-mode: t; -*- */

/* adapted from compiz's scale plugin */

var exposeLayer = document.getElementById ("exposeLayer");

var ss = new Object ();

/* for now, just always say windows should be scaled */
function isScaleWin (w) {
    return true;
}

function layoutSlots () {

    ss.slots = new Array ();
    var nSlots = 0;

    var lines = Math.floor (Math.sqrt (ss.windows.length + 1));

    var x1 = 0;
    var y1 = 0;
    var x2 = windowStack.offsetWidth;
    var y2 = windowStack.offsetHeight;

    y      = y1 + ss.spacing;
    height = Math.floor (((y2 - y1) - (lines + 1) * ss.spacing) / lines);

    for (i = 0; i < lines; i++) {
	n = Math.min (ss.windows.length - nSlots,
		      Math.ceil (ss.windows.length / lines));

	x     = x1 + ss.spacing;
	width = Math.floor (((x2 - x1) - (n + 1) * ss.spacing) / n);

	for (var j = 0; j < n; j++) {
	    var new_slot = new Object ();
	    new_slot.x1 = x;
	    new_slot.y1 = y;
	    new_slot.x2 = x + width;
	    new_slot.y2 = y + height;
	    new_slot.filled = false;

	    nSlots = ss.slots.push (new_slot);

	    x += width + ss.spacing;
	}

	y += height + ss.spacing;
    }
}

function findBestSlots () {

    var d;
    var d0 = 0;

    var sx, sy, cx, cy;

    for (var i = 0; i < ss.windows.length; i++) {
	w = ss.windows[i];

	if (w.slot != null)
	    continue;

	w.sid      = 0;
	w.distance = /* XXX something suitably large since there's no MAXSHORT */ 9999999;

	for (var j = 0; j < ss.slots.length; j++) {
	    if (!ss.slots[j].filled) {
		sx = (ss.slots[j].x2 + ss.slots[j].x1) / 2;
		sy = (ss.slots[j].y2 + ss.slots[j].y1) / 2;

		cx = w.orig_left + w.orig_width  / 2;
		cy = w.orig_top + w.orig_height / 2;

		cx -= sx;
		cy -= sy;

		d = Math.sqrt (cx * cx + cy * cy);

		if (d0 + d < w.distance) {
		    w.sid      = j;
		    w.distance = Math.floor (d0 + d);
		}
	    }
	}

	d0 += w.distance;
    }
}

function fillInWindows () {
    var width, height;
    var sx, sy, cx, cy;

    for (i = 0; i < ss.windows.length; i++) {
	w = ss.windows[i];

	if (w.slot == null) {
	    if (ss.slots[w.sid].filled)
		return true;

	    w.slot = ss.slots[w.sid];

	    width  = w.offsetWidth  /*+ w.input.left + w.input.right*/;
	    height = w.offsetHeight /*+ w.input.top  + w.input.bottom*/;

	    sx = (w.slot.x2 - w.slot.x1) / width;
	    sy = (w.slot.y2 - w.slot.y1) / height;

	    w.slot.scale = Math.min (Math.min (sx, sy), 1.0);

	    sx = width  * w.slot.scale;
	    sy = height * w.slot.scale;
	    cx = (w.slot.x1 + w.slot.x2) / 2;
	    cy = (w.slot.y1 + w.slot.y2) / 2;

	    //cx += w.input.left * w.slot.scale;
	    //cy += w.input.top  * w.slot.scale;

	    w.slot.x1 = cx - sx / 2;
	    w.slot.y1 = cy - sy / 2;
	    w.slot.x2 = cx + sx / 2;
	    w.slot.y2 = cy + sy / 2;

	    w.slot.filled = true;

	    w.lastThumbOpacity = 0.0;

	    w.adjust = true;
	}
    }

    return false;
}

function compareWindowsDistance (a, b)
{
    return a.distance - b.distance;
}

function layoutThumbs () {
    ss.windows = new Array ();

    /* add windows scale list, top most window first */
    for (var swi = 0; swi < ss.reverseWindows.length; swi ++) {
	var sw = ss.reverseWindows[swi];

	if (sw.slot != null)
	    sw.adjust = true;

	sw.slot = null;

	if (!isScaleWin (sw))
	    continue;

	ss.windows.push (sw);
    }

    if (ss.windows.length == 0)
	return false;

    /* create a grid of slots */
    layoutSlots ();

    do {
	/* find most appropriate slots for windows */
	findBestSlots ();

	ss.windows.sort (compareWindowsDistance);

    } while (fillInWindows ());

    return true;
}

function scaleTerminate () {
    if (ss.state != "none") {

	var callback_count = 0;

	for (var swi = 0; swi < ss.windows.length; swi++) {
	    var sw = ss.windows[swi];
	    if (sw.slot != null)
		callback_count ++;
	}

	for (var swi = 0; swi < ss.windows.length; swi++) {
	    var sw = ss.windows[swi];
	    if (sw.slot != null) {
		sw.slot = null;

		// animate the scaled windows back to their original spots
		$(sw).animate ( {left: sw.orig_left,
				 top: sw.orig_top,
				 width: sw.orig_width,
				 height: sw.orig_height } ,
				250, "linear",
				function () {
				    if (--callback_count == 0) {
					for (var i = 0; i < ss.windows.length; i ++) {
					    ss.windows[i].destroy ();
					    exposeLayer.removeChild (ss.windows[i]);
					}
					ss.reverseWindows = new Array ();
					ss.state = "none";
					Debug ("made it to none state");

					// animate the expose layer fading out
					$(exposeLayer).animate ( { opacity: 0.0 }, 100,
								 "linear",
								 function () {
								     exposeLayer.style.display = "none";
								 });
				    }
				});

		// also animate the original window fading in, so that it
		// ends a little bit after the movement animations.
		$(sw.orig_window).animate ({ opacity: sw.orig_opacity }, 350);
	    }
	}
    }

    return false;
}

function scaleInitiateCommon ()
{
    ss.state = "out";

    if (!layoutThumbs ()) {
	ss.state = "wait";
	return false;
    }

    var callback_count = 0;

    for (var swi = 0; swi < ss.windows.length; swi++) {
	var sw = ss.windows[swi];
	if (sw.slot != null)
	    callback_count ++;
    }

    exposeLayer.style.opacity = 0.0;
    exposeLayer.style.display = "block";
    $(exposeLayer).animate ( { opacity: 0.5 }, 250 );

    for (var swi = 0; swi < ss.windows.length; swi++) {
	var sw = ss.windows[swi];
	if (sw.slot != null) {

	    // animate the contents moving to the right place
	    $(sw).animate ({ left: sw.slot.x1,
			     top: sw.slot.y1,
			     width: sw.slot.x2-sw.slot.x1,
			     height: sw.slot.y2-sw.slot.y1 },
			   250, "linear",
			   function () {
			       if (--callback_count == 0) {
				   ss.state = "wait";
				   Debug ("made it to wait state");
			       }
			   });

	    // and also animate the original window fading out, very quickly
	    $(sw.orig_window).animate ({ opacity: 0.0 }, 100);
	}
    }

    return false;
}

function scaleInitiateAll ()
{
    if (ss.state != "wait" && ss.state != "out") {
	ss.type = "all";
	return scaleInitiateCommon ();
    }
    
    return false;
}

function addWindow (w) {
    var sw = CompzillaWindowContent (w.content.nativeWindow);
    sw.orig_window = w;

    sw.orig_left = w.offsetLeft;
    sw.orig_top = w.offsetTop;
    sw.orig_width = w._contentBox.offsetWidth; /* XXX need a getter */
    sw.orig_height = w._contentBox.offsetHeight;
    sw.orig_opacity = 1.0;

    Debug ("orig opacity = " + w.opacity);

    sw.style.left = sw.orig_left + "px";
    sw.style.top = sw.orig_top + "px";
    sw.style.width = sw.orig_width + "px";
    sw.style.height = sw.orig_height + "px";
    sw.width = w.content.width;
    sw.height = w.content.height;

    sw.scale = 1;
    sw.tx = sw.ty = 0;
    sw.xVelocity = sw.yVelocity = 0;
    sw.scaleVelocity = 1;

    sw.style.position = "absolute";

    exposeLayer.appendChild (sw);

    ss.reverseWindows.push (sw);
}

function scaleStart () {
    exposeLayer.style.display = "block";

    for (var el = windowStack.firstChild; el != null; el = el.nextSibling) {
	if (el.className == "windowFrame" /* it's a compzillaWindowFrame */
	    && el.content.nativeWindow /* it has native content */
	    && el.style.display == "block" /* it's displayed */) {
	    addWindow (el);
	}
    }

    scaleInitiateAll ();
}

ss.state = "none";
ss.reverseWindows = new Array ();
ss.spacing = 10;
ss.opacity = 1.0;
ss.speed = 1.5;
ss.timestep = 1.2;

if (exposeLayer != null)
    document.addEventListener("keypress", {
                              handleEvent: function (event) {
				  if (event.keyCode == event.DOM_VK_F11 && event.ctrlKey) {
				      if (ss.state == "none") {
					  scaleStart ();
				      } else {
					  scaleTerminate ();
                                      }
				  }

				  event.stopPropagation ();
			      } },
			      true);
