/* -*- mode: javascript; c-basic-offset: 4; indent-tabs-mode: t; -*- */

function _addUtilMethods (o)
{
    var UtilMethods = {
	addProperty: function (name, getter, setter) {
	    this.__defineGetter__ (name, getter);

	    /* allow setter to be undefined for read-only properties */
	    if (setter != undefined)
		this.__defineSetter__ (name, setter);
	},

	getPosition: function () {
	    var curleft = curtop = 0;
	    var obj = this;
	    while (obj) {
		curleft += obj.offsetLeft;
		curtop += obj.offsetTop;
		obj = obj.offsetParent;
	    }
	    return { left: curleft, top: curtop };
	}
    };

    jQuery.extend (o, UtilMethods);
}
