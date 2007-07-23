// lot of rss code ripped from here
// http://www.xml.com/pub/a/2006/09/13/rss-and-ajax-a-simple-news-reader.html

var flickrAnimationDuration = 750;

// 30 second timeout
var rss_sync_timeout = 30000;

var flickrFrame;

var flickrWindow = document.getElementById ("flickrWindow");
var flickrParent = document.getElementById ("flickrParent");
var flickrItemTemplate = document.getElementById ("flickrItemTemplate");
var flickrTimestamp = document.getElementById ("flickrTimestamp");
var flickrDiv = document.getElementById ("flickrDemo");
var flickrUrl = document.getElementById ("flickrUrl");

// 79 is the css height, the 5 is some extra margin
var flickrItemHeight = 79 + 5;

function flickrDemo (url)
{
  _addUtilMethods (this);

  this.generation = 0;

  this.addProperty ("url",
		    /* getter */
		    function () {
		      return this._url;
		    },
		    /* setter */
		    function (val) {
		      Debug ("setting flickr url to " + val);
		      this.generation ++;
		      this._url = val;
		      this.updateFlickrRss ();
		    });

  this.url = url;
}
flickrDemo.prototype = {
  updateFlickrRss: function () {
    if (this.timer) {
    	clearInterval (this.timer); /* we'll add it again once we've gotten the page */
	delete this.timer;
    }

    var demo = this;


    // we need permission to use XMLHttpRequest
    // - not when we're running in an extension, yay!
//     try {
//       netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
//     } catch (e) {
//       Debug("Permission UniversalBrowserRead denied.");
//     }

    var generation = this.generation;
    var client = new XMLHttpRequest();
    client.onreadystatechange = function () {
      Debug (" " + generation + " ?= " + demo.generation);
      if (generation != demo.generation)
	return;

      if (this.readyState == 4 && this.status == 200) {
	if (this.responseXML) {
	  // we need permission to iterate over the xml document
	  // - not when we're running in an extension, yay!
// 	  try {
// 	    netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
// 	  } catch (e) {
// 	    Debug("Permission UniversalBrowserRead denied.");
// 	  }
	  demo.parseRSS (this.responseXML);

	  demo.timer = setInterval (function( that ) { that.updateFlickrRss(); }, rss_sync_timeout, demo);
	}
      }
      else if (this.readyState == 4 && this.status != 200) {
	  Debug ("failed to get flickr rss");
	  demo.timer = setInterval (function( that ) { that.updateFlickrRss(); }, rss_sync_timeout, demo);
      }
    };
    Debug ("getting " + this.url);
    client.open("GET", this.url);
    client.send(null);
  },

  parseRSS: function (rssxml) {
    var t = new Date();

    
    flickrTimestamp.innerHTML = "Updated: " + t.toLocaleDateString() + " " + t.toLocaleTimeString();

    var rss = new RSS2Channel (rssxml);

    flickrFrame.title = rss.title;
    if (rss.image) {
      // XXX hack - don't do this at home, folks
      flickrFrame._icon.src = rss.image.url;
    }

    this.showRSS (rss);
  },

  showRSS: function (rsschan) {
    var demo = this; // needed for the animation callback

    var sb = new NIHStoryboard ();

    /* 1. delete all children now in the new rss */
    /* 2. mark all current children as not new */
    /* 3. add all new children */

    var children = { };
    var new_items = new Array ();

    // build up the lists of deleted children and new items at the
    // same time.
    for (var el = flickrDiv.firstChild; el != null; el = el.nextSibling) {
      children[el.rssItem.link] = el;
    }

    for (var i = 0; i < rsschan.items.length; i ++) {
      var rssitem = rsschan.items[i];
      if (children[rssitem.link])
	delete children[rssitem.link];
      else
	new_items.push (rssitem);
    }

    // animate all the children that we'll be deleting - make them
    // fadeout, and delete then when that animation finished.
    for (var l in children) {
      var child = children[l];
      child.removed = true;

      var a = new FadeOutAnimation (child, 20);

      // XXX this completed handler *should* be used, but for some
      // reason it's causing dom exceptions.  so, we do it in the
      // sb.completed function below.
      //a.completed = function () { flickrDiv.removeChild (child); };

      sb.addAnimation (a);
    }

    // for all the remaining items, figure out their new positions
    var offset = new_items.length * flickrItemHeight;
    for (var c = flickrDiv.firstChild; c != null; c = c.nextSibling) {

      // if the child is bound for deletion, skip it.
      if (c.removed)
	continue;

      // if the item was new, animate to the not-new state, and clear
      // the state when the animation finishes.
      if (c.getAttributeNS (_PYRO_NAMESPACE, "new-item") == "true") {
	var a = new NotNewColorAnimation (c.background, flickrAnimationDuration);
	var child = c;
	a.completed = function () {
	  child.setAttributeNS (_PYRO_NAMESPACE, "new-item", "false");
	}
	sb.addAnimation (a);
      }

      sb.addAnimation (new MoveTopAnimation (c, flickrAnimationDuration, offset));
      offset += flickrItemHeight;
    }

    // after we've moved everything out of the way, add the new items
    // to the display
    sb.completed = function () {
      // XXX this next loop really should be in the completed handler
      // for the fade out animations above, but for some reason I get
      // dom errors when i have it there */
      for (el = flickrDiv.lastChild; el; el = el.prevSibling)
	if (el.removed)
	  flickrDiv.removeChild (el);
      demo.addItems (new_items);
    };

    // kick it all off
    sb.start ();
  },

  addItems: function (items) {
    // now loop over the rss chan adding the new ones
    var y = 0;

    var sb = new NIHStoryboard ();

    var last = null;

    for (var i = 0; i < items.length; i ++) {
      var rssitem = items[i];

      var div = flickrItemTemplate.cloneNode (true);
      _addUtilMethods (div);

      div.background = div.getElementById ("flickrItemBackground");

      div.setAttributeNS (_PYRO_NAMESPACE, "new-item", "true");

      div.rssItem = rssitem;

      div.getElementById ("thumbnail").src = rssitem.thumbnail.url;

      var title = document.createTextNode (rssitem.title);
      div.getElementById("title").appendChild (title);

      var photographer = document.createTextNode (rssitem["media:credit"]);
      div.getElementById("photographer").appendChild (photographer);

      div.style.top = y + "px";
      div.style.display = "block";

      if (i == 0)
	flickrDiv.insertBefore (div, flickrDiv.firstChild);
      else
	flickrDiv.insertBefore (div, last.nextSibling);

      last = div;

      sb.addAnimation (new FadeInAnimation (div, flickrAnimationDuration));

      y += flickrItemHeight;
    }

      sb.start();
    },
}


