/* -*- mode: javascript; c-basic-offset: 4; indent-tabs-mode: t; -*- */

var pickerLayer = document.getElementById ("pickerLayer");
var pickerContents = document.getElementById ("pickerContents");
_addUtilMethods (pickerContents);
var pickerItemTemplate = document.getElementById ("pickerItem");
var pickerCanvas = document.getElementById ("pickerWindowBeauty");

var pickerItems = null;

var pickerWidth = 600;
var pickerItemWidth = 150;
var pickerItemPadding = 20;

function renderBeauty ()
{
    pickerCanvas.width = pickerWidth;
    pickerCanvas.height = 200;
    var ctx = pickerCanvas.getContext('2d');

    var gradient = ctx.createRadialGradient (pickerCanvas.width / 2, pickerCanvas.height / 2,
					     0,
					     pickerCanvas.width / 2, pickerCanvas.height / 2, pickerCanvas.width / 2);

    gradient.addColorStop (0.0, 'rgba(0, 0, 0, 0.0)');
    gradient.addColorStop (0.4, 'rgba(115, 155, 155, 0.4)');
    gradient.addColorStop (0.7, 'rgba(180, 180, 180, 0.7)');
    gradient.addColorStop (0.8, 'rgba(200, 200, 200, 0.8)');
    gradient.addColorStop (1.0, 'rgba(255, 255, 255, 1.0)');

    ctx.fillStyle = gradient;

    ctx.fillRect (0, 0,
		  pickerCanvas.width, pickerCanvas.height);
}


function newWindow (w)
{
    var item = pickerItemTemplate.cloneNode(true);
    _addUtilMethods (item);

    item.original_window = w;

    item._contents = item.getElementById ("window");
    item._label = item.getElementById ("label");
    
    item._native = CompzillaWindowContent (w.content.nativeWindow);

    _addUtilMethods (item._native); /* this facilitates the centering we do in populate picker */

    item._contents.appendChild (item._native);

    item._label.innerHTML = w.content.wmName;

    item.style.display = "block";

    _addUtilMethods (item);

    return item;
}

function addWindow (el)
{
    var win = newWindow (el);
    pickerContents.appendChild (win);
    pickerItems.push (win);
}

function sizeCanvas (pickerItem)
{
    // the width calculation stuff here relies on the fact that the
    // items are pickerItemWidth pixels, and assumes the canvases are
    // centered.
    var off = pickerItem._native.getPosition().left - pickerItem.getPosition().left;

    var size = pickerItemWidth - 2 * off;
    pickerItem._native.width = size;
    pickerItem._native.height = size;
}

function populatePicker (forward)
{
    renderBeauty ();

    pickerItems = new Array ();

    for (var el = windowStack.firstChild; el != null; el = el.nextSibling) {
	if (el.className == "uiDlg" /* it's a compzillaWindowFrame */
	    && el.content.nativeWindow /* it has native content */
	    && el.style.display == "block" /* it's displayed */) {
	    addWindow (el);
	}
    }

    if (pickerItems.length == 0) 
	return;

    var win_x;

    win_x = pickerItems[0].offsetLeft;
    sizeCanvas (pickerItems[0]);

    for (var i = 1; i < pickerItems.length; i ++) {
	var pickerItem = pickerItems[i];
	win_x += pickerItemWidth + pickerItemPadding;

	pickerItem.style.left = win_x + "px";
	sizeCanvas (pickerItem);
    }
}

function clearPicker ()
{
    for (i = 0; i < pickerItems.length; i ++) {
	var v = pickerItems[i];
	pickerContents.removeChild (v);
    }
}

var shown = false;

function showPicker (forward)
{
    if (!shown) {
      	populatePicker (forward);

  	pickerLayer.style.display = "block";

	document.addEventListener("keyup", {
	                              handleEvent: function (event) {
					  if (event.keyCode == event.DOM_VK_CONTROL) {
					      pickerLayer.style.display = "none";
					      shown = false;

					      pickerItems[selected_window].original_window.doFocus();

					      clearPicker ();

					      document.removeEventListener ("keyup", this, true);
					  }
				      }
				  },
				  true);

	shown = true;


        if (forward) {
	    selected_window = 0;
        }
        else {
	    selected_window = pickerItems.length - 1;
        }

	pickerItems[selected_window].setAttributeNS (_PYRO_NAMESPACE, "selected-picker-item", "true");

 	var contentPos = pickerContents.getPosition ();

 	var delta_to_center = (screen.width / 2) - contentPos.left - (pickerItems[selected_window].offsetLeft + pickerItems[selected_window].offsetWidth / 2);

 	pickerContents.style.left = (pickerContents.offsetLeft + delta_to_center) + "px";
    }

    if (pickerItems.length <= 1) {
	return;
    }

    if (forward) {
	nextWindow ();
    }
    else {
	prevWindow ();
    }
}

var selected_window;

var timer;
var start_time;
var end_time;
var progress;  /* 0-1, 0 = start_time, 1 = end_time */
var tick;      /* callback */
var completed; /* callback */

function update ()
{
    var t = (new Date()).getTime();

    if (t > end_time) {
	clearInterval (timer);
	progress = 1.0;
	completed ();
    }
    else {
	progress = (t - start_time) / (end_time - start_time);
    }

    tick (progress);
}

var from_x;
var to_x;

function movePickerContentsTick (progress)
{
    var v = from_x + (to_x - from_x) * progress;
    pickerContents.style.left = v + "px";
}

var animating;

function movePickerContentsCompleted ()
{
    animating = false;
    pickerItems[selected_window].setAttributeNS (_PYRO_NAMESPACE, "selected-picker-item", "true");
}

function nextWindow ()
{
    if (animating)
	return;

    if (selected_window == pickerItems.length - 1)
	return;

    var delta = pickerItems[selected_window].offsetLeft - pickerItems[selected_window + 1].offsetLeft;

    pickerItems[selected_window].setAttributeNS (_PYRO_NAMESPACE, "selected-picker-item", "false");

    selected_window ++;

    start_time = (new Date()).getTime();
    end_time = start_time + 200 /* duration */;
    from_x = pickerContents.offsetLeft;
    to_x = pickerContents.offsetLeft + delta;
    tick = movePickerContentsTick;
    completed = movePickerContentsCompleted;
    timer = setInterval (update, 13);
    animating = true;
}

function prevWindow ()
{
    if (animating)
	return;

    if (selected_window == 0)
	return;

    var delta = pickerItems[selected_window].offsetLeft - pickerItems[selected_window - 1].offsetLeft;

    pickerItems[selected_window].setAttributeNS (_PYRO_NAMESPACE, "selected-picker-item", "false");

    selected_window --;

    start_time = (new Date()).getTime();
    end_time = start_time + 200 /* duration */;
    from_x = pickerContents.offsetLeft;
    to_x = pickerContents.offsetLeft + delta;
    tick = movePickerContentsTick;
    completed = movePickerContentsCompleted;
    timer = setInterval (update, 13);
    animating = true;
}
