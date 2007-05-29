/* -*- mode: javascript; c-basic-offset: 4; indent-tabs-mode: t; -*- */

var svc = Components.classes['@pyrodesktop.org/compzillaService'].getService(
    Components.interfaces.compzillaIControl);


var xNone = 0;

function CompzillaWindowContent (nativewin) {
    Debug ("content", "Creating content for nativewin=" + nativewin.nativeWindowId);

    var content = document.getElementById ("windowContent").cloneNode (true);

    content._nativewin = nativewin;

    nativewin.AddContentNode (content);

    _addUtilMethods (content);
    _addContentMethods (content);
    _connectXProperties (content);

    return content;
}


var ContentMethods = {

    ondestroy: function () {
	Debug ("content", "content.destroy");
	this._nativewin.RemoveContentNode (this); 
	this._nativewin = null;
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
        Debug("FindPos: content left=" + pos.left + " top=" + pos.top);

        Debug("Calling ConfigureWindow: xid=" +
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
	// XXX remove the _NET_WM_WINDOW_STATE property from the window
	svc.UnmapWindow (this._nativewin.nativeWindowId);
    },

    onshow: function () {
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
	svc.SetRootWindowArrayProperty (Atoms._NET_ACTIVE_WINDOW, Atoms.XA_WINDOW, focused_list.length, focused_list);
    },

    ongotfocus: function () {
	// update _NET_ACTIVE_WINDOW on the root window
	var focused_list = [ this._nativewin.nativeWindowId ];
	svc.SetRootWindowArrayProperty (Atoms._NET_ACTIVE_WINDOW, Atoms.XA_WINDOW, focused_list.length, focused_list);
    },
}


function _addContentMethods (content) {
    for (var m in ContentMethods) {
	content[m] = ContentMethods[m];
    }

    content.addProperty ("nativeWindow",
			 /* getter */
			 function () {
			     return this._nativewin;
			 });


    content.addProperty ("wmClass",
			 /* getter */
			 function () {
			     var wmClass = this.XProps[Atoms.XA_WM_CLASS];

			     if (wmClass == null)
				 return "";
			     else
				 return wmClass.instanceName + " " + wmClass.className;
			 });

    content.addProperty ("wmName",
			 /* getter */
			 function () {
			     var name = this.XProps[Atoms._NET_WM_NAME];

			     // fall back to XA_WM_NAME if _NET_WM_NAME is undefined.
			     if (name == null)
				 name = this.XProps[Atoms.XA_WM_NAME];

			     return name;
			 });


    content.addProperty ("wmStruts",
			 /* getter */
			 function () {
			     var struts = this.XProps[Atoms._NET_WM_STRUT_PARTIAL];
			     // fall back to _NET_WM_STRUT if _PARTIAL isn't there.
			     if (struts == null)
				 struts = this.XProps[Atoms._NET_WM_STRUT];

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

			     var t = this.XProps[Atoms._NET_WM_WINDOW_TYPE];
			     switch (t) {

			     case Atoms._NET_WM_WINDOW_TYPE_DESKTOP: return "desktop";
			     case Atoms._NET_WM_WINDOW_TYPE_DIALOG: return "dialog";
			     case Atoms._NET_WM_WINDOW_TYPE_DOCK: return "dock";
			     case Atoms._NET_WM_WINDOW_TYPE_MENU: return "menu";
			     case Atoms._NET_WM_WINDOW_TYPE_SPLASH: return "splash";
			     case Atoms._NET_WM_WINDOW_TYPE_TOOLBAR: return "toolbar";
			     case Atoms._NET_WM_WINDOW_TYPE_UTILITY: return "utility";

			     case Atoms._NET_WM_WINDOW_TYPE_NORMAL:
			     default:
				 return "normal";
			     }
			 });
}


function _connectXProperties (content)
{
    content.XProps = new XProps (content);

    content.XProps.Add (Atoms._NET_WM_NAME,
			function (prop_bag) {
			    return prop_bag.getProperty (".text");
			});

    content.XProps.Add (Atoms.XA_WM_NAME,
			function (prop_bag) {
			    return prop_bag.getProperty (".text");
			});

    content.XProps.Add (Atoms.XA_WM_CLASS,
			function (prop_bag) {
			    return { instanceName: prop_bag.getProperty (".instanceName"),
                                     className: prop_bag.getProperty (".className")
                                   };
			});

    content.XProps.Add (Atoms._NET_WM_WINDOW_TYPE,
			function (prop_bag) {
			    // XXX _NET_WM_WINDOW_TYPE is actually an array of atoms
			    return prop_bag.getPropertyAsUint32 (".atom");
			});

    content.XProps.Add (Atoms._NET_WM_STRUT,
			function (prop_bag) {
			    return { partial: false,
				     left: prop_bag.getPropertyAsUint32 (".left"),
				     right: prop_bag.getPropertyAsUint32 (".right"),
				     top: prop_bag.getPropertyAsUint32 (".top"),
				     bottom: prop_bag.getPropertyAsUint32 (".bottom"),
				   };
			});

    content.XProps.Add (Atoms._NET_WM_STRUT_PARTIAL,
			function (prop_bag) {
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
		       });

    content.XProps.Add (Atoms._NET_WM_ICON_GEOMETRY,
			function (prop_bag) {
			    return { partial: false,
				     x: prop_bag.getPropertyAsUint32 (".x"),
				     y: prop_bag.getPropertyAsUint32 (".y"),
				     width: prop_bag.getPropertyAsUint32 (".width"),
				     height: prop_bag.getPropertyAsUint32 (".height")
				   };
		       });
}
