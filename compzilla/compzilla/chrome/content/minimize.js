/* -*- mode: javascript; c-basic-offset: 4; indent-tabs-mode: t; -*- */

function _initSnapshot (w) {
  var snap = CompzillaWindowContent (w.content.nativeWindow);

  snap.orig_left = w.offsetLeft;
  snap.orig_top = w.offsetTop;
  snap.orig_width = w._contentBox.offsetWidth; /* XXX need a getter */
  snap.orig_height = w._contentBox.offsetHeight;

  snap.style.left = w.offsetLeft + "px";
  snap.style.top = w.offsetTop + "px";
  snap.style.width = w._contentBox.offsetWidth + "px";
  snap.style.height = w._contentBox.offsetHeight + "px";
  snap.width = w.content.width;
  snap.height = w.content.height;

  return snap;
}

function MinimizeCompzillaFrame (w) {
  /* first we create a canvas of the right size/location */
  var min = _initSnapshot (w);

  /* insert the min window into the stack at the right level */
  Debug ("w.parentNode = " + w.parentNode.id);
  w.parentNode.appendChild (min);
  min.style.zIndex = w.style.zIndex;

  /* hide the real window, show the min window */
  w.hide ();
  min.style.position = "absolute";
  min.style.display = "block";

  var dest_x = 0;
  var dest_y = windowStack.offsetTop + windowStack.offsetHeight;

  $(min).animate ( { left: dest_x, top: dest_y, width: 0, height: 0, opacity: 0 }, 250 );
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
