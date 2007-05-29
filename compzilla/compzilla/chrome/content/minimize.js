/* -*- mode: javascript; c-basic-offset: 4; indent-tabs-mode: t; -*- */


function MinimizeCompzillaFrame (w) {
    /* first we create a canvas of the right size/location */	
    var min = CompzillaWindowContent (w.content.nativeWindow);

    min.style.position = "absolute";

    min.style.left = (w.offsetLeft + w.content.offsetLeft) + "px";
    min.style.top = (w.offsetTop + w.content.offsetTop) + "px";
    min.style.width = w._contentBox.offsetWidth + "px";
    min.style.height = w._contentBox.offsetHeight + "px";
    min.width = w._contentBox.offsetWidth;
    min.height = w._contentBox.offsetHeight;

    Debug ("starting geometry = [" +
	   min.offsetLeft + ", " +
	   min.offsetTop + ", " +
	   min.offsetWidth + ", " +
	   min.offsetHeight + "]");

    /* insert the min window into the stack at the right level */
    w.parentNode.appendChild (min);
    min.style.zIndex = w.style.zIndex;

    /* hide the real window, show the min window */
    w.hide ();
    min.style.display = "block";

    var geom = w.content.XProps[Atoms._NET_WM_ICON_GEOMETRY];
    if (!geom) {
	/* for windows without the hint, iconify to the bottom left */
	geom = {
	    x: 0,
	    y: screen.height,
	    width: 0,
	    height: 0
	};
    }

    Debug ("minimize geometry = [" +
	   geom.x + ", " +
	   geom.y + ", " +
	   geom.width + ", " +
	   geom.height + "]");

    $(min).animate ({ left: geom.x, 
		      top: geom.y, 
		      width: geom.width, 
		      height: geom.height,
		      opacity: 0 },
		    1000, 
		    "easeout",
		    function () {
                        min.parentNode.removeChild (min);
                    });
}


function hackMinimizeFrame ()
{
    MinimizeCompzillaFrame (minFrame);
}


/*
document.addEventListener("keypress", {
                              handleEvent: function (event) {
				  if (event.keyCode == event.DOM_VK_F8 && event.ctrlKey) {
				    hackMinimizeFrame ();
				  }
			      } },
			      true);
*/
