/* -*- mode: javascript; c-basic-offset: 4; indent-tabs-mode: t; -*- */

function WorkArea ()
{
    this.frameInfo = {};

    this.InitBounds ();
}
WorkArea.prototype = {

    UpdateStrutInfo: function (frame, strut) {
	if (strut == null) {
	    // we should be removing the strut info
	    if (frame in this.frameInfo)
		delete this.frameInfo[frame];
	}
	else {
	    this.frameInfo[frame] = strut;
	}

	this.UpdateBounds ();
    },

    InitBounds: function () {
	this.bounds = { left: 0,
			top: 0,
			width: screen.width,
			height: screen.height };
    },

    UpdateBounds: function () {
	this.InitBounds ();
						     
	for (var f in this.frameInfo) {
	    var strut = this.frameInfo[f];

	    this.bounds.left += strut.left;
	    this.bounds.top += strut.top;
	    this.bounds.width -= strut.left + strut.right;
	    this.bounds.height -= strut.top + strut.bottom;

	    /* let's ignore the partial strut stuff for now */
	}

	Debug ("workarea bounds set to " +
	       "{ " +
	       this.bounds.left + ", " +
	       this.bounds.top  + " - " +
	       this.bounds.width + "x" +
	       this.bounds.height +
	       " }");


	// update the root window property
	var vals = [ this.bounds.left, this.bounds.top, this.bounds.width, this.bounds.height ];
	svc.SetRootWindowProperty (Atoms._NET_WORKAREA, Atoms.XA_CARDINAL, vals.length, vals);

	// XXX somehow we should tell maximized windows to resize
	// themselves here.
    }

};
