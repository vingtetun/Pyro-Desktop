
const compzillaIControl = Components.interfaces.compzillaIControl;
const compzillaIWindowManager = Components.interfaces.compzillaIWindowManager;

function compzillaLoad()
{
   svccls = Components.classes['@beatniksoftware.com/compzillaService'];
   svc = svccls.getService(compzillaIControl);

   wmcls = Components.classes['@beatniksoftware.com/compzillaWindowManager'];
   wm = wmcls.createInstance(compzillaIWindowManager);

   var desktop = document.getElementById ("desktop");
   wm.SetDocument (desktop.contentDocument);

   var win = document.getElementById ("desktopWindow");

   win.width = screen.width;
   win.height = screen.height;

   // this will generates a call to compzillaWindowCreated for each window
   svc.RegisterWindowManager (window, wm);
}
