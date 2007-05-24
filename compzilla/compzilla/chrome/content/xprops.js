/* -*- mode: javascript; c-basic-offset: 4; indent-tabs-mode: t; -*- */

// an (optionally caching) class that's useful for abstracting away
// lots of the drudgery involved with converting betwen X properties
// (represented in the js by property bags) and whatever the actual
// value we'd like to associate with the property is.


function XProps (content)
{
    // set this to true to enable caching of X property values
    this._use_cache = true;

    this._content = content;
    this._fillers = new Object ();

    if (this._use_cache)
	this._values = new Object ();
}
XProps.prototype = {
    Add : function (atom, filler) {
	this._fillers[atom] = filler;
	this.Invalidate (atom);

	/* the atom+"" is needed because getter names must be strings, not ints */
	this.__defineGetter__ (atom+"",
			       function () {
				   if (this._use_cache && atom in this._values) {
				       return this._values[atom];
				   }
				   else {
				       var prop_bag = this._content.nativeWindow.GetProperty (atom);
				       var val = (prop_bag == null) ? null : this._fillers[atom](prop_bag);

				       if (this._use_cache)
					   this._values[atom] = val;
				       return val;
				   }
			       });
    },

    Invalidate: function (atom) {
	if (this._use_cache)
	    delete this._values[atom];
    }
}
