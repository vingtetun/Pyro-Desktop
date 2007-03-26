/*
window.hidechrome = true;
window.screenX = 0;
window.screenY = 0;
window.sizemode = maximized;
*/

//alert ("hi");
window.addEventListener("onload", compzillaLoad, true);

function compzillaLoad()
{
   //alert (3);
	cls = Components.classes['@beatniksoftware.com/compzillaService']

   //alert (cls);

	svc = cls.getService(Components.interfaces.compzillaIControl);

   alert (svc);

   alert (svc.addEventListener);
	svc.addEventListener ("onwindowcreated", compzillaWindowCreated, true);

   alert ("yo");

	// this next call will generated a call to compzillaWindowCreated for each window
	svc.RegisterWindowManager(window.document)

   //alert ("yo");
}

function compzillaWindowCreated ()
{
   alert ("yO!");
}

compzillaLoad ();