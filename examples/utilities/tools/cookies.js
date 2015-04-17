//
//  cookies.js
//
//  version 1.0
//
//  Created by Sam Gateau, 4/1/2015
//  A simple ui panel that present a list of porperties and the proper widget to edit it
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// The Slider class
Slider = function(x,y,width,thumbSize) {

    this.thumb = Overlays.addOverlay("text", {
                    backgroundColor: { red: 255, green: 255, blue: 255 },
                    x: x,
                    y: y,
                    width: thumbSize,
                    height: thumbSize,
                    alpha: 1.0,
                    backgroundAlpha: 1.0,
                    visible: true
                });
    this.background = Overlays.addOverlay("text", {
                    backgroundColor: { red: 125, green: 125, blue: 255 },
                    x: x,
                    y: y,
                    width: width,
                    height: thumbSize,
                    alpha: 1.0,
                    backgroundAlpha: 0.5,
                    visible: true
                });

    this.thumbSize = thumbSize;
    this.thumbHalfSize = 0.5 * thumbSize;

    this.minThumbX = x + this.thumbHalfSize;
    this.maxThumbX = x + width - this.thumbHalfSize;
    this.thumbX = this.minThumbX;

    this.minValue = 0.0;
    this.maxValue = 1.0;

    this.clickOffsetX = 0;
    this.isMoving = false;

    this.updateThumb = function() { 
        thumbTruePos = this.thumbX - 0.5 * this.thumbSize;
        Overlays.editOverlay(this.thumb, { x: thumbTruePos } );
    };

    this.onMouseMoveEvent = function(event) { 
        if (this.isMoving) {
            newThumbX = event.x - this.clickOffsetX;
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

    this.onMousePressEvent = function(event) {
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

    this.onMouseReleaseEvent = function(event) {
        this.isMoving = false;
    };

    // Public members:

    this.setNormalizedValue = function(value) {
        if (value < 0.0) {
            this.thumbX = this.minThumbX;
        } else if (value > 1.0) {
            this.thumbX = this.maxThumbX;
        } else {
            this.thumbX = value * (this.maxThumbX - this.minThumbX) + this.minThumbX;
        }
        this.updateThumb();
    };
    this.getNormalizedValue = function() {
        return (this.thumbX - this.minThumbX) / (this.maxThumbX - this.minThumbX);
    };

    this.setValue = function(value) {
        var normValue = (value - this.minValue) / (this.maxValue - this.minValue);
        this.setNormalizedValue(normValue);
    };

    this.getValue = function() {
        return this.getNormalizedValue() * (this.maxValue - this.minValue) + this.minValue;
    };

    this.onValueChanged = function(value) {};

    this.destroy = function() {
        Overlays.deleteOverlay(this.background);
        Overlays.deleteOverlay(this.thumb);
    };
}

// The Checkbox class
Checkbox = function(x,y,width,thumbSize) {

    this.thumb = Overlays.addOverlay("text", {
                    backgroundColor: { red: 255, green: 255, blue: 255 },
                    x: x,
                    y: y,
                    width: thumbSize,
                    height: thumbSize,
                    alpha: 1.0,
                    backgroundAlpha: 1.0,
                    visible: true
                });
    this.background = Overlays.addOverlay("text", {
                    backgroundColor: { red: 125, green: 125, blue: 255 },
                    x: x,
                    y: y,
                    width: width,
                    height: thumbSize,
                    alpha: 1.0,
                    backgroundAlpha: 0.5,
                    visible: true
                });

    this.thumbSize = thumbSize;
    this.thumbHalfSize = 0.5 * thumbSize;

    this.minThumbX = x + this.thumbHalfSize;
    this.maxThumbX = x + width - this.thumbHalfSize;
    this.thumbX = this.minThumbX;

    this.minValue = 0.0;
    this.maxValue = 1.0;

    this.clickOffsetX = 0;
    this.isMoving = false;

    this.updateThumb = function() { 
        thumbTruePos = this.thumbX - 0.5 * this.thumbSize;
        Overlays.editOverlay(this.thumb, { x: thumbTruePos } );
    };

    this.onMouseMoveEvent = function(event) { 
        if (this.isMoving) {
            newThumbX = event.x - this.clickOffsetX;
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

    this.onMousePressEvent = function(event) {
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

    this.onMouseReleaseEvent = function(event) {
        this.isMoving = false;
    };

    // Public members:

    this.setNormalizedValue = function(value) {
        if (value < 0.0) {
            this.thumbX = this.minThumbX;
        } else if (value > 1.0) {
            this.thumbX = this.maxThumbX;
        } else {
            this.thumbX = value * (this.maxThumbX - this.minThumbX) + this.minThumbX;
        }
        this.updateThumb();
    };
    this.getNormalizedValue = function() {
        return (this.thumbX - this.minThumbX) / (this.maxThumbX - this.minThumbX);
    };

    this.setValue = function(value) {
        var normValue = (value - this.minValue) / (this.maxValue - this.minValue);
        this.setNormalizedValue(normValue);
    };

    this.getValue = function() {
        return this.getNormalizedValue() * (this.maxValue - this.minValue) + this.minValue;
    };

    this.onValueChanged = function(value) {};

    this.destroy = function() {
        Overlays.deleteOverlay(this.background);
        Overlays.deleteOverlay(this.thumb);
    };
}

var textFontSize = 16;

function PanelItem(name, setter, getter, displayer, x, y, textWidth, valueWidth, height) {
    this.name = name;


    this.displayer = typeof displayer !== 'undefined' ? displayer : function(value) { return value.toFixed(2); };

    var topMargin = (height - textFontSize);
    this.title = Overlays.addOverlay("text", {
                    backgroundColor: { red: 255, green: 255, blue: 255 },
                    x: x,
                    y: y,
                    width: textWidth,
                    height: height,
                    alpha: 1.0,
                    backgroundAlpha: 0.5,
                    visible: true,
                    text: name,
                    font: {size: textFontSize},
                    topMargin: topMargin, 
                });

    this.value = Overlays.addOverlay("text", {
                    backgroundColor: { red: 255, green: 255, blue: 255 },
                    x: x + textWidth,
                    y: y,
                    width: valueWidth,
                    height: height,
                    alpha: 1.0,
                    backgroundAlpha: 0.5,
                    visible: true,
                    text: this.displayer(getter()),
                    font: {size: textFontSize},
                    topMargin: topMargin

                });
    this.getter = getter;

    this.setter = function(value) {
        setter(value);
        Overlays.editOverlay(this.value, {text: this.displayer(getter())});
        if (this.widget) {
            this.widget.setValue(value);
        }    
    };
    this.setterFromWidget = function(value) {
        setter(value);
        Overlays.editOverlay(this.value, {text: this.displayer(getter())});    
    };

    
    this.widget = null;

    this.destroy = function() {
        Overlays.deleteOverlay(this.title);
        Overlays.deleteOverlay(this.value);
        if (this.widget != null) {
            this.widget.destroy();
        }
    }
}

var textWidth = 180;
var valueWidth = 100;
var widgetWidth = 300;
var rawHeight = 20;
var rawYDelta = rawHeight * 1.5;

Panel = function(x, y) {

    this.x = x;
    this.y = y;
    this.nextY = y;

    this.widgetX = x + textWidth + valueWidth; 

    this.items = new Array();
    this.activeWidget = null;

    this.mouseMoveEvent = function(event) {
        if (this.activeWidget) {
            this.activeWidget.onMouseMoveEvent(event);
        }
    };

    // we also handle click detection in our mousePressEvent()
    this.mousePressEvent = function(event) {
        // Make sure we quitted previous widget
        if (this.activeWidget) {
            this.activeWidget.onMouseReleaseEvent(event);
        }
        this.activeWidget = null; 

        var clickedOverlay = Overlays.getOverlayAtPoint({x: event.x, y: event.y});

        // If the user clicked any of the slider background then...
        for (var i in this.items) {
            var widget = this.items[i].widget;

            if (clickedOverlay == widget.background) {
                this.activeWidget = widget;
                this.activeWidget.onMousePressEvent(event);        
              //  print("clicked... widget=" + i);        
                break;
            }    
        }  
    };

    this.mouseReleaseEvent = function(event) {
        if (this.activeWidget) {
            this.activeWidget.onMouseReleaseEvent(event);
        }
        this.activeWidget = null;
    };

    this.newSlider = function(name, minValue, maxValue, setValue, getValue, displayValue) {

        var sliderItem = new PanelItem(name, setValue, getValue, displayValue, this.x, this.nextY, textWidth, valueWidth, rawHeight);

        var slider = new Slider(this.widgetX, this.nextY, widgetWidth, rawHeight);
        slider.minValue = minValue;
        slider.maxValue = maxValue;
        slider.onValueChanged = function(value) { sliderItem.setterFromWidget(value); };


        sliderItem.widget = slider;
        sliderItem.setter(getValue());        
        this.items[name] = sliderItem;
        this.nextY += rawYDelta;
      //  print("created Item... slider=" + name);     
    };

    this.newCheckbox = function(name, setValue, getValue, displayValue) {

        var checkboxItem = new PanelItem(name, setValue, getValue, displayValue, this.x, this.nextY, textWidth, valueWidth, rawHeight);

        var checkbox = new Checkbox(this.widgetX, this.nextY, widgetWidth, rawHeight);
        checkbox.onValueChanged = function(value) { checkboxItem.setterFromWidget(value); };


        checkboxItem.widget = checkbox;
        checkboxItem.setter(getValue());        
        this.items[name] = checkboxItem;
        this.nextY += rawYDelta;
      //  print("created Item... slider=" + name);     
    };

    this.destroy = function() {
        for (var i in this.items) {
            this.items[i].destroy();  
        } 
    }

    this.set = function(name, value) {
        var item = this.items[name];
        if (item != null) {
            return item.setter(value);
        }
        return null;
    }

    this.get = function(name) {
        var item = this.items[name];
        if (item != null) {
            return item.getter();
        }
        return null;
    }
};


