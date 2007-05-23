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

    /* for windows without the hint, iconify to the bottom left */
    var dest_x = 0;
    var dest_y = windowStack.offsetTop + windowStack.offsetHeight;
    var dest_width = 0;
    var dest_height = 0;

    var propval = w.content.nativeWindow.GetProperty (Atoms._NET_WM_ICON_GEOMETRY);
    if (propval) {
	dest_x = propval.getPropertyAsUint32 (".x");
	dest_y = propval.getPropertyAsUint32 (".y");
	dest_width = propval.getPropertyAsUint32 (".width");
	dest_height = propval.getPropertyAsUint32 (".height");

	Debug ("minimize geometry = [" +
	       dest_x + ", " +
	       dest_y + ", " +
	       dest_width + ", " +
	       dest_height + "]");
    }

    $(min).animate ({ left: dest_x, 
		      top: dest_y, 
		      width: dest_width, 
		      height: dest_height, 
		      opacity: 0 },
		    250, 
		    "linear",
		    function () {
                        min.parentNode.removeChild (min);
                    });
}

function hackMinimizeFrame ()
{
    MinimizeCompzillaFrame (minFrame);
}

document.addEventListener("keypress", {
                              handleEvent: function (event) {
				  if (event.keyCode == event.DOM_VK_F8 && event.ctrlKey) {
				    hackMinimizeFrame ();
				  }
			      } },
			      true);
