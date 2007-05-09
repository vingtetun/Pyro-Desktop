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

		content = CompzillaWindowContent (ev.window);
		frame = CompzillaFrame (content);

		// ev is a compzillaIWindowConfigureEvent
		frame.moveResize (ev.width, ev.height, ev.x, ev.y);
		if (ev.mapped) {
		    frame.show ();
		}

		stack = document.getElementById ("windowStack");
    		stack.stackWindow (frame);
	    }
        },
	true);

   alert("Calling RegisterWindowManager!");

   // Register as the window manager and generate windowcreate events for
   // existing windows.
   svc.RegisterWindowManager (window);
}
