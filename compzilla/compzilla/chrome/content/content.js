/* -*- mode: javascript; c-basic-offset: 4; indent-tabs-mode: t; -*- */


function CompzillaWindowContent (nativewin) {
    content = document.getElementById ("windowContent").cloneNode (true);

    content.getNativeWin = function () { return nativewin; }

    nativewin.AddContentNode (content);
    return content;
}


