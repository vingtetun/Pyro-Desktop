/* -*- mode: javascript; c-basic-offset: 4; indent-tabs-mode: t; -*- */

function NIHStoryboard () {
    this.animations = new Array();
}

NIHStoryboard.prototype = {
    clear : function () {
	clearInterval (this.timer);
	this.animations = new Array ();
    },

    addAnimation : function (a) {
	this.animations.push (a);
    },

    completed: function () { },

    start: function () {
	var sb = this;
	this.start_time = (new Date()).getTime();
	this.timer = setInterval (function () { sb.tick() } , 13);

	for (var i = 0; i < this.animations.length; i ++) {
	    this.animations[i].start_time = this.start_time;
	    this.animations[i].end_time = this.start_time + this.animations[i].duration;
	}
    },

    tick: function () {
	var t = (new Date()).getTime();

	var more = false;

	for (var i = 0; i < this.animations.length; i ++) {
	    var a = this.animations[i];
	    var progress;

	    if (t > a.end_time) {
		progress = 1.0;
	    }
	    else {
		progress = (t - a.start_time) / (a.end_time - a.start_time);
	    }

	    this.animations[i].updateProgress(progress);

	    if (progress < 1.0)
		more = true;
	}

	if (!more) {
	    clearInterval (this.timer);

	    for (var i = 0; i < this.animations.length; i ++) {
		var a = this.animations[i];
		if (typeof (a.completed) == 'function')
		    a.completed();
	    }

	    this.completed ();
	}
    }
};
