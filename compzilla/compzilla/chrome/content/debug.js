/* -*- mode: javascript; c-basic-offset: 4; indent-tabs-mode: t; -*- */


// Make a frame for the debug window
var debugContent = document.getElementById ("debugContent");

var debugFrame = CompzillaFrame (debugContent);
debugFrame.moveResize (300, 400, 200, 50);
debugFrame.setTitle ("Debug Window");

var windowStack = document.getElementById ("windowStack");
windowStack.stackWindow (debugFrame);


var desktop = document.getElementById ("desktop");

desktop.Debug = function (str)
{
    if (debugContent) {
        debugContent.innerHTML = str + "<br>" + debugContent.innerHTML;
    }
}

