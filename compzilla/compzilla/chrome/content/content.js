/* -*- mode: javascript; c-basic-offset: 4; indent-tabs-mode: t; -*- */


function CompzillaWindowContent (nativewin) {
    Debug ("Creating content for nativewin=" + nativewin);

    var content = document.getElementById ("windowContent").cloneNode (true);

    // Allow the content to be focus()'d
    content.tabIndex = "1";

    content._nativewin = nativewin;
    content.getNativeWindow = function () { return content._nativewin; }

    content.destroy = function () { 
	Debug ("content.destroy");
	content._nativewin.RemoveContentNode (content); 
    }

    nativewin.AddContentNode (content);
    return content;
}


