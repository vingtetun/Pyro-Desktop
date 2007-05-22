/* -*- mode: javascript; c-basic-offset: 4; indent-tabs-mode: t; -*- */

const compzillaIControl = Components.interfaces.compzillaIControl;


function compzillaLoad()
{
    var xulwin = document.getElementById ("desktopWindow");
    xulwin.width = screen.width;
    xulwin.height = screen.height;

    svccls = Components.classes['@pyrodesktop.org/compzillaService'];
    svc = svccls.getService (compzillaIControl);

    svc.addEventListener (
	"windowcreate", 
	{
	    handleEvent: function (ev) {
		// NOTE: Be careful to not do anything which will create a new
		//       toplevel window, to avoid an infinite loop.

		var content = CompzillaWindowContent (ev.window);
		var frame = CompzillaFrame (content);
		frame.overrideRedirect = ev.overrideRedirect;

		windowStack.stackWindow (frame);

		// ev is a compzillaIWindowConfigureEvent
		frame.moveResize (ev.x, ev.y, ev.width, ev.height);
		if (ev.mapped) {
		    frame.show ();
		}
	    }
        },
	true);

   // Register as the window manager and generate windowcreate events for
   // existing windows.
   svc.RegisterWindowManager (window);
}
