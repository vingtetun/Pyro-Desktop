alert ("hi");

/*
window.hidechrome = true
window.screenX = 0
window.screenY = 0
window.sizemode = maximized

window.addEventListener("onload", compzillaLoad, true)
*/

function compzillaLoad()
{
   alert ("hi again");
	cls = Components.classes['@beatniksoftware.com/compzillaService']

   alert (cls);

	svc = cls.getService(Components.interfaces.compzillaIControl);

   alert (svc);

	//svc.addEventListener ("onwindowcreated", compzillaWindowCreated, true);

   alert ("yo");

	// this next call will generated a call to compzillaWindowCreated for each window
	svc.RegisterWindowManager()

   alert ("yo");
}

compzillaLoad ();