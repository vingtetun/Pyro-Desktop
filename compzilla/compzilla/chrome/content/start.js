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
		var frame = CompzillaMakeFrame (content, null, ev.overrideRedirect);

		// ev is a compzillaIWindowConfigureEvent
		frame.moveResize (ev.x, ev.y, ev.width, ev.height);
		if (ev.mapped) {
		    frame.show ();
		}

		/* initialize newly created frames to have all actions
		   available.  we'll selectively turn them off later
		   based on properties */

		stack = document.getElementById ("windowStack");
    		stack.stackWindow (frame);
	    }
        },
	true);

   // Register as the window manager and generate windowcreate events for
   // existing windows.
   svc.RegisterWindowManager (window);
}
