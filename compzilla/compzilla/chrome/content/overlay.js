/*
window.hidechrome = true;
window.screenX = 0;
window.screenY = 0;
window.sizemode = maximized;
*/

window.addEventListener("onload", compzillaLoad, true);

function compzillaLoad()
{
	cls = Components.classes['@beatniksoftware.com/compzillaService'];
	svc = cls.getService(Components.interfaces.compzillaIControl);

	wmcls = Components.classes['@beatniksoftware.com/compzillaWindowManager'];
	wm = wmcls.createInstance(Components.interfaces.compzillaIWindowManager);

	// this next call will generated a call to compzillaWindowCreated for each window
	svc.RegisterWindowManager(wm)
}

compzillaLoad ();