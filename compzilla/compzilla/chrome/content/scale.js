/* -*- mode: javascript; c-basic-offset: 4; indent-tabs-mode: t; -*- */

/* adapted from compiz's scale plugin */

var exposeLayer = document.getElementById ("exposeLayer");

var ss = new Object ();

var stepTimeout = 50;

/* for now, just always say windows should be scaled

function isNeverScaleWin (CompWindow *w) {
    if (w->attrib.override_redirect)
	return true;

    if (w->wmType & (CompWindowTypeDockMask | CompWindowTypeDesktopMask))
	return true;

    return false;
}

function isScaleWin (w) {
    SCALE_SCREEN (w->screen);

    if (isNeverScaleWin (w))
	return false;

    if (!ss->type || ss->type == ScaleTypeOutput) {
	if (!(*w->screen->focusWindow) (w))
	    return false;
    }

    if (w->state & CompWindowStateSkipPagerMask)
	return false;

    if (w->state & CompWindowStateShadedMask)
	return false;

    if (!w->mapNum || w->attrib.map_state != IsViewable)
	return false;

    switch (ss->type) {
    case ScaleTypeGroup:
	if (ss->clientLeader != w->clientLeader &&
	    ss->clientLeader != w->id)
	    return false;
	break;
    case ScaleTypeOutput:
	if (outputDeviceForWindow(w) != w->screen->currentOutputDev)
	    return false;
    default:
	break;
    }

    if (!matchEval (ss->currentMatch, w))
	return false;

    return true;
}
*/
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

function adjustScaleVelocity (sw) {
    var dx, dy, ds, adjust, amount;
    var x1, y1, scale;

    if (sw.slot != null) {
	x1 = sw.slot.x1;
	y1 = sw.slot.y1;
	scale = sw.slot.scale;
    }
    else {
	x1 = sw.orig_left;
	y1 = sw.orig_top;
	scale = 1.0;
    }

    dx = x1 - (sw.orig_left + sw.tx);

    adjust = dx * 0.15;
    amount = Math.abs (dx) * 1.5;
    if (amount < 0.5)
	amount = 0.5;
    else if (amount > 5.0)
	amount = 5.0;

    sw.xVelocity = (amount * sw.xVelocity + adjust) / (amount + 1.0);

    dy = y1 - (sw.orig_top + sw.ty);

    adjust = dy * 0.15;
    amount = Math.abs (dy) * 1.5;
    if (amount < 0.5)
	amount = 0.5;
    else if (amount > 5.0)
	amount = 5.0;

    sw.yVelocity = (amount * sw.yVelocity + adjust) / (amount + 1.0);

    ds = scale - sw.scale;

    adjust = ds * 0.1;
    amount = Math.abs (ds) * 7.0;
    if (amount < 0.01)
	amount = 0.01;
    else if (amount > 0.15)
	amount = 0.15;

    sw.scaleVelocity = (amount * sw.scaleVelocity + adjust) / (amount + 1.0);

    if (Math.abs (dx) < 0.1 && Math.abs (sw.xVelocity) < 0.2 &&
	Math.abs (dy) < 0.1 && Math.abs (sw.yVelocity) < 0.2 &&
	Math.abs (ds) < 0.001 && Math.abs (sw.scaleVelocity) < 0.002) {

	sw.xVelocity = sw.yVelocity = sw.scaleVelocity = 0.0;
	sw.tx = x1 - sw.orig_left;
	sw.ty = y1 - sw.orig_top;
	sw.scale = scale;

	sw.style.left = Math.floor (sw.orig_left + sw.tx / sw.scale) + "px";
	sw.style.top = Math.floor (sw.orig_top + sw.ty / sw.scale) + "px";
	sw.style.width = Math.floor (sw.orig_width * sw.scale) + "px";
	sw.style.height = Math.floor (sw.orig_height * sw.scale) + "px";

	return 0;
    }

    return 1;
}

function scaleDoStep ()
{
    var new_time = new Date().getTime();

    var ms = new_time - ss.lastTime;

    ss.lastTime = new_time;

    scaleStep (ms);
}

function scaleStep (msSinceLastStep) {

    if (ss.state != "none" && ss.state != "wait") {
	var amount = msSinceLastStep * 0.05 * ss.speed;
	var steps  = Math.floor (amount / (0.5 * ss.timestep));
	if (steps == 0) steps = 1;
	var chunk  = amount / steps;

	while (steps--) {
	    ss.moreAdjust = 0;

	    for (var swi = 0; swi < ss.windows.length; swi++) {
		var sw = ss.windows[swi];

		if (sw.adjust) {
		    sw.adjust = adjustScaleVelocity (sw);

		    ss.moreAdjust |= sw.adjust;
		    
		    sw.tx += sw.xVelocity * chunk;
		    sw.ty += sw.yVelocity * chunk;
		    sw.scale += sw.scaleVelocity * chunk;

		    if (sw.adjust) {
			sw.style.left = Math.floor (sw.orig_left + sw.tx / sw.scale) + "px";
			sw.style.top = Math.floor (sw.orig_top + sw.ty / sw.scale) + "px";
 			sw.style.width = Math.floor (sw.orig_width * sw.scale) + "px";
 			sw.style.height = Math.floor (sw.orig_height * sw.scale) + "px";
		    }
		}
	    }

	    if (!ss.moreAdjust)
		break;
	}
    }


    if (ss.state != "none") {
	if (ss.moreAdjust) {
	    setTimeout (scaleDoStep, stepTimeout);
	}
	else {
	    if (ss.state == "in") {
		// we're done zooming out.  hide the expose layer
		exposeLayer.style.display = "none";
		for (var swi = 0; swi < ss.windows.length; swi++) {
		    var sw = ss.reverseWindows[swi];
		    sw.destroy ();
		    exposeLayer.removeChild (sw);
		}
		ss.reverseWindows = new Array ();
		ss.state = "none";
	    }
	    else if (ss.state == "out")
		ss.state = "wait";
	}
    }
}

function scaleTerminate () {
    if (ss.state != "none") {

	for (var swi = 0; swi < ss.windows.length; swi++) {
	    var sw = ss.windows[swi];
	    if (sw.slot != null) {
		sw.slot = null;
		sw.adjust = true;
	    }
	}

	/*
	  not yet - toshok

	if (ss.state != "in") {
	    w = findWindowAtScreen (s, sd->lastActiveWindow);
	    if (w) {
		int x, y;

		activateWindow (w);

		defaultViewportForWindow (w, &x, &y);

		if (x != s->x || y != s->y)
		    sendViewportMoveRequest (s,
					     x * s->width,
					     y * s->height);
	    }
	}
	*/

	ss.state = "in";

	ss.lastTime = new Date ().getTime();
	setTimeout (scaleDoStep, stepTimeout);
    }

    //sd->lastActiveNum = 0;

    return false;
}

function scaleInitiateCommon ()
{
    ss.state = "out";

    if (!layoutThumbs ()) {
	ss.state = "wait";
	return false;
    }

    /* not yet 

    if (!sd->lastActiveNum)
	sd->lastActiveNum = s->activeNum - 1;

    sd->lastActiveWindow = s->display->activeWindow;
    sd->selectedWindow   = s->display->activeWindow;

    */

    ss.lastTime = new Date ().getTime();

    /* start the timeout */
    setTimeout (scaleDoStep, stepTimeout);

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
