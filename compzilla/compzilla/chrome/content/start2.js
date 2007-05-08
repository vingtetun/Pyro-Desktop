
const compzillaIControl = Components.interfaces.compzillaIControl;


function ShowListener2 (frame, content) {
	this.frame = frame;
	this.content = content;
}
ShowListener2.prototype.handleEvent = function (ev) {
	this.frame.style.visibility = "visible";
	this.content.style.visibility = "visible";

	stack = document.getElementById ("compzillaWindowStack");
	stack.innerHTML = "SHOW2 " + stack.innerHTML;
}


function HideListener2 (frame, content) { 
	this.frame = frame; 
	this.content = content;
}
HideListener2.prototype.handleEvent = function (ev) { 
	this.frame.style.visibility = "hidden";
	this.content.style.visibility = "hidden";

	stack = document.getElementById ("compzillaWindowStack");
	stack.innerHTML = "HIDE2 " + stack.innerHTML;
}


function ShowListener (frame) { this.frame = frame; }
ShowListener.prototype.handleEvent = function (ev) { this.frame.show (ev); }

function HideListener (frame) { this.frame = frame; }
HideListener.prototype.handleEvent = function (ev) { this.frame.hide (ev); }

function DestroyListener (frame) { this.frame = frame; }
DestroyListener.prototype.handleEvent = function (ev) { this.frame.windowDestroy (ev); }

function MoveResizeListener (frame) { this.frame = frame; }
MoveResizeListener.prototype.handleEvent = function (ev) { this.frame.moveResize (ev); }

function PropertyChangeListener (frame) { this.frame = frame; }
PropertyChangeListener.prototype.handleEvent = function (ev) { this.frame.propertyChange (ev); }


function CompzillaWindowFrame (nativewin) {
	this._nativewin = nativewin;
	this._makeFrameNode ();

	//this._commands = new CompzillaCommands (this);

	this._nativewin.addEventListener ("destroy", new DestroyListener (this), true);
	this._nativewin.addEventListener ("moveresize", new MoveResizeListener (this), true);
	this._nativewin.addEventListener ("show", new ShowListener (this), true);
	this._nativewin.addEventListener ("hide", new HideListener (this), true);
	this._nativewin.addEventListener ("propertychange", new PropertyChangeListener (this), 
					  true);
};
CompzillaWindowFrame.prototype = {
	_nativewin: null,
	_frame: null,
	_content: null,
	_commands: null,

	get frame () { return this._frame; },
	set frame (aFrame) { this._frame = aFrame; },

	get content () { return this._content; },
	set content (aContent) { this._content = aContent; },

	get commands () { return this._commands; },
	set commands (aCommands) { this._commands = aCommands; },

	_makeFrameNode: function () {
		this._frame = document.getElementById ("compzillaWindowFrame").cloneNode (true);
		this._content = 
			document.getElementById ("compzillaWindowContent").cloneNode (true);
		this._frame.appendChild (this._content);
		this._nativewin.AddContentNode (this._content);
        },

	windowDestroy: function (ev) {
		if (this._frame && this._frame.parentNode) {
			this._frame.parentNode.removeChild (this._frame);
		}

		stack = document.getElementById ("compzillaWindowStack");
		stack.innerHTML = "DESTROY " + stack.innerHTML;
        },

	moveResize: function (ev) {
		if (this._content.offsetWidth != ev.width) {
			this._content.style.width = ev.width + "px";
		}
		if (this._content.offsetHeight != ev.height) {
			this._content.style.height = ev.height + "px";
		}
		if (this._frame.offsetLeft != ev.x) {
			this._frame.style.left = ev.x + "px";
		}
		if (this._frame.offsetTop != ev.y) {
			this._frame.style.top = ev.y + "px";
		}

		stack = document.getElementById ("compzillaWindowStack");
		stack.innerHTML = "MOVE " + stack.innerHTML;
	},

	show: function (ev) {
		this._frame.style.visibility = "visible";
		this._content.style.visibility = "visible";

		stack = document.getElementById ("compzillaWindowStack");
		stack.innerHTML = "SHOW " + stack.innerHTML;
	},

	hide: function (ev) {
		this._frame.style.visibility = "hidden";
		this._content.style.visibility = "hidden";

		stack = document.getElementById ("compzillaWindowStack");
		stack.innerHTML = "HIDE " + stack.innerHTML;
	},

	propertyChange: function (ev) {
		stack = document.getElementById ("compzillaWindowStack");
		stack.innerHTML = "PROPERTY " + stack.innerHTML;
	},
};


function compzillaLoad()
{
   var xulwin = document.getElementById ("desktopWindow");
   xulwin.width = screen.width;
   xulwin.height = screen.height;

   svccls = Components.classes['@pyrodesktop.org/compzillaService'];
   svc = svccls.getService(compzillaIControl);

   svc.addEventListener (
	"windowcreate", 
	{
	    handleEvent: function (ev) {
		// NOTE: Be careful to not do anything which will create a new
		//       toplevel window, to avoid an infinite loop.

		stack = document.getElementById ("compzillaWindowStack");

		frame = new CompzillaWindowFrame (ev.window);

		// ev is a compzillaIWindowConfigureEvent
		frame.moveResize (ev);
		if (ev.mapped) {
			frame.show (ev);
		}

		//frame.style.visibility = "visible";
		//content.style.visibility = "visible";

		stack = document.getElementById ("compzillaWindowStack");
    		stack.appendChild (frame);
	    }
        },
	true);

   alert("Calling RegisterWindowManager!");

   // Register as the window manager and generate windowcreate events for
   // existing windows.
   svc.RegisterWindowManager (window);
}
