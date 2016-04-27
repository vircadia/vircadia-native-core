//
//  cookies.js
//
//  version 2.0
//
//  Created by Sam Gateau, 4/1/2015
//  A simple ui panel that present a list of porperties and the proper widget to edit it
//  
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";

var SLIDER_RANGE_INCREMENT_SCALE = 1 / 1000;
var THUMB_COLOR = {
    red: 150,
    green: 150,
    blue: 150
};
var THUMB_HIGHLIGHT = {
    red: 255,
    green: 255,
    blue: 255
};
var CHECK_MARK_COLOR = {
    red: 70,
    green: 70,
    blue: 90
};

// The Slider class
(function() {
    var Slider = function(x, y, width, thumbSize) {

        this.background = Overlays.addOverlay("text", {
            backgroundColor: {
                red: 200,
                green: 200,
                blue: 255
            },
            x: x,
            y: y,
            width: width,
            height: thumbSize,
            alpha: 1.0,
            backgroundAlpha: 0.5,
            visible: true
        });
        this.thumb = Overlays.addOverlay("text", {
            backgroundColor: THUMB_COLOR,
            x: x,
            y: y,
            width: thumbSize,
            height: thumbSize,
            alpha: 1.0,
            backgroundAlpha: 1.0,
            visible: true
        });

        this.thumbSize = thumbSize;
        this.thumbHalfSize = 0.5 * thumbSize;

        this.minThumbX = x + this.thumbHalfSize;
        this.maxThumbX = x + width - this.thumbHalfSize;
        this.thumbX = this.minThumbX;

        this.minValue = 0.0;
        this.maxValue = 1.0;

        this.y = y;

        this.clickOffsetX = 0;
        this.isMoving = false;
        this.visible = true;
    };

    Slider.prototype.updateThumb = function() {
        var thumbTruePos = this.thumbX - 0.5 * this.thumbSize;
        Overlays.editOverlay(this.thumb, {
            x: thumbTruePos
        });
    };

    Slider.prototype.isClickableOverlayItem = function(item) {
        return (item == this.thumb) || (item == this.background);
    };


    Slider.prototype.onMousePressEvent = function(event, clickedOverlay) {
        if (!this.isClickableOverlayItem(clickedOverlay)) {
            this.isMoving = false;
            return;
        }
        this.highlight();
        this.isMoving = true;
        var clickOffset = event.x - this.thumbX;
        if ((clickOffset > -this.thumbHalfSize) && (clickOffset < this.thumbHalfSize)) {
            this.clickOffsetX = clickOffset;
        } else {
            this.clickOffsetX = 0;
            this.thumbX = event.x;
            this.updateThumb();
            this.onValueChanged(this.getValue());
        }
    };

    Slider.prototype.onMouseMoveEvent = function(event) {
        if (this.isMoving) {
            var newThumbX = event.x - this.clickOffsetX;
            if (newThumbX < this.minThumbX) {
                newThumbX = this.minThumbX;
            }
            if (newThumbX > this.maxThumbX) {
                newThumbX = this.maxThumbX;
            }
            this.thumbX = newThumbX;
            this.updateThumb();
            this.onValueChanged(this.getValue());
        }
    };

    Slider.prototype.onMouseReleaseEvent = function(event) {
        this.isMoving = false;
        this.unhighlight();
    };

    Slider.prototype.updateWithKeys = function(direction) {
        this.range = this.maxThumbX - this.minThumbX;
        this.thumbX += direction * (this.range * SCALE);
        this.updateThumb();
        this.onValueChanged(this.getValue());
    };

    Slider.prototype.highlight = function() {
        if (this.highlighted) {
            return;
        }
        Overlays.editOverlay(this.thumb, {
            backgroundColor: {
                red: 255,
                green: 255,
                blue: 255
            }
        });
        this.highlighted = true;
    };

    Slider.prototype.unhighlight = function() {
        if (!this.highlighted) {
            return;
        }
        Overlays.editOverlay(this.thumb, {
            backgroundColor: THUMB_COLOR
        });
        this.highlighted = false;
    };

    Slider.prototype.setNormalizedValue = function(value) {
        if (value < 0.0) {
            this.thumbX = this.minThumbX;
        } else if (value > 1.0) {
            this.thumbX = this.maxThsumbX;
        } else {
            this.thumbX = value * (this.maxThumbX - this.minThumbX) + this.minThumbX;
        }
        this.updateThumb();
    };
    Slider.prototype.getNormalizedValue = function() {
        return (this.thumbX - this.minThumbX) / (this.maxThumbX - this.minThumbX);
    };

    Slider.prototype.setValue = function(value) {
        var normValue = (value - this.minValue) / (this.maxValue - this.minValue);
        this.setNormalizedValue(normValue);
    };
    Slider.prototype.getValue = function() {
        return this.getNormalizedValue() * (this.maxValue - this.minValue) + this.minValue;
    };

    Slider.prototype.reset = function(resetValue) {
        this.setValue(resetValue);
        this.onValueChanged(resetValue);
    };


    Slider.prototype.setMinValue = function(minValue) {
        var currentValue = this.getValue();
        this.minValue = minValue;
        this.setValue(currentValue);
    };
    Slider.prototype.getMinValue = function() {
        return this.minValue;
    };
    Slider.prototype.setMaxValue = function(maxValue) {
        var currentValue = this.getValue();
        this.maxValue = maxValue;
        this.setValue(currentValue);
    };
    Slider.prototype.getMaxValue = function() {
        return this.maxValue;
    };

    Slider.prototype.onValueChanged = function(value) {};

    Slider.prototype.getHeight = function() {
        if (!this.visible) {
            return 0;
        }
        return 1.5 * this.thumbSize;
    };

    Slider.prototype.moveUp = function(newY) {
        Overlays.editOverlay(this.background, {
            y: newY
        });
        Overlays.editOverlay(this.thumb, {
            y: newY
        });
    };

    Slider.prototype.moveDown = function() {
        Overlays.editOverlay(this.background, {
            y: this.y
        });
        Overlays.editOverlay(this.thumb, {
            y: this.y
        });
    };

    this.setThumbColor = function(color) {
        Overlays.editOverlay(this.thumb, {
            backgroundColor: {
                red: color.x * 255,
                green: color.y * 255,
                blue: color.z * 255
            }
        });
    };
    this.setBackgroundColor = function(color) {
        Overlays.editOverlay(this.background, {
            backgroundColor: {
                red: color.x * 255,
                green: color.y * 255,
                blue: color.z * 255
            }
        });
    };

    Slider.prototype.hide = function() {
        Overlays.editOverlay(this.background, {
            visible: false
        });
        Overlays.editOverlay(this.thumb, {
            visible: false
        });
        this.visible = false;
    };

    Slider.prototype.show = function() {
        Overlays.editOverlay(this.background, {
            visible: true
        });
        Overlays.editOverlay(this.thumb, {
            visible: true
        });
        this.visible = true;
    };

    Slider.prototype.destroy = function() {
        Overlays.deleteOverlay(this.background);
        Overlays.deleteOverlay(this.thumb);
    };

    this.Slider = Slider;

    // The Checkbox class
    var Checkbox = function(x, y, width, thumbSize) {

        this.thumb = Overlays.addOverlay("text", {
            backgroundColor: THUMB_COLOR,
            textFontSize: 10,
            x: x,
            y: y,
            width: thumbSize,
            height: thumbSize,
            alpha: 1.0,
            backgroundAlpha: 1.0,
            visible: true
        });


        this.thumbSize = thumbSize;
        var checkX = x + (0.25 * thumbSize);
        var checkY = y + (0.25 * thumbSize);
        this.y = y;
        this.boxCheckStatus = true;
        this.clickedBox = false;
        this.visible = true;


        this.checkMark = Overlays.addOverlay("text", {
            backgroundColor: CHECK_MARK_COLOR,
            x: checkX,
            y: checkY,
            width: thumbSize / 2.0,
            height: thumbSize / 2.0,
            alpha: 1.0,
            visible: true
        });
    };

    Checkbox.prototype.updateThumb = function() {
        Overlays.editOverlay(this.checkMark, {
            visible: this.boxCheckStatus
        });
    };

    Checkbox.prototype.isClickableOverlayItem = function(item) {
        return (item == this.thumb) || (item == this.checkMark);
    };

    Checkbox.prototype.onMousePressEvent = function(event, clickedOverlay) {
        if (!this.isClickableOverlayItem(clickedOverlay)) {
            return;
        }
        this.boxCheckStatus = !this.boxCheckStatus;
        this.onValueChanged(this.getValue());
        this.updateThumb();
    };

    Checkbox.prototype.onMouseReleaseEvent = function(event) {};

    Checkbox.prototype.updateWithKeys = function() {
        this.boxCheckStatus = !this.boxCheckStatus;
        this.onValueChanged(this.getValue());
        this.updateThumb();
    };

    Checkbox.prototype.highlight = function() {
        Overlays.editOverlay(this.thumb, {
            backgroundColor: THUMB_HIGHLIGHT
        });
        this.highlighted = true;
    };

    Checkbox.prototype.unhighlight = function() {
        Overlays.editOverlay(this.thumb, {
            backgroundColor: THUMB_COLOR
        });
        this.highlighted = false;
    };

    Checkbox.prototype.setValue = function(value) {
        this.boxCheckStatus = value;
    };

    Checkbox.prototype.setterFromWidget = function(value) {
        this.updateThumb();
    };

    Checkbox.prototype.getValue = function() {
        return this.boxCheckStatus;
    };

    Checkbox.prototype.reset = function(resetValue) {
        this.setValue(resetValue);
    };

    Checkbox.prototype.onValueChanged = function(value) {};

    Checkbox.prototype.getHeight = function() {
        if (!this.visible) {
            return 0;
        }
        return 1.5 * this.thumbSize;
    };

    Checkbox.prototype.moveUp = function(newY) {
        Overlays.editOverlay(this.background, {
            y: newY
        });
        Overlays.editOverlay(this.thumb, {
            y: newY
        });
        Overlays.editOverlay(this.checkMark, {
            y: newY + (0.25 * this.thumbSize)
        });
        Overlays.editOverlay(this.unCheckMark, {
            y: newY + (0.25 * this.thumbSize)
        });
    };

    Checkbox.prototype.moveDown = function() {
        Overlays.editOverlay(this.background, {
            y: this.y
        });
        Overlays.editOverlay(this.thumb, {
            y: this.y
        });
        Overlays.editOverlay(this.checkMark, {
            y: this.y + (0.25 * this.thumbSize)
        });
        Overlays.editOverlay(this.unCheckMark, {
            y: this.y+ (0.25 * this.thumbSize)
        });
    };

    Checkbox.prototype.hide = function() {
        Overlays.editOverlay(this.background, {
            visible: false
        });
        Overlays.editOverlay(this.thumb, {
            visible: false
        });
        Overlays.editOverlay(this.checkMark, {
            visible: false
        });
        Overlays.editOverlay(this.unCheckMark, {
            visible: false
        });
        this.visible = false;
    }

    Checkbox.prototype.show = function() {
        Overlays.editOverlay(this.background, {
            visible: true
        });
        Overlays.editOverlay(this.thumb, {
            visible: true
        });
        Overlays.editOverlay(this.checkMark, {
            visible: true
        });
        Overlays.editOverlay(this.unCheckMark, {
            visible: true
        });
        this.visible = true;
    }

    Checkbox.prototype.destroy = function() {
        Overlays.deleteOverlay(this.background);
        Overlays.deleteOverlay(this.thumb);
        Overlays.deleteOverlay(this.checkMark);
        Overlays.deleteOverlay(this.unCheckMark);
    };

    this.Checkbox = Checkbox;

    // The ColorBox class
    var ColorBox = function(x, y, width, thumbSize) {
        var self = this;

        var slideHeight = thumbSize / 3;
        var sliderWidth = width;
        this.red = new Slider(x, y, width, slideHeight);
        this.green = new Slider(x, y + slideHeight, width, slideHeight);
        this.blue = new Slider(x, y + 2 * slideHeight, width, slideHeight);
        this.red.setBackgroundColor({
            x: 1,
            y: 0,
            z: 0
        });
        this.green.setBackgroundColor({
            x: 0,
            y: 1,
            z: 0
        });
        this.blue.setBackgroundColor({
            x: 0,
            y: 0,
            z: 1
        });

        this.red.onValueChanged = this.setterFromWidget;
        this.green.onValueChanged = this.setterFromWidget;
        this.blue.onValueChanged = this.setterFromWidget;

        this.visible = true;
    };

    ColorBox.prototype.isClickableOverlayItem = function(item) {
        return this.red.isClickableOverlayItem(item) || this.green.isClickableOverlayItem(item) || this.blue.isClickableOverlayItem(item);
    };

    ColorBox.prototype.onMousePressEvent = function(event, clickedOverlay) {
        this.red.onMousePressEvent(event, clickedOverlay);
        if (this.red.isMoving) {
            return;
        }

        this.green.onMousePressEvent(event, clickedOverlay);
        if (this.green.isMoving) {
            return;
        }

        this.blue.onMousePressEvent(event, clickedOverlay);
    };

    ColorBox.prototype.onMouseMoveEvent = function(event) {
        this.red.onMouseMoveEvent(event);
        this.green.onMouseMoveEvent(event);
        this.blue.onMouseMoveEvent(event);
    };

    ColorBox.prototype.onMouseReleaseEvent = function(event) {
        this.red.onMouseReleaseEvent(event);
        this.green.onMouseReleaseEvent(event);
        this.blue.onMouseReleaseEvent(event);
    };

    ColorBox.prototype.updateWithKeys = function(direction) {
        this.red.updateWithKeys(direction);
        this.green.updateWithKeys(direction);
        this.blue.updateWithKeys(direction);
    }

    ColorBox.prototype.highlight = function() {
        this.red.highlight();
        this.green.highlight();
        this.blue.highlight();

        this.highlighted = true;
    };

    ColorBox.prototype.unhighlight = function() {
        this.red.unhighlight();
        this.green.unhighlight();
        this.blue.unhighlight();

        this.highlighted = false;
    };

    ColorBox.prototype.setterFromWidget = function(value) {
        var color = this.getValue();
        this.onValueChanged(color);
        this.updateRGBSliders(color);
    };

    ColorBox.prototype.onValueChanged = function(value) {};

    ColorBox.prototype.updateRGBSliders = function(color) {
        this.red.setThumbColor({
            x: color.x,
            y: 0,
            z: 0
        });
        this.green.setThumbColor({
            x: 0,
            y: color.y,
            z: 0
        });
        this.blue.setThumbColor({
            x: 0,
            y: 0,
            z: color.z
        });
    };

    // Public members:
    ColorBox.prototype.setValue = function(value) {
        this.red.setValue(value.x);
        this.green.setValue(value.y);
        this.blue.setValue(value.z);
        this.updateRGBSliders(value);
    };

    ColorBox.prototype.getValue = function() {
        var value = {
            x: this.red.getValue(),
            y: this.green.getValue(),
            z: this.blue.getValue()
        };
        return value;
    };

    ColorBox.prototype.reset = function(resetValue) {
        this.setValue(resetValue);
        this.onValueChanged(resetValue);
    };


    ColorBox.prototype.getHeight = function() {
        if (!this.visible) {
            return 0;
        }
        return 1.5 * this.thumbSize;
    };

    ColorBox.prototype.moveUp = function(newY) {
        this.red.moveUp(newY);
        this.green.moveUp(newY);
        this.blue.moveUp(newY);
    };

    ColorBox.prototype.moveDown = function() {
        this.red.moveDown();
        this.green.moveDown();
        this.blue.moveDown();
    };

    ColorBox.prototype.hide = function() {
        this.red.hide();
        this.green.hide();
        this.blue.hide();
        this.visible = false;
    }

    ColorBox.prototype.show = function() {
        this.red.show();
        this.green.show();
        this.blue.show();
        this.visible = true;
    }

    ColorBox.prototype.destroy = function() {
        this.red.destroy();
        this.green.destroy();
        this.blue.destroy();
    };

    this.ColorBox = ColorBox;

    // The DirectionBox class
    var DirectionBox = function(x, y, width, thumbSize) {
        var self = this;

        var slideHeight = thumbSize / 2;
        var sliderWidth = width;
        this.yaw = new Slider(x, y, width, slideHeight);
        this.pitch = new Slider(x, y + slideHeight, width, slideHeight);


        this.yaw.setThumbColor({
            x: 1,
            y: 0,
            z: 0
        });
        this.yaw.minValue = -180;
        this.yaw.maxValue = +180;

        this.pitch.setThumbColor({
            x: 0,
            y: 0,
            z: 1
        });
        this.pitch.minValue = -1;
        this.pitch.maxValue = +1;

        this.yaw.onValueChanged = this.setterFromWidget;
        this.pitch.onValueChanged = this.setterFromWidget;

        this.visible = true;
    };

    DirectionBox.prototype.isClickableOverlayItem = function(item) {
        return this.yaw.isClickableOverlayItem(item) || this.pitch.isClickableOverlayItem(item);
    };

    DirectionBox.prototype.onMousePressEvent = function(event, clickedOverlay) {
        this.yaw.onMousePressEvent(event, clickedOverlay);
        if (this.yaw.isMoving) {
            return;
        }
        this.pitch.onMousePressEvent(event, clickedOverlay);
    };

    DirectionBox.prototype.onMouseMoveEvent = function(event) {
        this.yaw.onMouseMoveEvent(event);
        this.pitch.onMouseMoveEvent(event);
    };

    DirectionBox.prototype.onMouseReleaseEvent = function(event) {
        this.yaw.onMouseReleaseEvent(event);
        this.pitch.onMouseReleaseEvent(event);
    };

    DirectionBox.prototype.updateWithKeys = function(direction) {
        this.yaw.updateWithKeys(direction);
        this.pitch.updateWithKeys(direction);
    };

    DirectionBox.prototype.highlight = function() {
        this.pitch.highlight();
        this.yaw.highlight();

        this.highlighted = true;
    };

    DirectionBox.prototype.unhighlight = function() {
        this.pitch.unhighlight();
        this.yaw.unhighlight();

        this.highlighted = false;
    };

    DirectionBox.prototype.setterFromWidget = function(value) {
        var yawPitch = this.getValue();
        this.onValueChanged(yawPitch);
    };

    DirectionBox.prototype.onValueChanged = function(value) {};

    DirectionBox.prototype.setValue = function(direction) {
        var flatXZ = Math.sqrt(direction.x * direction.x + direction.z * direction.z);
        if (flatXZ > 0.0) {
            var flatX = direction.x / flatXZ;
            var flatZ = direction.z / flatXZ;
            var yaw = Math.acos(flatX) * 180 / Math.PI;
            if (flatZ < 0) {
                yaw = -yaw;
            }
            this.yaw.setValue(yaw);
        }
        this.pitch.setValue(direction.y);
    };

    DirectionBox.prototype.getValue = function() {
        var dirZ = this.pitch.getValue();
        var yaw = this.yaw.getValue() * Math.PI / 180;
        var cosY = Math.sqrt(1 - dirZ * dirZ);
        var value = {
            x: cosY * Math.cos(yaw),
            y: dirZ,
            z: cosY * Math.sin(yaw)
        };
        return value;
    };

    DirectionBox.prototype.reset = function(resetValue) {
        this.setValue(resetValue);
        this.onValueChanged(resetValue);
    };

    DirectionBox.prototype.getHeight = function() {
        if (!this.visible) {
            return 0;
        }
        return 1.5 * this.thumbSize;
    };

    DirectionBox.prototype.moveUp = function(newY) {
        this.pitch.moveUp(newY);
        this.yaw.moveUp(newY);
    };

    DirectionBox.prototype.moveDown = function(newY) {
        this.pitch.moveDown();
        this.yaw.moveDown();
    };

    DirectionBox.prototype.hide = function() {
        this.pitch.hide();
        this.yaw.hide();
        this.visible = false;
    }

    DirectionBox.prototype.show = function() {
        this.pitch.show();
        this.yaw.show();
        this.visible = true;
    }

    DirectionBox.prototype.destroy = function() {
        this.yaw.destroy();
        this.pitch.destroy();
    };

    this.DirectionBox = DirectionBox;

    var textFontSize = 12;

    var CollapsablePanelItem = function(name, x, y, textWidth, height) {
        this.name = name;
        this.height = height;
        this.y = y;
        this.isCollapsable = true;

        var topMargin = (height - 1.5 * textFontSize);

        this.thumb = Overlays.addOverlay("image", {
            color: {
                red: 255,
                green: 255,
                blue: 255
            },
            imageURL: HIFI_PUBLIC_BUCKET + 'images/tools/expand-ui.svg',
            x: x,
            y: y,
            width: rawHeight,
            height: rawHeight,
            alpha: 1.0,
            visible: true
        });

        this.title = Overlays.addOverlay("text", {
            backgroundColor: {
                red: 255,
                green: 255,
                blue: 255
            },
            x: x + rawHeight * 1.5,
            y: y,
            width: textWidth,
            height: height,
            alpha: 1.0,
            backgroundAlpha: 0.5,
            visible: true,
            text: " " + name,
            font: {
                size: textFontSize
            },
            topMargin: topMargin
        });
    };

    CollapsablePanelItem.prototype.destroy = function() {
        Overlays.deleteOverlay(this.title);
        Overlays.deleteOverlay(this.thumb);
    };

    CollapsablePanelItem.prototype.editTitle = function(opts) {
        Overlays.editOverlay(this.title, opts);
    };

    CollapsablePanelItem.prototype.hide = function() {
        Overlays.editOverlay(this.title, {
            visible: false
        });
        Overlays.editOverlay(this.thumb, {
            visible: false
        });

        if (this.widget != null) {
            this.widget.hide();
        }
    };

    CollapsablePanelItem.prototype.show = function() {
        Overlays.editOverlay(this.title, {
            visible: true
        });
        Overlays.editOverlay(this.thumb, {
            visible: true
        });

        if (this.widget != null) {
            this.widget.show();
        }
    };

    CollapsablePanelItem.prototype.moveUp = function(newY) {
        Overlays.editOverlay(this.title, {
            y: newY
        });
        Overlays.editOverlay(this.thumb, {
            y: newY
        });

        if (this.widget != null) {
            this.widget.moveUp(newY);
        }
    }

    CollapsablePanelItem.prototype.moveDown = function() {
        Overlays.editOverlay(this.title, {
            y: this.y
        });
        Overlays.editOverlay(this.thumb, {
            y: this.y
        });

        if (this.widget != null) {
            this.widget.moveDown();
        }
    }
    this.CollapsablePanelItem = CollapsablePanelItem;

    var PanelItem = function(name, setter, getter, displayer, x, y, textWidth, valueWidth, height) {
        //print("creating panel item: " + name);

        this.isCollapsable = false;
        this.name = name;
        this.y = y;
        this.isCollapsed = false;

        this.displayer = typeof displayer !== 'undefined' ? displayer : function(value) {
            if (value == true) {
                return "On";
            } else if (value == false) {
                return "Off";
            }
            return value.toFixed(2);
        };

        var topMargin = (height - 1.5 * textFontSize);
        this.title = Overlays.addOverlay("text", {
            backgroundColor: {
                red: 255,
                green: 255,
                blue: 255
            },
            x: x,
            y: y,
            width: textWidth,
            height: height,
            alpha: 1.0,
            backgroundAlpha: 0.5,
            visible: true,
            text: " " + name,
            font: {
                size: textFontSize
            },
            topMargin: topMargin
        });

        this.value = Overlays.addOverlay("text", {
            backgroundColor: {
                red: 255,
                green: 255,
                blue: 255
            },
            x: x + textWidth,
            y: y,
            width: valueWidth,
            height: height,
            alpha: 1.0,
            backgroundAlpha: 0.5,
            visible: true,
            text: this.displayer(getter()),
            font: {
                size: textFontSize
            },
            topMargin: topMargin
        });

        this.getter = getter;
        this.resetValue = getter();

        this.setter = function(value) {

            setter(value);

            Overlays.editOverlay(this.value, {
                text: this.displayer(getter())
            });

            if (this.widget) {
                this.widget.setValue(value);
            }

            //print("successfully set value of widget to " + value);
        };
        this.setterFromWidget = function(value) {
            setter(value);
            // ANd loop back the value after the final setter has been called
            var value = getter();

            if (this.widget) {
                this.widget.setValue(value);
            }
            Overlays.editOverlay(this.value, {
                text: this.displayer(value)
            });
        };

        this.widget = null;
    };

    PanelItem.prototype.hide = function() {
        Overlays.editOverlay(this.title, {
            visible: false
        });
        Overlays.editOverlay(this.value, {
            visible: false
        });

        if (this.widget != null) {
            this.widget.hide();
        }
    };


    PanelItem.prototype.show = function() {
        Overlays.editOverlay(this.title, {
            visible: true
        });
        Overlays.editOverlay(this.value, {
            visible: true
        });

        if (this.widget != null) {
            this.widget.show();
        }

    };

    PanelItem.prototype.moveUp = function(newY) {

        Overlays.editOverlay(this.title, {
            y: newY
        });
        Overlays.editOverlay(this.value, {
            y: newY
        });

        if (this.widget != null) {
            this.widget.moveUp(newY);
        }

    };

    PanelItem.prototype.moveDown = function() {

        Overlays.editOverlay(this.title, {
            y: this.y
        });
        Overlays.editOverlay(this.value, {
            y: this.y
        });

        if (this.widget != null) {
            this.widget.moveDown();
        }

    };

    PanelItem.prototype.destroy = function() {
        Overlays.deleteOverlay(this.title);
        Overlays.deleteOverlay(this.value);

        if (this.widget != null) {
            this.widget.destroy();
        }
    };
    this.PanelItem = PanelItem;

    var textWidth = 180;
    var valueWidth = 100;
    var widgetWidth = 300;
    var rawHeight = 20;
    var rawYDelta = rawHeight * 1.5;
    var outerPanel = true;
    var widgets;


    var Panel = function(x, y) {

        if (outerPanel) {
            widgets = [];
        }
        outerPanel = false;

        this.x = x;
        this.y = y;
        this.nextY = y;

        print("creating panel at x: " + this.x + " y: " + this.y);

        this.widgetX = x + textWidth + valueWidth;

        this.items = new Array();
        this.activeWidget = null;
        this.visible = true;
        this.indentation = 30;

    };

    Panel.prototype.mouseMoveEvent = function(event) {
        if (this.activeWidget) {
            this.activeWidget.onMouseMoveEvent(event);
        }
    };

    Panel.prototype.mousePressEvent = function(event) {
        // Make sure we quitted previous widget
        if (this.activeWidget) {
            this.activeWidget.onMouseReleaseEvent(event);
        }
        this.activeWidget = null;

        var clickedOverlay = Overlays.getOverlayAtPoint({
            x: event.x,
            y: event.y
        });

        this.handleCollapse(clickedOverlay);

        // If the user clicked any of the slider background then...
        for (var i in this.items) {
            var item = this.items[i];
            var widget = this.items[i].widget;

            if (widget.isClickableOverlayItem(clickedOverlay)) {
                this.activeWidget = widget;
                this.activeWidget.onMousePressEvent(event, clickedOverlay);
                break;

            }
        }
    };

    Panel.prototype.mouseReleaseEvent = function(event) {
        if (this.activeWidget) {
            this.activeWidget.onMouseReleaseEvent(event);
        }
        this.activeWidget = null;
    };

    // Reset panel item upon double-clicking
    Panel.prototype.mouseDoublePressEvent = function(event) {
        var clickedOverlay = Overlays.getOverlayAtPoint({
            x: event.x,
            y: event.y
        });
        this.handleReset(clickedOverlay);
    };


    Panel.prototype.handleReset = function(clickedOverlay) {
        for (var i in this.items) {

            var item = this.items[i];
            var widget = item.widget;

            if (item.isSubPanel && widget) {
                widget.handleReset(clickedOverlay);
            }

            if (clickedOverlay == item.title) {
                item.activeWidget = widget;
                item.activeWidget.reset(item.resetValue);
                break;
            }
        }
    };

    Panel.prototype.handleCollapse = function(clickedOverlay) {
        for (var i in this.items) {

            var item = this.items[i];
            var widget = item.widget;

            if (item.isSubPanel && widget) {
                widget.handleCollapse(clickedOverlay);
            }

            if (!item.isCollapsed && item.isCollapsable && clickedOverlay == item.thumb) {
                Overlays.editOverlay(item.thumb, {
                    imageURL: HIFI_PUBLIC_BUCKET + 'images/tools/expand-right.svg'
                });
                this.collapse(clickedOverlay);
                item.isCollapsed = true;
            } else if (item.isCollapsed && item.isCollapsable && clickedOverlay == item.thumb) {
                Overlays.editOverlay(item.thumb, {
                    imageURL: HIFI_PUBLIC_BUCKET + 'images/tools/expand-ui.svg'
                });
                this.expand(clickedOverlay);
                item.isCollapsed = false;
            }
        }
    };

    Panel.prototype.collapse = function(clickedOverlay) {
        var keys = Object.keys(this.items);

        for (var i = 0; i < keys.length; ++i) {
            var item = this.items[keys[i]];

            if (item.isCollapsable && clickedOverlay == item.thumb) {
                var panel = item.widget;
                panel.hide();
                break;
            }
        }

        // Now recalculate new heights of subsequent widgets
        for (var j = i + 1; j < keys.length; ++j) {
            this.items[keys[j]].moveUp(this.getCurrentY(keys[j]));
        }
    };

    Panel.prototype.expand = function(clickedOverlay) {
        var keys = Object.keys(this.items);

        for (var i = 0; i < keys.length; ++i) {
            var item = this.items[keys[i]];

            if (item.isCollapsable && clickedOverlay == item.thumb) {
                var panel = item.widget;
                panel.show();
                break;
            }
        }
        // Now recalculate new heights of subsequent widgets
        for (var j = i + 1; j < keys.length; ++j) {
            this.items[keys[j]].moveDown();
        }
    };


    Panel.prototype.onMousePressEvent = function(event, clickedOverlay) {
        for (var i in this.items) {
            var item = this.items[i];
            if (item.widget.isClickableOverlayItem(clickedOverlay)) {
                item.activeWidget = item.widget;
                item.activeWidget.onMousePressEvent(event, clickedOverlay);
            }
        }
    };

    Panel.prototype.onMouseMoveEvent = function(event) {
        for (var i in this.items) {
            var item = this.items[i];
            if (item.activeWidget) {
                item.activeWidget.onMouseMoveEvent(event);
            }
        }
    };

    Panel.prototype.onMouseReleaseEvent = function(event, clickedOverlay) {
        for (var i in this.items) {
            var item = this.items[i];
            if (item.activeWidget) {
                item.activeWidget.onMouseReleaseEvent(event);
            }
            item.activeWidget = null;
        }
    };

    Panel.prototype.onMouseDoublePressEvent = function(event, clickedOverlay) {
        for (var i in this.items) {
            var item = this.items[i];
            if (item.activeWidget) {
                item.activeWidget.onMouseDoublePressEvent(event);
            }
        }
    };

    var tabView = false;
    var tabIndex = 0;

    Panel.prototype.keyPressEvent = function(event) {
        if (event.text == "TAB" && !event.isShifted) {
            tabView = true;
            if (tabIndex < widgets.length) {
                if (tabIndex > 0 && widgets[tabIndex - 1].highlighted) {
                    // Unhighlight previous widget
                    widgets[tabIndex - 1].unhighlight();
                }
                widgets[tabIndex].highlight();
                tabIndex++;
            } else {
                widgets[tabIndex - 1].unhighlight();
                //Wrap around to front
                tabIndex = 0;
                widgets[tabIndex].highlight();
                tabIndex++;
            }
        } else if (tabView && event.isShifted) {
            if (tabIndex > 0) {
                tabIndex--;
                if (tabIndex < widgets.length && widgets[tabIndex + 1].highlighted) {
                    // Unhighlight previous widget
                    widgets[tabIndex + 1].unhighlight();
                }
                widgets[tabIndex].highlight();
            } else {
                widgets[tabIndex].unhighlight();
                //Wrap around to end
                tabIndex = widgets.length - 1;
                widgets[tabIndex].highlight();
            }
        } else if (event.text == "LEFT") {
            for (var i = 0; i < widgets.length; i++) {
                // Find the highlighted widget, allow control with arrow keys
                if (widgets[i].highlighted) {
                    var k = -1;
                    widgets[i].updateWithKeys(k);
                    break;
                }
            }
        } else if (event.text == "RIGHT") {
            for (var i = 0; i < widgets.length; i++) {
                // Find the highlighted widget, allow control with arrow keys
                if (widgets[i].highlighted) {
                    var k = 1;
                    widgets[i].updateWithKeys(k);
                    break;
                }
            }
        }
    };

    // Widget constructors
    Panel.prototype.newSlider = function(name, minValue, maxValue, setValue, getValue, displayValue) {
        this.nextY = this.y + this.getHeight();

        var item = new PanelItem(name, setValue, getValue, displayValue, this.x, this.nextY, textWidth, valueWidth, rawHeight);
        var slider = new Slider(this.widgetX, this.nextY, widgetWidth, rawHeight);
        slider.minValue = minValue;
        slider.maxValue = maxValue;
        widgets.push(slider);

        item.widget = slider;
        item.widget.onValueChanged = function(value) {
            item.setterFromWidget(value);
        };
        item.setter(getValue());
        this.items[name] = item;

    };

    Panel.prototype.newCheckbox = function(name, setValue, getValue, displayValue) {
        var display;
        if (displayValue == true) {
            display = function() {
                return "On";
            };
        } else if (displayValue == false) {
            display = function() {
                return "Off";
            };
        }

        this.nextY = this.y + this.getHeight();

        var item = new PanelItem(name, setValue, getValue, display, this.x, this.nextY, textWidth, valueWidth, rawHeight);

        var checkbox = new Checkbox(this.widgetX, this.nextY, widgetWidth, rawHeight);
        widgets.push(checkbox);

        item.widget = checkbox;
        item.widget.onValueChanged = function(value) {
            item.setterFromWidget(value);
        };
        item.setter(getValue());
        this.items[name] = item;

        //print("created Item... checkbox=" + name);     
    };

    Panel.prototype.newColorBox = function(name, setValue, getValue, displayValue) {
        this.nextY = this.y + this.getHeight();

        var item = new PanelItem(name, setValue, getValue, displayValue, this.x, this.nextY, textWidth, valueWidth, rawHeight);

        var colorBox = new ColorBox(this.widgetX, this.nextY, widgetWidth, rawHeight);
        widgets.push(colorBox);

        item.widget = colorBox;
        item.widget.onValueChanged = function(value) {
            item.setterFromWidget(value);
        };
        item.setter(getValue());
        this.items[name] = item;

        //  print("created Item... colorBox=" + name);     
    };

    Panel.prototype.newDirectionBox = function(name, setValue, getValue, displayValue) {
        this.nextY = this.y + this.getHeight();

        var item = new PanelItem(name, setValue, getValue, displayValue, this.x, this.nextY, textWidth, valueWidth, rawHeight);

        var directionBox = new DirectionBox(this.widgetX, this.nextY, widgetWidth, rawHeight);
        widgets.push(directionBox);

        item.widget = directionBox;
        item.widget.onValueChanged = function(value) {
            item.setterFromWidget(value);
        };
        item.setter(getValue());
        this.items[name] = item;

        //  print("created Item... directionBox=" + name);     
    };

    Panel.prototype.newSubPanel = function(name) {

        this.nextY = this.y + this.getHeight();

        var item = new CollapsablePanelItem(name, this.x, this.nextY, textWidth, rawHeight);
        item.isSubPanel = true;

        this.nextY += 1.5 * item.height;

        var subPanel = new Panel(this.x + this.indentation, this.nextY);

        item.widget = subPanel;
        this.items[name] = item;
        return subPanel;
        //    print("created Item... subPanel=" + name);
    };

    Panel.prototype.onValueChanged = function(value) {
        for (var i in this.items) {
            this.items[i].widget.onValueChanged(value);
        }
    };


    Panel.prototype.set = function(name, value) {
        var item = this.items[name];
        if (item != null) {
            return item.setter(value);
        }
        return null;
    };

    Panel.prototype.get = function(name) {
        var item = this.items[name];
        if (item != null) {
            return item.getter();
        }
        return null;
    };

     Panel.prototype.getWidget = function(name) {
        var item = this.items[name];
        if (item != null) {
            return item.widget;
        }
        return null;
    };   

    Panel.prototype.update = function(name) {
        var item = this.items[name];
        if (item != null) {
            return item.setter(item.getter());
        }
        return null;
    };

    Panel.prototype.isClickableOverlayItem = function(item) {
        for (var i in this.items) {
            if (this.items[i].widget.isClickableOverlayItem(item)) {
                return true;
            }
        }
        return false;
    };

    Panel.prototype.getHeight = function() {
        var height = 0;

        for (var i in this.items) {
            height += this.items[i].widget.getHeight();
            if (this.items[i].isSubPanel && this.items[i].widget.visible) {
                height += 1.5 * rawHeight;
            }
        }

        return height;
    };

    Panel.prototype.moveUp = function() {
        for (var i in this.items) {
            this.items[i].widget.moveUp();
        }
    };

    Panel.prototype.moveDown = function() {
        for (var i in this.items) {
            this.items[i].widget.moveDown();
        }
    };

    Panel.prototype.getCurrentY = function(key) {
        var height = 0;
        var keys = Object.keys(this.items);

        for (var i = 0; i < keys.indexOf(key); ++i) {
            var item = this.items[keys[i]];

            height += item.widget.getHeight();

            if (item.isSubPanel) {
                height += 1.5 * rawHeight;

            }
        }
        return this.y + height;
    };


    Panel.prototype.hide = function() {
        for (var i in this.items) {
            if (this.items[i].isSubPanel) {
                this.items[i].widget.hide();
            }
            this.items[i].hide();
        }
        this.visible = false;
    };

    Panel.prototype.show = function() {
        for (var i in this.items) {
            if (this.items[i].isSubPanel) {
                this.items[i].widget.show();
            }
            this.items[i].show();
        }
        this.visible = true;
    };


    Panel.prototype.destroy = function() {
        for (var i in this.items) {

            if (this.items[i].isSubPanel) {
                this.items[i].widget.destroy();
            }
            this.items[i].destroy();
        }
    };
    this.Panel = Panel;
})();


Script.scriptEnding.connect(function scriptEnding() {
    Controller.releaseKeyEvents({
        text: "left"
    });
    Controller.releaseKeyEvents({
        key: "right"
    });
});


Controller.captureKeyEvents({
    text: "left"
});
Controller.captureKeyEvents({
    text: "right"
});
