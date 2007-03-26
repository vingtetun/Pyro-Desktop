
const CLSID        = Components.ID('{24bbc668-c039-478c-acb4-20734215bdf6}');
const CONTRACTID   = "@beatnicksoftware.com/compzillaWindowManager";
const nsISupports  = Components.interfaces.nsISupports;
const nsIComponentRegistrar        = Components.interfaces.nsIComponentRegistrar;

function CompzillaIWindowManager() {}
CompzillaIWindowManager.prototype =
{
  /* nsISupports */
  QueryInterface : function handler_QI(iid) {
    if (iid.equals(nsISupports))
      return this;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  },


  /* compzillaIWindowManager methods */
  WindowCreated : function(xid) {
     alert ("window " + xid + " created");
  },

  WindowDestroyed : function(xid) {
     alert ("window " + xid + " destroyed");
  },

  WindowMapped : function(xid) {
     alert ("window " + xid + " mapped");
  },

  WindowUnmapped : function(xid) {
     alert ("window " + xid + " unmapped");
  },

  WindowConfigured : function(xid) {
     alert ("window " + xid + " configured");
  },
};

var CompzillaWindowManagerModule =
{
  registerSelf : function(compMgr, fileSpec, location, type)
  {
     debug ("hi!");
     compMgr = compMgr.QueryInterface(nsIComponentRegistrar);

     compMgr.registerFactoryLocation(CLSID,
                                    "Compzilla Window Manager Service",
                                    CONTRACTID,
                                    fileSpec,
                                    location,
                                    type);
  },

  unregisterSelf : function(compMgr, fileSpec, location)
  {
    compMgr = compMgr.QueryInterface(nsIComponentRegistrar);

    compMgr.unregisterFactoryLocation(CLSID, fileSpec);
  },


  getClassObject: function (compMgr, cid, iid) {
     if (!cid.equals(this.myCID))
        throw Components.results.NS_ERROR_NO_INTERFACE;

     if (!iid.equals(Components.interfaces.nsIFactory))
        throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

     return this.myFactory;
  },

  myFactory: {
     /*
      * Construct an instance of the interface specified by iid, possibly
      * aggregating it with the provided outer.  (If you don't know what
      * aggregation is all about, you don't need to.  It reduces even the
      * mightiest of XPCOM warriors to snivelling cowards.)
      */
      createInstance: function (outer, iid) {
         debug("CI: " + iid + "\n");
         if (outer != null)
            throw Components.results.NS_ERROR_NO_AGGREGATION;

         return (new CompzillaWindowManager()).QueryInterface(iid);
      }
  }
};

function NSGetModule(compMgr, fileSpec) {
  return CompzillaWindowManagerModule;
}
