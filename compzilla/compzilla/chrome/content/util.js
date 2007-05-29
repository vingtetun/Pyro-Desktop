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
    };

    for (var m in UtilMethods) {
	o[m] = Utilmethods[m];
    }
}


function findPos (obj) 
{
    var curleft = curtop = 0;
    if (obj.offsetParent) {
        curleft = obj.offsetLeft;
        curtop = obj.offsetTop;
        while (obj = obj.offsetParent) {
            curleft += obj.offsetLeft;
            curtop += obj.offsetTop;
        }
    }
    return { left: curleft, top: curtop };
}