function RSS2Channel(rssxml)
{
    /*array of RSS2Item objects*/
    this.items = new Array();

    var chanElement = rssxml.getElementsByTagName("channel")[0];

    for (var p = chanElement.firstChild; p != null; p = p.nextSibling) {
      if (!p || !p.tagName)
	continue;

      switch (p.tagName) {
      case "item":
        this.items.push(new RSS2Item(p));
	break;
      case "title":
      case "link":
      case "language":
      case "copyright":
      case "managingEditor":
      case "webMaster":
      case "pubDate":
      case "lastBuildDate":
      case "generator":
      case "docs":
      case "ttl":
      case "rating":
	if (!this[p.tagName]) {
	  try {
	    this[p.tagName] = p.childNodes[0].nodeValue;
	  }
	  catch (e) {
	    /*Debug ("exception getting " + properties[i] + " from item node");*/
	    this[p.tagName] = "";
	  }
	}

	break;
      case "image":
	this.image = new RSS2ChannelImage (p);
	break;
      }
    }
}

var done = false;

function RSS2Item(itemxml)
{
  for (var p = itemxml.firstChild; p != null; p = p.nextSibling) {
    if (!p || !p.tagName)
      continue;

    switch (p.tagName) {
    case "title":
    case "link":
    case "description":
    case "author":
    case "comments":
    case "pubDate":
    case "media:credit":
      if (!this[p.tagName]) {
	try {
	  this[p.tagName] = p.childNodes[0].nodeValue;
	}
	catch (e) {
	  /*Debug ("exception getting " + properties[i] + " from item node");*/
	  this[p.tagName] = "";
	}
      }
      break;

    case "media:content":
      if (!this.content) {
	try {
	  this.content = new RSS2Image (p);
	}
	catch (e) { /*Debug ("exception getting content from item node"); */};
      }
      break;
    case "media:thumbnail":
      if (!this.thumbnail) {
	try {
	  this.thumbnail = new RSS2Image (p);
	}
	catch (e) {
	  /*Debug ("exception getting thumbnail from item node: " + e);*/
	};
      }
      break;
    }
  }
}

