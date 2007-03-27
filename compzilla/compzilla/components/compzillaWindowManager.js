
const CLSID        = Components.ID('{38054e08-223b-4eea-adcf-442c58945704}');
const CONTRACTID   = "@beatniksoftware.com/compzillaWindowManager";
const nsISupports  = Components.interfaces.nsISupports;
const nsIComponentRegistrar        = Components.interfaces.nsIComponentRegistrar;
const compzillaIWindowManager      = Components.interfaces.compzillaIWindowManager;

function CompzillaWindowManager() {}
CompzillaWindowManager.prototype =
{
  /* nsISupports */
  QueryInterface : function handler_QI(iid) {
    if (iid.equals(nsISupports))
      return this;
    if (iid.equals(compzillaIWindowManager))
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
     debug ("registerSelf");
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
     debug ("unregisterSelf");
    compMgr = compMgr.QueryInterface(nsIComponentRegistrar);

    compMgr.unregisterFactoryLocation(CLSID, fileSpec);
  },


  getClassObject: function (compMgr, cid, iid) {
     debug ("getClassObject");
     if (!cid.equals(CLSID))
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
