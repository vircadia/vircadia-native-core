//
//	uiwidgets.js
//	examples/libraries
//
//	Created by Seiji Emery, 8/10/15
//	Copyright 2015 High Fidelity, Inc
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


(function(){

// Setup externals
(function() {

// We need a Vec2 impl, with add() and a clone function. If this is not part of hifi, we'll just add it:
if (this.Vec2 == undefined) {
	var Vec2 = this.Vec2 = function (x, y) {
		this.x = x || 0.0;
		this.y = y || 0.0;
	}
	Vec2.sum = function (a, b) {
		return new Vec2(a.x + b.x, a.y + b.y);
	}
	Vec2.clone = function (v) {
		return new Vec2(v.x, v.y);
	}
} else if (this.Vec2.clone == undefined) {
	print("Vec2 exists; adding Vec2.clone");
	this.Vec2.clone = function (v) {
		return { 'x': v.x || 0.0, 'y': v.y || 0.0 };
	}
} else {
	print("Vec2...?");
}
})();

var Rect = function (xmin, ymin, xmax, ymax) {
	this.x0 = xmin;
	this.y0 = ymin;
	this.x1 = xmax;
	this.y1 = ymax;
}
Rect.prototype.grow = function (pt) {
	this.x0 = Math.min(this.x0, pt.x);
	this.y0 = Math.min(this.y0, pt.y);
	this.x1 = Math.max(this.x1, pt.x);
	this.y1 = Math.max(this.y1, pt.y);
}
Rect.prototype.getWidth = function () {
	return this.x1 - this.x0;
}
Rect.prototype.getHeight = function () {
	return this.y1 - this.y0;
}
Rect.prototype.getTopLeft = function () {
	return { 'x': this.x0, 'y': this.y0 };
}
Rect.prototype.getBtmRight = function () {
	return { 'x': this.x1, 'y': this.y1 };
}
Rect.prototype.getCenter = function () {
	return { 
		'x': 0.5 * (this.x1 + this.x0),
		'y': 0.5 * (this.y1 + this.y0)
	};
}

var __trace = new Array();
var __traceDepth = 0;

var assert = function (cond, expr) {
	if (!cond) {
		var callstack = "";
		var maxRecursion = 10;
		caller = arguments.callee.caller;
		while (maxRecursion > 0 && caller) {
			--maxRecursion;
			callstack += ">> " + caller.toString();
			caller = caller.caller;
		}
		throw new Error("assertion failed: " + expr + " (" + cond + ")" + "\n" +
			"Called from: " + callstack + " " +
			"Traceback: \n\t" + __trace.join("\n\t"));
	}
}
var traceEnter = function(fcn) {
	var l = __trace.length;
	// print("TRACE ENTER: " + (l+1));
	s = "";
	for (var i = 0; i < __traceDepth+1; ++i)
		s += "-";
	++__traceDepth;
	__trace.push(s + fcn);
	__trace.push(__trace.pop() + ":" + this);
	return {
		'exit': function () {
			--__traceDepth;
			// while (__trace.length != l)
				// __trace.pop();
		}
	};
}

// UI namespace
var UI = this.UI = {};

var rgb = UI.rgb = function (r, g, b) {
	if (typeof(r) == 'string') {
		rs = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(r);
		if (rs) {
			r = parseInt(rs[0], 16);
			g = parseInt(rs[1], 16);
			b = parseInt(rs[2], 16);
		}
	}
	if (typeof(r) != 'number' || typeof(g) != 'number' || typeof(b) != 'number') {
		ui.err("Invalid args to UI.rgb (" + r + ", " + g + ", " + b + ")");
		return null;
	}
	return { 'r': r, 'g': g, 'b': b };
}
var rgba = UI.rgba = function (r, g, b, a) {
	if (typeof(r) == 'string')
		return rgb(r);
	return { 'r': r || 0, 'g': g || 0, 'b': b || 0, 'a': a };
}

// Protected UI state
var ui = {
	defaultVisible: true,
	widgetList: new Array(),
	attachmentList: new Array()
};

ui.complain = function (msg) {
	print("WARNING (uiwidgets.js): " + msg);
}
ui.errorHandler = function (err) {
	print(err);
}
ui.assert = function (condition, message) {
	if (!condition) {
		message = "FAILED ASSERT (uiwidgets.js): " + message || "(" + condition + ")";
		ui.errorHandler(message);
		if (typeof(Error) !== 'undefined')
			throw new Error(message);
		throw message;
	}
}

UI.setDefaultVisibility = function (visible) {
	ui.defaultVisible = visible;
}

// Hide overlays impl
function makeOverlay(type, properties) {
	var _TRACE = traceEnter.call(this, "makeOverlay");
	var overlay = Overlays.addOverlay(type, properties);
	// overlay.update = function (properties) {
	// 	Overlays.editOverlay(overlay, properties);
	// }
	// overlay.destroy = function () {
	// 	Overlays.deleteOverlay(overlay);
	// }
	// return overlay;
	_TRACE.exit();
	return {
		'update': function (properties) {
			var _TRACE = traceEnter.call(this, "Overlay.update");
			Overlays.editOverlay(overlay, properties);
			_TRACE.exit();
		},
		'destroy': function () {
			var _TRACE = traceEnter.call(this, "Overlay.destroy");
			Overlays.deleteOverlay(overlay);
			_TRACE.exit();
		},
		'getId': function () {
			return overlay;
		}
	}
}

var COLOR_WHITE = rgb(255, 255, 255);
var COLOR_GRAY  = rgb(125, 125, 125);

// Widget base class
var Widget = function () {};

__widgetId = 0;

Widget.prototype.constructor = function () {
	var _TRACE = traceEnter.call(this, "Widget.constructor()");
	this.position = { 'x': 0.0, 'y': 0.0 };
	this.visible = ui.defaultVisible;
	this.actions = {};
	this._dirty = true;
	this.parent = null;
	this._parentVisible = true;
	this.id = __widgetId++;
	ui.widgetList.push(this);
	assert(this.position, "this.position != undefined");
	_TRACE.exit();
}

Widget.prototype.setPosition = function () {
	var _TRACE = traceEnter.call(this, "Widget.setPosition()");
	if (arguments.length == 1) {
		(function (pos) {
			this.position = { x: pos.x, y: pos.y };
		}).apply(this, arguments);
	} else {
		(function (x, y) {
			this.position = { x: x, y: y };
		}).apply(this, arguments);
	}
	this.setDirty();
	_TRACE.exit();
}
Widget.prototype.setVisible = function(visible) {
	var _TRACE = traceEnter.call(this, "Widget.setVisible()");
	if (this.visible != visible) {
		this.setDirty();
	}
	this.visible = visible;
	_TRACE.exit();
}
Widget.prototype.isVisible = function () {
	return this.visible && this._parentVisible;
}
Widget.prototype.getOverlay = function () {
	return null;
}
Widget.prototype.addAction = function (name, callback) {
	var _TRACE = traceEnter.call(this, "Widget.addAction()");
	if (!this.actions[name]) {
		this.actions[name] = [ callback ];
	} else {
		this.actions[name].push(callback);
	}
	_TRACE.exit();
}
Widget.prototype.setDirty = function () {
	this._dirty = true;
}

var __first = 0;
Widget.prototype.toString = function () {
	// var _TRACE = (__trace[__trace.length - 1] != "Widget.toString()") && __first++ == 0 ?
		// traceEnter.call(this, "Widget.toString()") : { 'exit': function(){} };
	// assert(this.position, "this.position != undefined");
	// _TRACE.exit();
	return "[Widget " + this.id + " ]"//+" (pos=(" + this.position.x + ", " + this.position.y + "), parent=" + this.parent + ")";
}

var WidgetContainer = UI.WidgetContainer = function () {
	Widget.prototype.constructor.call(this);
	this.children = new Array();
	this._lastPos = { x: this.position.x, y: this.position.y };
};
WidgetContainer.prototype = new Widget();
WidgetContainer.prototype.constructor = WidgetContainer;

// WidgetContainer.prototype.setPosition = function (pos) {
// 	if (arguments.length > 1)
// 		pos = { x: arguments[0], y: arguments[1] };
// 	this.newPos = pos;
// 	this.setDirty();
// }

WidgetContainer.prototype.add = function (child) {
	this.children.push(child);
	child.parent = this;
	return child;
}
WidgetContainer.prototype.relayout = function (layout) {
	var rx = this.position.x - this._lastPos.x;
	var ry = this.position.y - this._lastPos.y;
	if (rx || ry) {
		this.children.forEach(function (child) {
			child.setPosition(child.position.x + rx, child.position.y + ry);
			child.relayout(layout);
		}, this);
	}
	this._lastPos.x = this.position.x;
	this._lastPos.y = this.position.y;
	this._dirty = false;
}
WidgetContainer.prototype.getWidth = function () {
	return 0;
}
WidgetContainer.prototype.getHeight = function () {
	return 0;
}
WidgetContainer.prototype.hasOverlay = function () {
	return false;
}

// Inherits from Widget
var WidgetStack = UI.WidgetStack = function (properties) {
	var _TRACE = traceEnter.call(this, "WidgetStack.constructor()");
	Widget.prototype.constructor.call(this);
	assert(ui.widgetList[ui.widgetList.length-1] === this, "ui.widgetList.back() == this");

	properties = properties || {};
	properties['dir'] = properties['dir'] || '+y';

	var dir = undefined;
	switch(properties['dir']) {
		case '+y': dir = { 'x': 0.0, 'y': 1.0 }; break;
		case '-y': dir = { 'x': 0.0, 'y': -1.0 }; break;
		case '+x': dir = { 'x': 1.0, 'y': 0.0 }; break;
		case '-x': dir = { 'x': -1.0, 'y': 0.0 }; break;
		default: ui.complain("Unrecognized UI.WidgetStack property 'dir': \"" + dir + "\"");
	}
	dir = dir || { 'x': 1.0, 'y': 0.0 };

	this.layoutDir = dir;
	this.border  = properties.border  || { 'x': 0.0, 'y': 0.0 };
	this.padding = properties.padding || { 'x': 0.0, 'y': 0.0 };
	this.visible = properties.visible != undefined ? properties.visible : this.visible;

	if (properties.background) {
		var background = properties.background;
		background.x = this.position ? this.position.x : 0;
		background.y = this.position ? this.position.y : 0;
		background.width  = background.width  || 100.0;
		background.height = background.height || 100.0;
		background.backgroundColor = background.backgroundColor || COLOR_GRAY;
		background.backgroundAlpha = background.backgroundAlpha || 0.5;
		background.textColor = background.textColor || COLOR_WHITE;
		background.alpha = background.alpha || 1.0;
		background.visible = this.visible;
		this.backgroundOverlay = makeOverlay("text", background);
	}

	this.widgets = new Array();

	_TRACE.exit();
}
WidgetStack.prototype = new Widget();
WidgetStack.prototype.constructor = WidgetStack;

WidgetStack.prototype.toString = function () {
	return "[WidgetStack " + this.id + " ]";
}

WidgetStack.prototype.add = function (widget) {
	var _TRACE = traceEnter.call(this, "WidgetStack.add()");
	this.widgets.push(widget);
	widget.parent = this;
	this.setDirty();

	assert(this.widgets, "this.widgets != undefined");

	_TRACE.exit();
	return widget;
}
WidgetStack.prototype.relayout = function (layoutFrom) {
	var _TRACE = traceEnter.call(this, "WidgetStack.relayout()");
	// print("this = " + this);
	// print("this.position = { " + this.position.x + ", " + this.position.y + " }")

	// this.position.x += 500;

	// this.setPosition({
	// 	x: this.position.x + 5,
	// 	y: this.position.y
	// });

	var layoutStart = {
		'x': this.position.x + this.border.x,
		'y': this.position.y + this.border.y
	}

	var layout = {
		'pos':    Vec2.clone(layoutStart),
		'bounds': new Rect(this.position.x, this.position.y, this.position.x, this.position.y)
	};
	// print("this.widgets = " + this.widgets);
	assert(this.widgets, "this.widgets != undefined");

	if (this.parent == null)
		this._parentVisible = true;
	var this_isVisible = this.isVisible();

	this.widgets.forEach(function (child, i, elems) {
		child.setPosition(layout.pos.x, layout.pos.y);
		child._parentVisible = this_isVisible;
		child.relayout(layout);

		if (child.isVisible()) {
			layout.bounds.grow({
				'x': layout.pos.x + child.getWidth(),
				'y': layout.pos.y + child.getHeight()
			});
			layout.pos.x += (child.getWidth()  + this.padding.x) * this.layoutDir.x;
			layout.pos.y += (child.getHeight() + this.padding.y) * this.layoutDir.y;
		}
		// elems[i] = child;
	}, this);

	this._dirty = false;

	layout.bounds.grow(Vec2.sum(layout.bounds.getBtmRight(), this.border));
	layout.pos.x += this.border.x * this.layoutDir.x;
	layout.pos.y += this.border.y * this.layoutDir.y;

	this._calculatedDimensions = {
		'x': layout.bounds.getWidth(),
		'y': layout.bounds.getHeight()
	};
	if (this.backgroundOverlay) {
		this.backgroundOverlay.update({
			'x': this.position.x,// - this.border.x,
			'y': this.position.y,// - this.border.y,
			'width':  this._calculatedDimensions.x,
			'height': this._calculatedDimensions.y,
			'visible': this.visible
		});
	}
	print("RELAYOUT " + this);
	print("layoutDir: " + this.layoutDir.x + ", " + this.layoutDir.y);
	print("padding: " + this.padding.x + ", " + this.padding.y);
	print("topleft: " + layout.bounds.getTopLeft().x + ", " + layout.bounds.getTopLeft().y);
	print("btmright: " + layout.bounds.getBtmRight().x + ", " + layout.bounds.getBtmRight().y);
	print("width, height = " + layout.bounds.getWidth() + ", " + layout.bounds.getHeight());
	print("start: " + this.position.x + ", " + this.position.y);
	print("end:   " + layout.pos.x + ", " + layout.pos.y);
	

	if (layoutFrom) {
		layoutFrom.pos = layout.pos;
		layoutFrom.bounds.grow(layout.getTopLeft());
		layoutFrom.bounds.grow(layout.getBtmRight());
	}
	_TRACE.exit();
}
WidgetStack.prototype.getWidth = function () {
	var _TRACE = traceEnter.call(this, "WidgetStack.getWidth()");
	if (this._dirty || !this._calculatedDimensions)
		this.relayout();
	_TRACE.exit();
	return this._calculatedDimensions.x;
}
WidgetStack.prototype.getHeight = function () {
	var _TRACE = traceEnter.call(this, "WidgetStack.getHeight()");
	if (this._dirty || !this._calculatedDimensions)
		this.relayout();
	_TRACE.exit();
	return this._calculatedDimensions.y;
}
WidgetStack.prototype.show = function () {
	this.relayout();
}
WidgetStack.prototype.hasOverlay = function (overlayId) {
	return this.backgroundOverlay && this.backgroundOverlay.getId() === overlayId;
}
WidgetStack.prototype.getOverlay = function () {
	return this.backgroundOverlay;
}
WidgetStack.prototype.setDirty = function () {
	this._dirty = true;
	this.widgets.forEach(function(widget){
		widget.setDirty();
	});
}
WidgetStack.prototype.destroy = function () {
	var _TRACE = traceEnter.call(this, "WidgetStack.destroy()");
	if (this.backgroundOverlay) {
		this.backgroundOverlay.destroy();
		this.backgroundOverlay = null;
	}
	_TRACE.exit();
}
WidgetStack.prototype.setColor = function (color) {
	if (arguments.length != 1) {
		color = rgba.apply(arguments);
	}
	this.backgroundOverlay.update({
		'color': color,
		'alpha': color.a
	});
}

var Icon = UI.Icon = function (properties) {
	var _TRACE = traceEnter.call(this, "Icon.constructor()");
	Widget.prototype.constructor.call(this);

	this.visible = properties.visible != undefined ? properties.visible : this.visible;
	this.width  = properties.width  || 1.0;
	this.height = properties.height || 1.0;

	var iconProperties = {
		'color':    properties.color || COLOR_GRAY,
		'alpha':    properties.alpha || 1.0,
		'imageURL': properties.imageURL,
		'width':  this.width,
		'height': this.height,
		'x': this.position ? this.position.x : 0.0,
		'y': this.position ? this.position.y : 0.0,
		'visible': this.visible
	}
	this.iconOverlay = makeOverlay("image", iconProperties);
	_TRACE.exit()
}
Icon.prototype = new Widget();
Icon.prototype.constructor = Icon;
Icon.prototype.toString = function () {
	return "[UI.Icon " + this.id + " ]";
}

Icon.prototype.relayout = function (layout) {
	var _TRACE = traceEnter.call(this, "Icon.relayout()");
	this._dirty = false;
	this.iconOverlay.update({
		'x': this.position.x,
		'y': this.position.y,
		'width': this.width,
		'height': this.height,
		'visible': this.visible
	});
	_TRACE.exit()
}
Icon.prototype.getHeight = function () {
	return this.height;
}
Icon.prototype.getWidth = function () {
	return this.width;
}
Icon.prototype.hasOverlay = function (overlayId) {
	return this.iconOverlay.getId() === overlayId;
}
Icon.prototype.getOverlay = function () {
	return this.iconOverlay;
}

Icon.prototype.destroy = function () {
	if (this.iconOverlay) {
		this.iconOverlay.destroy();
		this.iconOverlay = null;
	}
}
Icon.prototype.setColor = function (color) {
	if (arguments.length != 1) {
		color = rgba.apply(arguments);
	}
	this.iconOverlay.update({
		'color': color,
		'alpha': color.a
	});
}

var Attachment = UI.Attachment = function (target, impl) {
	Widget.prototype.constructor.apply(this);
	this.rel = { 
		x: target.x - this.position.x, 
		y: target.y - this.position.y 
	};
}
Attachment.prototype.setDirty = function (layout) {
	this._dirty = true;
	this.target.setDirty();
}
Attachment.prototype.relayout = function (layout) {
	targetPos = Vec2.sum(this.position, this.target.position);
}

// UI.updateLayout = function () {
// 	var touched = {};

// 	function recalcDimensions(widget) {
// 		if (widget.parent && !touched[widget.parent.id])
// 			recalcDimensions(widget.parent);
// 		touched[widget.id] = true;
// 		widget.cachedWidth  = widget.calcWidth();
// 		widget.cachedHeight = widget.calcHeight();
// 	}
// 	widgetList.forEach(function (widget) {
// 		if (!touched[widget.id]) {
// 			recalcDimensions(widget);
// 		}
// 	});
// }

UI.setDefaultVisibility = function(visibility) {
	ui.defaultVisible = visibility;
};
UI.updateLayout = function () {
	var _TRACE = traceEnter.call(this, "UI.updateLayout()");
	ui.widgetList.forEach(function(widget, index, array) {
		assert(typeof(widget) == 'object', 'typeof(widget) == "object"');
		if (widget._dirty) {
			var top = widget;
			while (top.parent && top.parent._dirty)
				top = top.parent;
			top.relayout();
			if (widget !== top && widget._dirty)
				widget.relayout();
			assert(!widget._dirty, "widget._dirty == false");
		}
	});
	_TRACE.exit();
};

function dispatchEvent(actions, widget, event) {
	var _TRACE = traceEnter.call(this, "UI.dispatchEvent()");
	actions.forEach(function(action) {
		action.call(widget, event);
	});
	_TRACE.exit();
}

ui.focusedWidget = null;
// ui.selectedWidget = null;

var getWidgetWithOverlay = function (overlay) {
	// print("trying to find overlay: " + overlay);

	var foundWidget = null;
	ui.widgetList.forEach(function(widget) {
		if (widget.hasOverlay(overlay)) {
			// print("found overlay in " + widget);
			foundWidget = widget;
			return;
		}
	});
	// if (!foundWidget)
		// print("could not find overlay");
	return foundWidget;
}

var getFocusedWidget = function (event) {
	return getWidgetWithOverlay(Overlays.getOverlayAtPoint({ 'x': event.x, 'y': event.y }));
}

var dispatchEvent = function (action, event, widget) {
	function dispatchActions (actions) {
		actions.forEach(function(action) {
			action(event, widget);
		});
	}

	if (widget.actions[action]) {
		print("Dispatching action '" + action + "'' to " + widget);
		dispatchActions(widget.actions[action]);
	} else {
		for (var parent = widget.parent; parent != null; parent = parent.parent) {
			if (parent.actions[action]) {
				print("Dispatching action '" + action + "'' to parent widget " + widget);
				dispatchActions(parent.actions[action]);
				return;
			}
		}
		print("No action '" + action + "' in " + widget);
	}
}

UI.handleMouseMove = function (event) {
	// print("mouse moved x = " + event.x + ", y = " + event.y);
	var focused = getFocusedWidget(event);

	print("got focus: " + focused);

	if (focused != ui.focusedWidget) {
		if (focused)
			dispatchEvent('onMouseOver', event, focused);
		if (ui.focusedWidget)
			dispatchEvent('onMouseExit', event, ui.focusedWidget);
		ui.focusedWidget = focused;
	}
}

UI.handleMousePress = function (event) {
	print("Mouse clicked");
	UI.handleMouseMove(event);
	if (ui.focusedWidget) {
		dispatchEvent('onMouseDown', event, ui.focusedWidget);
	}
}

UI.handleMouseRelease = function (event) {
	print("Mouse released");
	UI.handleMouseMove(event);
	if (ui.focusedWidget) {
		dispatchEvent('onMouseUp', event, ui.focusedWidget);
		dispatchEvent('onClick', event, ui.focusedWidget);
	}
}

UI.teardown = function () {
	print("Teardown");
	ui.widgetList.forEach(function(widget) {
		widget.destroy();
	});
	ui.widgetList = [];
	ui.focusedWidget = null;
};
UI.setErrorHandler = function (errorHandler) {
	if (typeof(errorHandler) !== 'function') {
		ui.complain("UI.setErrorHandler -- invalid argument: \"" + errorHandler + "\"");
	} else {
		ui.errorHandler = errorHandler;
	}
}

UI.printWidgets = function () {
	print("widgetlist.length = " + ui.widgetList.length);
	ui.widgetList.forEach(function(widget) {
		print(""+widget + " position=(" + widget.position.x + ", " + widget.position.y + ")" + 
			" parent = " + widget.parent + " visible = " + widget.isVisible() + 
			" width = " + widget.getWidth() + ", height = " + widget.getHeight() +
			" overlay = " + (widget.getOverlay() && widget.getOverlay().getId()) +
			(widget.border ? " border = " + widget.border.x + ", " + widget.border.y : "") + 
			(widget.padding ? " padding = " + widget.padding.x + ", " + widget.padding.y : ""));
	});
}

})();


















