function RSS2Image(imgElement)
{
    if (imgElement == null) {
      this.url = null;
      this.link = null;
      this.width = null;
      this.height = null;
      this.description = null;
    } else {
        imgAttribs = new Array("url","title","link","width","height","description");
        for (var i=0; i<imgAttribs.length; i++)
            if (imgElement.getAttribute(imgAttribs[i]) != null)
	      this[imgAttribs[i]] = imgElement.getAttribute (imgAttribs[i]);
    }
}

function RSS2ChannelImage(imgElement)
{
  for (var p = imgElement.firstChild; p != null; p = p.nextSibling) {
    if (!p || !p.tagName)
      continue;

    switch (p.tagName) {
    case "title":
    case "link":
    case "url":
      if (!this[p.tagName]) {
	try {
	  this[p.tagName] = p.childNodes[0].nodeValue;
	}
	catch (e) {
	  /*Debug ("exception getting " + properties[i] + " from item node");*/
	  this[p.tagName] = "";
	}
      }
      break;
    }
  }
}


function MoveTopAnimation (el, duration, to) {
    this.from_top = el.offsetTop;
    this.to_top = to;

    this.delta_top = this.to_top - this.from_top;

    this.duration = duration;
    this.el = el;
}

MoveTopAnimation.prototype = {
    updateProgress: function (progress) {
	var v = this.from_top + this.delta_top * progress;
	this.el.style.top = v + "px";
    }
}


function RGB2HTML(red, green, blue)
{
    var decColor = red + 256 * green + 65536 * blue;
    return decColor.toString(16);
}

function NotNewColorAnimation (el, duration) {

  this.from_red = 0x46;
  this.from_green = 0xaa;
  this.from_blue = 0x2d;

  this.to_red =
    this.to_green =
    this.to_blue = 0xaa;

  this.delta_red = this.to_red - this.from_red;
  this.delta_green = this.to_green - this.from_green;
  this.delta_blue = this.to_blue - this.from_blue;

  this.duration = duration;
  this.el = el;
}

NotNewColorAnimation.prototype = {
    updateProgress: function (progress) {
      var r = Math.floor (this.from_red + this.delta_red * progress);
      var g = Math.floor (this.from_green + this.delta_green * progress);
      var b = Math.floor (this.from_blue + this.delta_blue * progress);

      this.el.style.backgroundColor = "#" + RGB2HTML (r, g, b);
    }
}

function FadeOutAnimation (el, duration) {
  this.duration = duration;
  this.el = el;
}

FadeOutAnimation.prototype = {
    updateProgress: function (progress) {
      this.el.style.opacity = 1.0 - progress;
    }
}

function FadeInAnimation (el, duration) {
  this.duration = duration;
  this.el = el;
}

FadeInAnimation.prototype = {
    updateProgress: function (progress) {
      this.el.style.opacity = progress;
    }
}


function updateFlickrUrl ()
{
  Debug ("updateFlickrUrl called");

    url = flickrUrl.value;
    if (url) {
        flickrUrl.flickr.url = url;
    }
}

var default_url = "http://api.flickr.com/services/feeds/photos_public.gne?tags=hdr&lang=en-us&format=rss_200";

var flickr = new flickrDemo (default_url);

flickrFrame = CompzillaFrame (flickrWindow);
flickrFrame.flickr = flickr;

flickrUrl.value = default_url;
flickrUrl.flickr = flickr;

flickrFrame.title = "Flickr Feed";
flickrFrame.moveResize (10, 10, 210, 550);
flickrFrame.show ();
flickrFrame.allowClose =
  flickrFrame.allowMinimize =
  flickrFrame.allowMaximize = false;

windowStack.stackWindow (flickrFrame);

