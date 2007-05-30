/* -*- mode: javascript; c-basic-offset: 4; indent-tabs-mode: t; -*- */


var overlayLayer = $("#overlayLayer")[0];
var overlayContent = $("#overlayContent", overlayLayer)[0];


function toggleOverlay ()
{
    var display = overlayLayer.style.display;
    overlayLayer.style.display = (!display || display == "none") ? 
	"block" : "none";

    if (overlayContent.src == "") {
        overlayContent.src = "http://www.google.com/ig";
    }
}


/*
document.addEventListener("keypress", {
                              handleEvent: function (event) {
				  if (event.keyCode == event.DOM_VK_F9 && event.ctrlKey) {
				      toggleOverlay ();
				  }
			      }
                          },
                          true);
*/
