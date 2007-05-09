/* -*- mode: javascript; c-basic-offset: 4; indent-tabs-mode: t; -*- */


function CompzillaWindowContent (nativewin) {
    content = document.getElementById ("windowContent").cloneNode (true);

    Debug ("Creating content for nativewin=" + nativewin);

    content._nativewin = nativewin;
    content.getNativeWindow = function () { return content._nativewin; }

    content.destroy = function () { 
	Debug ("content.destroy");
	content._nativewin.RemoveContentNode (content); 
    }

    nativewin.AddContentNode (content);
    return content;
}


