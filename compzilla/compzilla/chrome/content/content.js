/* -*- mode: javascript; c-basic-offset: 4; indent-tabs-mode: t; -*- */


function CompzillaWindowContent (nativewin) {
    Debug ("content", "Creating content for nativewin=" + nativewin.nativeWindowId);

    var content = document.getElementById ("windowContent").cloneNode (true);

    content._nativewin = nativewin;

    content.__defineGetter__ ("nativeWindow",
			      function () { return this._nativewin; });

    content.destroy = function () { 
	Debug ("content", "content.destroy");
	content._nativewin.RemoveContentNode (content); 
	content._nativewin = null;
    }

    nativewin.AddContentNode (content);
    return content;
}


