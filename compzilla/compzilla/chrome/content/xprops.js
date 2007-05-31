/* -*- mode: javascript; c-basic-offset: 4; indent-tabs-mode: t; -*- */

// an (optionally caching) class that's useful for abstracting away
// lots of the drudgery involved with converting betwen X properties
// (represented in the js by property bags) and whatever the actual
// value we'd like to associate with the property is.


var use_cache = true;


function XProps (nativewin)
{
    // set this to true to enable caching of X property values
    if (use_cache)
	this._values = new Object ();

    this._nativewin = nativewin;
    this._fillers = new Object ();

    // Add the default property fillers
    for (var atom in PropertyFillers) {
	this.Add (Atoms[atom], PropertyFillers[atom]);
    }
}
XProps.prototype = {
    Add : function (atom, filler) {
	this._fillers[atom] = filler;
	this.Invalidate (atom);

	/* the atom+"" is needed because getter names must be strings, not ints */
	this.__defineGetter__ (atom+"",
			       function () {
				   if (use_cache && atom in this._values) {
				       return this._values[atom];
				   }
				   else {
				       var val = null;

				       var prop_bag = this._nativewin.GetProperty (atom);
				       if (prop_bag && this._fillers[atom]) {
					   val = this._fillers[atom](prop_bag);
				       }

				       if (use_cache)
					   this._values[atom] = val;

				       return val;
				   }
			       });
    },

    Invalidate: function (atom) {
	if (use_cache)
	    delete this._values[atom];
    }
}


/*
 * PropertyFillers return a friendly object with properties pulled from the 
 * X atom's property bag.
 */
PropertyFillers = {
    _NET_WM_NAME: function (prop_bag) {
	return prop_bag.getProperty (".text");
    },

    XA_WM_NAME: function (prop_bag) {
	return prop_bag.getProperty (".text");
    },

    XA_WM_CLASS: function (prop_bag) {
	return { instanceName: prop_bag.getProperty (".instanceName"),
		 className: prop_bag.getProperty (".className")
	       };
    },

    _NET_WM_WINDOW_TYPE: function (prop_bag) {
	// XXX _NET_WM_WINDOW_TYPE is actually an array of atoms
	return prop_bag.getPropertyAsUint32 (".atom");
    },

    _NET_WM_STRUT: function (prop_bag) {
	return { partial: false,
		 left: prop_bag.getPropertyAsUint32 (".left"),
		 right: prop_bag.getPropertyAsUint32 (".right"),
		 top: prop_bag.getPropertyAsUint32 (".top"),
		 bottom: prop_bag.getPropertyAsUint32 (".bottom"),
	       };
    },

    _NET_WM_STRUT_PARTIAL: function (prop_bag) {
	return { partial: true,
	         left: prop_bag.getPropertyAsUint32 (".left"),
		 right: prop_bag.getPropertyAsUint32 (".right"),
		 top: prop_bag.getPropertyAsUint32 (".top"),
		 bottom: prop_bag.getPropertyAsUint32 (".bottom"),
		 leftStartY: prop_bag.getPropertyAsUint32 (".leftStartY"),
		 leftEndY: prop_bag.getPropertyAsUint32 (".leftEndY"),
		 rightStartY: prop_bag.getPropertyAsUint32 (".rightStartY"),
		 rightEndY: prop_bag.getPropertyAsUint32 (".rightEndY"),
		 topStartX: prop_bag.getPropertyAsUint32 (".topStartX"),
		 topEndX: prop_bag.getPropertyAsUint32 (".topEndX"),
		 bottomStartX: prop_bag.getPropertyAsUint32 (".bottomStartX"),
		 bottomEndX: prop_bag.getPropertyAsUint32 (".bottomEndX"),
	       };
    },

    _NET_WM_ICON_GEOMETRY: function (prop_bag) {
	return { x: prop_bag.getPropertyAsUint32 (".x"),
		 y: prop_bag.getPropertyAsUint32 (".y"),
		 width: prop_bag.getPropertyAsUint32 (".width"),
		 height: prop_bag.getPropertyAsUint32 (".height")
	       };
    },
};
