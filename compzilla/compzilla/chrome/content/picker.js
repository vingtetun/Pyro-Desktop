
var pickerLayer = $("#pickerLayer")[0];

function newWindow (w)
{
  var win = $("#pickerItem").clone()[0];
  win._contents = $("#window", win)[0];
  win._label = $("#label", win)[0];

  win._native = CompzillaWindowContent (w.content.nativeWindow);
  win._contents.appendChild (win._native);

  win._label.innerHTML = w.content.wmName;

  win.style.display = "block";

  return win;
}

var window_x = 20;

function addWindow (el)
{
  Debug ("addWindow (" + el + ")");
  $("#pickerWindow")[0].appendChild (el);
  el.style.left = window_x;

  window_x += 150 + 20;
}

function renderBeauty ()
{
  var canvas = $("#pickerWindowBeauty")[0];
  canvas.width = 620;
  canvas.height = 200;
  //  alert ("canvas = " + canvas.width + " x " + canvas.height);
  var ctx = canvas.getContext('2d');

  var gradient = ctx.createRadialGradient (canvas.width / 2, canvas.height / 2, 0, canvas.width / 2, canvas.height / 2, canvas.width / 2);

  gradient.addColorStop (0.0, 'rgba(0, 0, 0, 0.0)');
  gradient.addColorStop (0.4, 'rgba(115, 155, 155, 0.4)');
  gradient.addColorStop (0.7, 'rgba(180, 180, 180, 0.7)');
  gradient.addColorStop (0.8, 'rgba(200, 200, 200, 0.8)');
  gradient.addColorStop (1.0, 'rgba(255, 255, 255, 1.0)');

  ctx.fillStyle = gradient;

  ctx.fillRect (0, 0,
		canvas.width, canvas.height);
}

function populatePicker ()
{
  renderBeauty ();

  for (var el = windowStack.firstChild; el != null; el = el.nextSibling) {
    if (el.className == "uiDlg" /* it's a compzillaWindowFrame */
	&& el.content.nativeWindow /* it has native content */
	&& el.style.display == "block" /* it's displayed */) {
      addWindow (newWindow (el));
    }
  }
}

var shown = false;

function showPicker (forward)
{
  Debug ("showPicker called");
  Debug (pickerLayer);
  Debug (pickerLayer.style.display);

  if (!shown) {
    Debug ("yay!");
      	populatePicker ();

  	pickerLayer.style.display = "block";
	// XXX install key release handler to hide the layer

	shown = true;
  }
  else
    Debug ("BOO");

   if (forward)
     nextWindow ();
   else
     prevWindow ();
}

function nextWindow ()
{
  var vs = $(".pickerItem", $("#pickerWindow"));

  for (i = 0; i < vs.length; i ++) {
    var v = vs[i];
    $(v).animate ({left: v.offsetLeft - 220 }, "linear");
  }
}

function prevWindow ()
{
  var vs = $(".pickerItem", $("#pickerWindow"));

  for (i = 0; i < vs.length; i ++) {
    var v = vs[i];
    $(v).animate ({left: v.offsetLeft + 220 }, "linear");
  }
}
