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

  /* now animate the minimization */

  min.dest_x = 0;
  min.dest_y = windowStack.offsetTop + windowStack.offsetHeight;

  min.update =  function () {
    var another = false;

    if (min.offsetLeft > min.dest_x + 5) {
      min.style.left = (min.offsetLeft - 5) + "px";
      another = true;
    }
    if (min.offsetTop < min.dest_y + 10) {
      min.style.top = (min.offsetTop + 10) + "px";
      another = true;
    }

    if (min.offsetWidth > 5)
      min.style.width = (min.offsetWidth - 5) + "px";

    if (min.offsetHeight > 5)
      min.style.height = (min.offsetHeight - 5) + "px";

    if (another)
      setTimeout (min.update, 50);
  };

  setTimeout (min.update, 50);
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
