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


function debugClearLog ()
{
    debugLog.innerHTML = "";
}


function debugEval ()
{
    text = document.getElementById ("debugEntry").value;
    if (text) {
	try {
	    result = eval(text);
	} catch (e) {
	    result = e;
	}
	Debug ("Eval: " + result);
    }
}


function debugListWindows ()
{
    for (var i = 0; i < layers.length; i ++) {
	for (var el = layers[i].firstChild; el != null; el = el.nextSibling) {
	    if (el.className != "windowFrame") {
		continue;
	    }

	    res = "Layer" + i + ": [";
	    if (el.getTitle) {
		res += "Title:" + el.getTitle() + ", ";
	    }
	    if (el.getContent) {
		content = el.getContent();
		if (content.getNativeWindow) {
		    res += "XID:" + content.getNativeWindow().nativeWindowId + ", ";
		}
	    }
	    res += "Rect: (x:" + el.offsetLeft + " y:" + el.offsetTop + 
		" w:" + el.offsetWidth + " h:" + el.offsetHeight + ")";
	    res += "]";
	    Debug (res);
	}
    }
    return "";
}
