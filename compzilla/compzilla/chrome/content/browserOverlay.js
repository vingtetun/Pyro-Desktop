/* -*- mode: javascript; c-basic-offset: 4; indent-tabs-mode: t; -*- */


function startCompzilla ()
{
    var svc = Components.classes['@pyrodesktop.org/compzillaService;1']
	                .getService(Components.interfaces.compzillaIControl);

    if (!svc.HasWindowManager (window) || promptReplaceWindowManager ()) {
	window.openDialog ("chrome://compzilla/content/start.xul", 
			   "_blank", 
			   "chrome,dialog=no");
    }
}


function autoStartCompzilla ()
{
    if (!getBoolPref ("compzilla.autostart", true))
	return;

    var wm = Components.classes['@mozilla.org/appshell/window-mediator;1']
	               .getService(Components.interfaces.nsIWindowMediator);
    var enum = wm.getEnumerator("navigator:browser");
    var firstWindow = enum.getNext () && !enum.hasMoreElements ();

    if (!firstWindow)
	return;

    startCompzilla ();
}


// Run when opening a new browser window
autoStartCompzilla ();
