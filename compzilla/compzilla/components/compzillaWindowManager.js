
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

  document: null,

  /* compzillaIWindowManager methods */
  WindowCreated : function(xid) {
     //alert ("window " + xid + " created");

     var content = this.document.createElement ("canvas");

     cls = Components.classes['@beatniksoftware.com/compzillaService'];
     svc = cls.getService(Components.interfaces.compzillaIControl);

     var chrome_root = this.CreateWMChrome (content, xid);

     this.document.body.appendChild (chrome_root)

     return content;
  },

  WindowDestroyed : function(content) {
     var chrome_root = content.crome;
     document.removeChild (chrome_root);
  },

  WindowMapped : function(content) {
     var chrome_root = content.chrome;
     content.chrome.style.visibility = "visible";
  },

  WindowUnmapped : function(content) {
     var chrome_root = content.chrome;
     content.chrome.style.visibility = "hidden";
  },

  WindowConfigured : function(content, x, y, width, height) {
     var chrome_root = content.chrome;

     chrome_root.style.left = x;
     chrome_root.style.top = y;

     var need_relayout = false;

     if (chrome_root.offsetWidth != width) {
	chrome_root.style.width = width;
	need_relayout = true;
     }
     if (chrome_root.offsetHeight != height) {
	chrome_root.style.height = height;
	need_relayout = true;
     }

     if (need_relayout)
	this.LayoutChrome (chrome_root);
  },

   SetDocument : function(doc) {
     this.document = doc;
  },

  CreateWMChrome : function (content, xid) {
     var root = this.document.createElement ("div");
     var titlebar = this.document.createElement ("div");
     var title = this.document.createElement ("span");

     titlebar.appendChild (title);
     root.appendChild (titlebar);
     root.appendChild (content);

     root.style.width = 200;
     root.style.hidth = 100;

     root.className = "window";
     titlebar.className = "titlebar";
     title.className = "windowtitle";
     content.className = "content";

     content.style.background = "#" + genHex();
     
     // back reference so we can find the chrome in all the other methods
     content.chrome = root;

     // windows start hidden initially
     root.style.visibility = "hidden";

     // a couple of convenience refs
     root.titlebar = titlebar;
     root.content = content;

     this.LayoutChrome (root);

     return root;
  },

 BorderSize: 3,
 TitleContentGap: 3,
 TitleBarHeight: 15,
 CornerSize: 25,

 LayoutChrome: function(element) {

      var bs = this.BorderSize;
      var tbs = this.TitleBarHeight;
      var tcg = this.TitleContentGap;

      element.titlebar.style.left = bs;
      element.titlebar.style.top = bs;
      element.titlebar.style.width = element.offsetWidth - 2 * bs;
      element.titlebar.style.height = tbs;

      element.content.style.left = bs;
      element.content.style.top = element.titlebar.offsetTop + element.titlebar.offsetHeight + tcg;
      element.content.style.width = element.offsetWidth - 2 * bs;
      element.content.style.height = element.offsetHeight - element.content.offsetTop - bs;
   },
};


function genHex(){
colors = new Array(14)
colors[0]="0"
colors[1]="1"
colors[2]="2"
colors[3]="3"
colors[4]="4"
colors[5]="5"
colors[5]="6"
colors[6]="7"
colors[7]="8"
colors[8]="9"
colors[9]="a"
colors[10]="b"
colors[11]="c"
colors[12]="d"
colors[13]="e"
colors[14]="f"

digit = new Array(5)
color=""
for (i=0;i<6;i++){
digit[i]=colors[Math.round(Math.random()*14)]
color = color+digit[i]
}

return color;
}

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


  getClassObject: function (compMgr, cid, iid) 
  {
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
