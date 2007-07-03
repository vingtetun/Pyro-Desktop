/* -*- mode: javascript; c-basic-offset: 4; indent-tabs-mode: t; -*- */

// An (optionally caching) class that handles converting from the nsIPropertyBag
// returned from compzillaWindow::GetProperty.

var use_cache = true;


function XProps (nativewin)
{
    // set this to true to enable caching of X property values
    if (use_cache)
	this._values = new Object ();

    this._nativewin = nativewin;

    this._observer = {
	propertyChange: function (atom, isDeleted) {
	    Debug ("xprops: Invalidating atom " + atom);
	    this.invalidate (atom);
	}
    };
    nativewin.addObserver (this._observer);
}
XProps.prototype = {
    destroy: function (atom) {
	this._nativewin.removeObserver (this._observer);
    },

    lookup: function (atom) {
	if (use_cache && atom in this._values) {
	    return this._values[atom];
	}
	else {
	    var prop_bag = this._nativewin.GetProperty (atom);
	    if (!prop_bag)
		return null;

	    var val = new Object ();
	    var prop;
	    var propcnt = 0;

	    var enum = prop_bag.enumerator;
	    while (enum.hasMoreElements()) {
		prop = enum.getNext().QueryInterface(Components.interfaces.nsIProperty);

		val[prop.name] = prop.value;
		propcnt++;
	    }

	    // If there's only one property with a known name, use it directly.
	    if (propcnt == 1 && (prop.name == "atom" ||
				 prop.name == "text" ||
				 prop.name == "data")) {
		val = prop.value;
	    }

	    if (use_cache)
		this._values[atom] = val;

	    return val;
	}
    },

    invalidate: function (atom) {
	if (use_cache)
	    delete this._values[atom];
    }
}
