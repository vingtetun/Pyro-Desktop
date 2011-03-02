/* -*- mode: javascript; c-basic-offset: 2; indent-tabs-mode: t; -*- */

let overlayLayer = document.getElementById("overlayLayer");
let overlayContent = document.getElementById("overlayContent");
let display = null;


function toggleOverlay () {
  display = overlayLayer.style.display;
  overlayLayer.style.display = (!display || display == "none") ? "block" : "none";

  if (overlayContent.getAttribute("src") == "")
    overlayContent.setAttribute("src", "http://pyrodesktop.org");
}

