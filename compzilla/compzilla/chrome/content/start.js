
function compzillaLoad()
{
	cls = Components.classes['@beatniksoftware.com/compzillaService'];
	svc = cls.getService(Components.interfaces.compzillaIControl);

	wmcls = Components.classes['@beatniksoftware.com/compzillaWindowManager'];
	wminst = wmcls.createInstance(Components.interfaces.compzillaIWindowManager);

   var wm = wminst.QueryInterface (Components.interfaces.compzillaIWindowManager);

   var desktop = document.getElementById ("desktop");

   wm.SetDocument (desktop.contentDocument);

	// this next call will generated a call to compzillaWindowCreated for each window
	svc.RegisterWindowManager(wm);
}
