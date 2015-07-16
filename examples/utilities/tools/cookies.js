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
// The Slider class
Slider = function(x,y,width,thumbSize) {
    this.background = Overlays.addOverlay("text", {
                    backgroundColor: { red: 200, green: 200, blue: 255 },
                    x: x,
                    y: y,
                    width: width,
                    height: thumbSize,
                    alpha: 1.0,
                    backgroundAlpha: 0.5,
                    visible: true
                });
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

    this.isClickableOverlayItem = function(item) {
        return (item == this.thumb) || (item == this.background);
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

    this.onMousePressEvent = function(event, clickedOverlay) {
        if (!this.isClickableOverlayItem(clickedOverlay)) {
            this.isMoving = false;
            return;
        }
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

    this.setThumbColor = function(color) {
        Overlays.editOverlay(this.thumb, {backgroundColor: { red: color.x*255, green: color.y*255, blue: color.z*255 }});
    };
    this.setBackgroundColor = function(color) {
        Overlays.editOverlay(this.background, {backgroundColor: { red: color.x*255, green: color.y*255, blue: color.z*255 }});
    };
}

// The Checkbox class
Checkbox = function(x,y,width,thumbSize) {

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

    this.thumb = Overlays.addOverlay("text", {
                    backgroundColor: { red: 255, green: 255, blue: 255 },
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

    
    var checkMark = Overlays.addOverlay("text", {
                    backgroundColor: { red: 0, green: 255, blue: 0 },
                    x: checkX,
                    y: checkY,
                    width: thumbSize / 2.0,
                    height: thumbSize / 2.0,
                    alpha: 1.0,
                    visible: true
                });

    var unCheckMark = Overlays.addOverlay("image", {
                    backgroundColor: { red: 255, green: 255, blue: 255 },
                    x: checkX + 1.0,
                    y: checkY + 1.0,
                    width: thumbSize / 2.5,
                    height: thumbSize / 2.5,
                    alpha: 1.0,
                    visible: boxCheckStatus
                });

    
    var boxCheckStatus;
    var clickedBox = false;

    this.updateThumb = function() {   
        if (clickedBox) {
            boxCheckStatus = !boxCheckStatus;
            if (boxCheckStatus) {
                Overlays.editOverlay(unCheckMark, { visible: false });
            } else {
                Overlays.editOverlay(unCheckMark, { visible: true });
            }
        }
    };

    this.isClickableOverlayItem = function(item) {
        return (item == this.thumb) || (item == checkMark) || (item == unCheckMark);
    };
    
    this.onMousePressEvent = function(event, clickedOverlay) {
        if (!this.isClickableOverlayItem(clickedOverlay)) {
            this.isMoving = false;
            clickedBox = false;
            return;
        }
        
        clickedBox = true;
        this.updateThumb();
        this.onValueChanged(this.getValue());
    };

    this.onMouseReleaseEvent = function(event) {
        this.clickedBox = false;
    };

    // Public members:
    this.setNormalizedValue = function(value) {
        boxCheckStatus = value;
    };

    this.getNormalizedValue = function() {
        return boxCheckStatus; 
    };

    this.setValue = function(value) {
        this.setNormalizedValue(value);
    };

    this.getValue = function() {
        return boxCheckStatus;
    };

    this.onValueChanged = function(value) {};

    this.destroy = function() {
        Overlays.deleteOverlay(this.background);
        Overlays.deleteOverlay(this.thumb);
        Overlays.deleteOverlay(checkMark);
        Overlays.deleteOverlay(unCheckMark);
    };

    this.setThumbColor = function(color) {
        Overlays.editOverlay(this.thumb, { red: 255, green: 255, blue: 255 } );
    };
    this.setBackgroundColor = function(color) {
        Overlays.editOverlay(this.background, { red: 125, green: 125, blue: 255  });
    };

}

// The ColorBox class
ColorBox = function(x,y,width,thumbSize) {
    var self = this;
    
    var slideHeight = thumbSize / 3;
    var sliderWidth = width;
    this.red = new Slider(x, y, width, slideHeight);
    this.green = new Slider(x, y + slideHeight, width, slideHeight);
    this.blue = new Slider(x, y + 2 * slideHeight, width, slideHeight);
    this.red.setBackgroundColor({x: 1, y: 0, z: 0});
    this.green.setBackgroundColor({x: 0, y: 1, z: 0});
    this.blue.setBackgroundColor({x: 0, y: 0, z: 1});

    this.isClickableOverlayItem = function(item) {
        return this.red.isClickableOverlayItem(item) 
            || this.green.isClickableOverlayItem(item)
            || this.blue.isClickableOverlayItem(item);
    };

    this.onMouseMoveEvent = function(event) { 
        this.red.onMouseMoveEvent(event);
        this.green.onMouseMoveEvent(event);
        this.blue.onMouseMoveEvent(event);
    };

    this.onMousePressEvent = function(event, clickedOverlay) {
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

    this.onMouseReleaseEvent = function(event) {
        this.red.onMouseReleaseEvent(event);
        this.green.onMouseReleaseEvent(event);
        this.blue.onMouseReleaseEvent(event);
    };

    this.setterFromWidget = function(value) {
        var color = self.getValue();
        self.onValueChanged(color);
        self.updateRGBSliders(color);
    };

    this.red.onValueChanged = this.setterFromWidget;
    this.green.onValueChanged = this.setterFromWidget;
    this.blue.onValueChanged = this.setterFromWidget;

    this.updateRGBSliders = function(color) {
        this.red.setThumbColor({x: color.x, y: 0, z: 0});
        this.green.setThumbColor({x: 0, y: color.y, z: 0});
        this.blue.setThumbColor({x: 0, y: 0, z: color.z});
    };

    // Public members:
    this.setValue = function(value) {
        this.red.setValue(value.x);
        this.green.setValue(value.y);
        this.blue.setValue(value.z);
        this.updateRGBSliders(value);  
    };

    this.getValue = function() {
        var value = {x:this.red.getValue(), y:this.green.getValue(),z:this.blue.getValue()};
        return value;
    };

    this.destroy = function() {
        this.red.destroy();
        this.green.destroy();
        this.blue.destroy();
    };

    this.onValueChanged = function(value) {};
}

// The DirectionBox class
DirectionBox = function(x,y,width,thumbSize) {
    var self = this;
    
    var slideHeight = thumbSize / 2;
    var sliderWidth = width;
    this.yaw = new Slider(x, y, width, slideHeight);
    this.pitch = new Slider(x, y + slideHeight, width, slideHeight);
    
    this.yaw.setThumbColor({x: 1, y: 0, z: 0});
    this.yaw.minValue = -180;
    this.yaw.maxValue = +180;

    this.pitch.setThumbColor({x: 0, y: 0, z: 1});
    this.pitch.minValue = -1;
    this.pitch.maxValue = +1;

    this.isClickableOverlayItem = function(item) {
        return this.yaw.isClickableOverlayItem(item) 
            || this.pitch.isClickableOverlayItem(item);
    };

    this.onMouseMoveEvent = function(event) { 
        this.yaw.onMouseMoveEvent(event);
        this.pitch.onMouseMoveEvent(event);
    };

    this.onMousePressEvent = function(event, clickedOverlay) {
        this.yaw.onMousePressEvent(event, clickedOverlay);
        if (this.yaw.isMoving) {
            return;
        }
        this.pitch.onMousePressEvent(event, clickedOverlay);
    };

    this.onMouseReleaseEvent = function(event) {
        this.yaw.onMouseReleaseEvent(event);
        this.pitch.onMouseReleaseEvent(event);
    };

    this.setterFromWidget = function(value) {
        var yawPitch = self.getValue();
        self.onValueChanged(yawPitch);
    };

    this.yaw.onValueChanged = this.setterFromWidget;
    this.pitch.onValueChanged = this.setterFromWidget;

    // Public members:
    this.setValue = function(direction) {
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

    this.getValue = function() {
        var dirZ = this.pitch.getValue();
        var yaw = this.yaw.getValue() * Math.PI / 180;
        var cosY = Math.sqrt(1 - dirZ*dirZ);
        var value = {x:cosY * Math.cos(yaw), y:dirZ, z: cosY * Math.sin(yaw)};
        return value;
    };

    this.destroy = function() {
        this.yaw.destroy();
        this.pitch.destroy();
    };

    this.onValueChanged = function(value) {};
}

var textFontSize = 12;

function PanelItem(name, setter, getter, displayer, x, y, textWidth, valueWidth, height) {
    //print("creating panel item: " + name);
    
    this.name = name;

    this.displayer = typeof displayer !== 'undefined' ? displayer : function(value) { 
        if(value == true) { 
            return "On";
        } else if (value == false) {
            return "Off";
        }
        return value.toFixed(2); 
    };

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
    this.resetValue = getter();

    this.setter = function(value) {
        
        setter(value);

        Overlays.editOverlay(this.value, {text: this.displayer(getter())});

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
        Overlays.editOverlay(this.value, {text: this.displayer(value)});    
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

            if (widget.isClickableOverlayItem(clickedOverlay)) {
                this.activeWidget = widget;
                this.activeWidget.onMousePressEvent(event, clickedOverlay);        
              
                break;
            }    
        }  
    };

    // Reset panel item upon double-clicking
    this.mouseDoublePressEvent = function(event) {
        var clickedOverlay = Overlays.getOverlayAtPoint({x: event.x, y: event.y});
        for (var i in this.items) {

            var item = this.items[i]; 
            var widget = item.widget;
            
            if (item.title == clickedOverlay || item.value == clickedOverlay) {
                widget.updateThumb();
                widget.onValueChanged(item.resetValue);
                break;
            }    
        }  
    }

    this.mouseReleaseEvent = function(event) {
        if (this.activeWidget) {
            this.activeWidget.onMouseReleaseEvent(event);
        }
        this.activeWidget = null;
    };

    this.newSlider = function(name, minValue, maxValue, setValue, getValue, displayValue) {

        var item = new PanelItem(name, setValue, getValue, displayValue, this.x, this.nextY, textWidth, valueWidth, rawHeight);

        var slider = new Slider(this.widgetX, this.nextY, widgetWidth, rawHeight);
        slider.minValue = minValue;
        slider.maxValue = maxValue;

        item.widget = slider;
        item.widget.onValueChanged = function(value) { item.setterFromWidget(value); };
        item.setter(getValue());      
        this.items[name] = item;
        this.nextY += rawYDelta; 
    };

    this.newCheckbox = function(name, setValue, getValue, displayValue) {
        var display;
        if (displayValue == true) {
            display = function() {return "On";};
        } else if (displayValue == false) {
            display = function() {return "Off";};
        }
 
        var item = new PanelItem(name, setValue, getValue, display, this.x, this.nextY, textWidth, valueWidth, rawHeight);

        var checkbox = new Checkbox(this.widgetX, this.nextY, widgetWidth, rawHeight);
 
        item.widget = checkbox;
        item.widget.onValueChanged = function(value) { item.setterFromWidget(value); };
        item.setter(getValue()); 
        this.items[name] = item;
        this.nextY += rawYDelta;
        //print("created Item... checkbox=" + name);     
    };

    this.newColorBox = function(name, setValue, getValue, displayValue) {

        var item = new PanelItem(name, setValue, getValue, displayValue, this.x, this.nextY, textWidth, valueWidth, rawHeight);

        var colorBox = new ColorBox(this.widgetX, this.nextY, widgetWidth, rawHeight);

        item.widget = colorBox;
        item.widget.onValueChanged = function(value) { item.setterFromWidget(value); };
        item.setter(getValue());      
        this.items[name] = item;
        this.nextY += rawYDelta;
      //  print("created Item... colorBox=" + name);     
    };

    this.newDirectionBox= function(name, setValue, getValue, displayValue) {

        var item = new PanelItem(name, setValue, getValue, displayValue, this.x, this.nextY, textWidth, valueWidth, rawHeight);

        var directionBox = new DirectionBox(this.widgetX, this.nextY, widgetWidth, rawHeight);

        item.widget = directionBox;
        item.widget.onValueChanged = function(value) { item.setterFromWidget(value); };
        item.setter(getValue());      
        this.items[name] = item;
        this.nextY += rawYDelta;
      //  print("created Item... directionBox=" + name);     
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

    this.update = function(name) {
        var item = this.items[name];
        if (item != null) {
            return item.setter(item.getter());
        }
        return null;
    }

};


