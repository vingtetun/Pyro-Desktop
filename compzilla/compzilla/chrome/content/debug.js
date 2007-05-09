/* -*- mode: javascript; c-basic-offset: 4; indent-tabs-mode: t; -*- */


Debug = function (str)
{
    if (debugLog) {
	debugLog.insertBefore (document.createElementNS ("http://www.w3.org/1999/xhtml", "br"),
			       debugLog.firstChild);
	debugLog.insertBefore (document.createTextNode (str),
			       debugLog.firstChild);
    }
}
document.Debug = Debug;


// Build the debug window
var debugContent = document.getElementById ("debugContent");
var debugLog = document.getElementById ("debugLog");

// Make a frame for the debug window
var debugFrame = CompzillaFrame (debugContent);
debugFrame.moveResize (300, 300, 200, 50);
debugFrame.setTitle ("Debug Window");
debugFrame.show ();

var windowStack = document.getElementById ("windowStack");
windowStack.stackWindow (debugFrame);


function toggleDebugWindow ()
{
    debugFrame.style.visibility = (debugFrame.style.visibility == "hidden") ? 
	"visible" : "hidden";
}


function clearDebugLog ()
{
    debugLog.innerHTML = "";
}
