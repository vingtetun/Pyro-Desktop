/* -*- mode: javascript; c-basic-offset: 4; indent-tabs-mode: t; -*- */

var desktop = document.getElementById ("desktop");

desktop.Atoms = {
    svc: null,
    cache : new Object (),

    Get : function (atom_name) {
      if (this.svc == null) {
	  cls = Components.classes['@pyrodesktop.org/compzillaService'];
	  this.svc = cls.getService(Components.interfaces.compzillaIControl);
      }

      if (this.cache[atom_name] == null)
	  this.cache[atom_name] = this.svc.InternAtom (atom_name);

      return this.cache[atom_name];
    },

    WM_NAME : function () { return this.Get ("WM_NAME"); },
    WM_NORMAL_HINTS : function () { return this.Get ("WM_NORMAL_HINTS"); },
    _NET_WM_NAME : function () { return this.Get ("_NET_WM_NAME"); },
    _NET_WM_WINDOW_TYPE : function () { return this.Get ("_NET_WM_WINDOW_TYPE"); },
    _NET_WM_WINDOW_TYPE_DOCK : function () { return this.Get ("_NET_WM_WINDOW_TYPE_DOCK"); },
    _NET_WM_WINDOW_TYPE_MENU : function () { return this.Get ("_NET_WM_WINDOW_TYPE_MENU"); },
    _NET_WM_WINDOW_TYPE_DESKTOP : function () { return this.Get ("_NET_WM_WINDOW_TYPE_DESKTOP"); },
    _NET_WM_WINDOW_TYPE_SPLASH : function () { return this.Get ("_NET_WM_WINDOW_TYPE_SPLASH"); },
}
