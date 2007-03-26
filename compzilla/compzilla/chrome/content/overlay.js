
window.hidechrome = true
window.screenX = 0
window.screenY = 0
window.sizemode = maximized

window.addEventListener("onload", compzillaLoad, true)

function compzillaLoad()
{
	cls = Components.classes['@beatniksoftware.com/compzillaService;1']
	svc = cls.getService(Components.interfaces.compzillaIControl);
	svc.RegisterWindowManager(window)
}
