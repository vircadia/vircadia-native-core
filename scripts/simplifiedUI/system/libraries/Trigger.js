"use strict";

/*jslint vars: true, plusplus: true*/
/*globals Script, Overlays, Controller, Reticle, HMD, Camera, Entities, MyAvatar, Settings, Menu, ScriptDiscoveryService, Window, Vec3, Quat, print*/

Trigger = function(properties) {
    properties = properties || {};
    var that = this;
    that.label = properties.label || Math.random();
    that.SMOOTH_RATIO = properties.smooth || 0.1; //  Time averaging of trigger - 0.0 disables smoothing
    that.DEADZONE = properties.deadzone || 0.10; // Once pressed, a trigger must fall below the deadzone to be considered un-pressed once pressed.
    that.HYSTERESIS = properties.hystersis || 0.05; // If not pressed, a trigger must go above DEADZONE + HYSTERSIS to be considered pressed

    that.value = 0;
    that.pressed = false;
    that.clicked = false;

    // Handlers
    that.onPress = properties.onPress || function(){
        print("Pressed trigger " + that.label)
    };
    that.onRelease = properties.onRelease || function(){
        print("Released trigger " + that.label)
    };
    that.onClick = properties.onClick || function(){
        print("Clicked trigger " + that.label)
    };
    that.onUnclick = properties.onUnclick || function(){
        print("Unclicked trigger " + that.label)
    };
    
    // Getters
    that.isPressed = function() {
        return that.pressed;
    }
    
    that.isClicked = function() {
        return that.clicked;
    }
    
    that.getValue = function() {
        return that.value;
    }

    
    // Private values
    var controller = properties.controller || Controller.Standard.LT;
    var controllerClick = properties.controllerClick || Controller.Standard.LTClick;
    that.mapping =  Controller.newMapping('com.highfidelity.controller.trigger.' + controller + '-' + controllerClick + '.' + that.label + Math.random());
    Script.scriptEnding.connect(that.mapping.disable);

    // Setup mapping,
    that.mapping.from(controller).peek().to(function(value) {
        that.value = (that.value * that.SMOOTH_RATIO) +
            (value * (1.0 - that.SMOOTH_RATIO)); 

        var oldPressed = that.pressed;
        if (!that.pressed && that.value >= (that.DEADZONE + that.HYSTERESIS)) {
            that.pressed = true;
            that.onPress();
        }

        if (that.pressed && that.value < that.HYSTERESIS) {
            that.pressed = false;
            that.onRelease();
        }
    });
    
    that.mapping.from(controllerClick).peek().to(function(value){
        if (!that.clicked && value > 0.0) {
            that.clicked = true;
            that.onClick();
        } 
        if (that.clicked && value == 0.0) {
            that.clicked = false;
            that.onUnclick();
        }
    });

    that.enable = function() {
        that.mapping.enable();
    }

    that.disable = function() {
        that.mapping.disable();
    }
}
