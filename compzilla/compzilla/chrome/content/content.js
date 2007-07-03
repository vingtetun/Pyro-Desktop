/* -*- mode: javascript; c-basic-offset: 4; indent-tabs-mode: t; -*- */

var svc = Components.classes['@pyrodesktop.org/compzillaService'].getService(
    Components.interfaces.compzillaIControl);


var xNone = 0;


function CompzillaWindowContent (nativewin) {
    Debug ("content", "Creating content for nativewin=" + nativewin.nativeWindowId);

    var content = $("#windowContent").clone()[0];
    content.style.display = "block";

    content._nativewin = nativewin;

    nativewin.AddContentNode (content);

    _addUtilMethods (content);
    _addContentMethods (content);

    content._xprops = new XProps (nativewin);

    return content;
}


var ContentMethods = {

    ondestroy: function () {
	Debug ("content", "content.destroy");
	this._nativewin.RemoveContentNode (this);
	this._nativewin = null;

	this._xprops.destroy ();
    },

    onmoveresize: function (overrideRedirect) {

	// at the very least we need to make sure we update our
	// width/height to the right size, or the resize will scale
	// us.
	this.width = this.offsetWidth;
	this.height = this.offsetHeight;

	// that's it for override redirect windows
	if (overrideRedirect)
	    return;

        var pos = this.getPosition();

        Debug("frame", "Calling ConfigureWindow: xid=" +
              this._nativewin.nativeWindowId + ", x=" +
              pos.left + ", y=" +
              pos.top + ", width=" +
              this.width + ", height=" +
              this.height);

        svc.ConfigureWindow (this._nativewin.nativeWindowId,
                             pos.left,
                             pos.top,
                             this.width,
                             this.height,
                             0);
    },

    onhide: function () {
	this.style.visibility = "hidden";

	// XXX remove the _NET_WM_WINDOW_STATE property from the window
	svc.UnmapWindow (this._nativewin.nativeWindowId);
    },

    onshow: function () {
	this.style.visibility = "visible";

	// XXX calculate and add the _NET_WM_WINDOW_STATE property to the window
	svc.MapWindow (this._nativewin.nativeWindowId);
    },

    onmaximize: function () {
	// XXX update the _NET_WM_WINDOW_STATE property
    },

    onfullscreen: function () {
	// XXX update the _NET_WM_WINDOW_STATE property
    },

    onlostfocus: function () {
	// update _NET_ACTIVE_WINDOW on the root window
	var focused_list = [ xNone ];
	svc.SetRootWindowProperty (Atoms._NET_ACTIVE_WINDOW, 
				   Atoms.XA_WINDOW, 
				   focused_list.length, 
				   focused_list);
    },

    ongotfocus: function () {
	// update _NET_ACTIVE_WINDOW on the root window
	var focused_list = [ this._nativewin.nativeWindowId ];
	svc.SetRootWindowProperty (Atoms._NET_ACTIVE_WINDOW, 
				   Atoms.XA_WINDOW, 
				   focused_list.length, 
				   focused_list);
    },

    onmovetotop: function () {
	svc.MoveToTop (this._nativewin.nativeWindowId);
    },

    onmovetobottom: function () {
	svc.MoveToBottom (this._nativewin.nativeWindowId);
    },

    getNativeProperty: function (atom) {
	return this._xprops.lookup (atom);
    },
}


function _addContentMethods (content) {
    jQuery.extend (content, ContentMethods);

    content.addProperty ("nativeWindow",
			 /* getter */
			 function () {
			     return this._nativewin;
			 });


    content.addProperty ("wmClass",
			 /* getter */
			 function () {
			     var wmClass = this.getNativeProperty (Atoms.XA_WM_CLASS);
			     if (wmClass == null)
				 return "";

			     return wmClass.instanceName + " " + wmClass.className;
			 });

    content.addProperty ("wmName",
			 /* getter */
			 function () {
			     var name = this.getNativeProperty (Atoms._NET_WM_NAME);

			     // fall back to XA_WM_NAME if _NET_WM_NAME is undefined.
			     if (name == null)
				 name = this.getNativeProperty (Atoms.XA_WM_NAME);

			     return name;
			 });

    content.addProperty ("wmIconName",
			 /* getter */
			 function () {
			     var name = this.getNativeProperty (Atoms._NET_WM_ICON_NAME);

			     // fall back to XA_WM_ICON_NAME if _NET isn't set.
			     if (name == null)
				 name = this.getNativeProperty (Atoms.XA_WM_ICON_NAME);

			     return name;
			 });

    content.addProperty ("wmIcon",
			 /* getter */
			 function () {
			     var data = this.getNativeProperty (Atoms._NET_WM_ICON);

			     // FIXME: fall back to WMhints if _NET isn't set.

			     return data;
			 });

    content.addProperty ("wmStruts",
			 /* getter */
			 function () {
			     var struts = this.getNativeProperty (Atoms._NET_WM_STRUT_PARTIAL);
			     // fall back to _NET_WM_STRUT if _PARTIAL isn't there.
			     if (struts == null)
				 struts = this.getNativeProperty (Atoms._NET_WM_STRUT);

			     return struts;
			 });

    content.addProperty ("wmWindowType",
			 /* getter */
			 function () {
			     // there's a little more smarts require
			     // than this, check out the EWMH.
			     // windows with TRANSIENT_FOR without a
			     // type must be DIALOG, for instance,
			     // etc.

			     var t = this.getNativeProperty (Atoms._NET_WM_WINDOW_TYPE);
			     switch (t) {
			     case Atoms._NET_WM_WINDOW_TYPE_DESKTOP: return "desktop";
			     case Atoms._NET_WM_WINDOW_TYPE_DIALOG: return "dialog";
			     case Atoms._NET_WM_WINDOW_TYPE_DOCK: return "dock";
			     case Atoms._NET_WM_WINDOW_TYPE_MENU: return "menu";
			     case Atoms._NET_WM_WINDOW_TYPE_SPLASH: return "splash";
			     case Atoms._NET_WM_WINDOW_TYPE_TOOLBAR: return "toolbar";
			     case Atoms._NET_WM_WINDOW_TYPE_UTILITY: return "utility";
			     case Atoms._NET_WM_WINDOW_TYPE_DROPDOWN_MENU: return "dropdownmenu";
			     case Atoms._NET_WM_WINDOW_TYPE_POPUP_MENU: return "popupmenu";
			     case Atoms._NET_WM_WINDOW_TYPE_TOOLTIP: return "tooltip";
			     case Atoms._NET_WM_WINDOW_TYPE_NOTIFICATION: return "notification";
			     case Atoms._NET_WM_WINDOW_TYPE_COMBO: return "combo";
			     case Atoms._NET_WM_WINDOW_TYPE_DND: return "dnd";

			     case Atoms._NET_WM_WINDOW_TYPE_NORMAL:
			     default:
				 return "normal";
			     }
			 });
}
