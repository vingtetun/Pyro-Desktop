/* -*- mode: javascript; c-basic-offset: 4; indent-tabs-mode: t; -*- */

var desktop = document.getElementById ("desktop");

// Make a frame for the debug window
var debugContent = document.getElementById ("debugContent");

var debugFrame = CompzillaFrame (debugContent);
debugFrame.moveResize (300, 400, 200, 50);
debugFrame.setTitle ("Debug Window");

var windowStack = document.getElementById ("windowStack");
windowStack.stackWindow (debugFrame);


var debugLog = null;
for (var el = debugContent.firstChild; el != null; el = el.nextSibling) {
    if (el.id == "debugLog") {
	debugLog = el;
	break;
    }
}
if (debugLog == null)
    throw "debug content of invalid form.  you need to specify an element with id = `debugLog'";
    
desktop.Debug = function (str)
{
    if (debugLog) {
	debugLog.insertBefore (document.createElementNS ("http://www.w3.org/1999/xhtml", "html:br"),
			       debugLog.firstChild);
	debugLog.insertBefore (document.createTextNode (str),
			       debugLog.firstChild);
    }
}

function toggleDebugWindow ()
{
    debugFrame.style.visibility = (debugFrame.style.visibility == "hidden") ? "visible" : "hidden";
}
