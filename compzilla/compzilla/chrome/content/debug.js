/* -*- mode: javascript; c-basic-offset: 4; indent-tabs-mode: t; -*- */


Debug = function (log, str)
{
    /* we accept the 1 arg form, and use "all" for the log in that
       case. */
    if (str == undefined) {
	str = log;
	log = "all";
    }

    if (log == "all" || log in Debug) {
	if (debugLog) {
	    debugLog.value = str + "\n" + debugLog.value;
	}
    }
}
document.Debug = Debug;


function debugToggle (el) {
    if (el.selected == undefined)
	el.selected = false;

    if (el.selected) {
	debugDeselectLog (el.name);
    }
    else {
	debugSelectLog (el.name);
    }

    el.selected = !el.selected;
}


function debugSelectLog (log) {
    Debug ("selecting debug log '" + log + "'");
    Debug[log] = true;
}


function debugDeselectLog (log) {
    Debug ("deselecting debug log '" + log + "'");
    delete Debug[log];
}


// Build the debug window
var debugContent = document.getElementById ("debugContent");
var debugLog = document.getElementById ("debugLog");

// Make a frame for the debug window
var debugFrame = CompzillaFrame (debugContent);

debugFrame.id = "debugFrame";
debugFrame.title = "Debug Window";
debugFrame.moveResize (200, 50, 300, 300);
debugFrame.show ();
debugFrame.allowClose =
  debugFrame.allowMinimize =
  debugFrame.allowMaximize = false;

windowStack.stackWindow (debugFrame);


// should we hide the window, or the entire layer?
function debugToggleWindow ()
{
    debugFrame.style.display = (debugFrame.style.display == "none") ? 
	"block" : "none";
}


function debugClearLog ()
{
    debugLog.value = "";
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
		res += "Title: " + el.getTitle() + ", ";
	    }
	    res += "ID: " + el.id + ", ";
	    res += "Rect: (x:" + el.offsetLeft + " y:" + el.offsetTop + 
		" w:" + el.offsetWidth + " h:" + el.offsetHeight + ")";
	    res += "]";
	    Debug (res);
	}
    }
    return "";
}


// XXX until we get commands working in the .xul file
/*
document.addEventListener("keypress", {
                              handleEvent: function (event) {
				  if (event.keyCode == event.DOM_VK_F10 && event.ctrlKey) {
				      debugToggleWindow ();
				  }
			      } },
			      true);
*/
