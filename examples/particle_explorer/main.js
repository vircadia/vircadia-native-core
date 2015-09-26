//
//  main.js
//  
//
//  Created by James B. Pollack @imgntnon 9/26/2015
//  Copyright 2014 High Fidelity, Inc.
//
//  Web app side of the App - contains GUI.
//  Quickly edit the aesthetics of a particle system.  This is an example of a new, easy way to do two way bindings between dynamically created GUI and in-world entities.  
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  todo: folders, color pickers, animation settings, scale gui width with window resizing  
//

var PI = 3.141593,
    DEG_TO_RAD = PI / 180.0;

function radiansToDegrees(radians) {
    return radians * (180 / PI)
}

// need to add '_group' to the end of properties that will need to be made part of a group on the backend
// property_subproperty_group = value 
// i.e. color_red_group = 0;

var groups = [
'accelerationSpread',
'color',
'colorSpread',
'colorStart',
'colorFinish',
'emitAcceleration',
'emitDimensions',
'emitOrientation'
]

var ParticleExplorer = function() {
    this.animationIsPlaying = true;
    this.textures = "https://hifi-public.s3.amazonaws.com/alan/Particles/Particle-Sprite-Smoke-1.png";
    this.lifespan = 5.0;
    this.visible = false;
    this.locked = false;
    this.lifetime = 3600 // 1 hour; just in case
    this.accelerationSpread_x = 0.1;
    this.accelerationSpread_y = 0.1;
    this.accelerationSpread_z = 0.1;
    this.alpha = 0.5;
    this.alphaStart = 1.0;
    this.alphaFinish = 0.1;
    this.color_red= 0;
    this.color_green = 0;
    this.color_blue = 0;
    this.colorSpread_red= 0;
    this.colorSpread_green = 0;
    this.colorSpread_blue = 0;
    this.colorStart_red = 0;
    this.colorStart_green = 0;
    this.colorStart_blue= 0;
    this.colorFinish_red = 0;
    this.colorFinish_green = 0;
    this.colorFinish_blue = 0;
    this.azimuthStart = -PI / 2.0;
    this.azimuthFinish = PI / 2.0;
    this.emitAcceleration_x= 0.01;
    this.emitAcceleration_y = 0.01;
    this.emitAcceleration_z= 0.01;
    this.emitDimensions_x= 0.01;
    this.emitDimensions_y = 0.01;
    this.emitDimensions_z = 0.01;
    this.emitOrientation_x = 0.01;
    this.emitOrientation_y = 0.01;
    this.emitOrientation_z = 0.01;
    this.emitOrientation_w = 0.01;
    this.emitRate = 0.1;
    this.emitSpeed = 0.1;
    this.polarStart = 0.01;
    this.polarFinish = 2.0 * DEG_TO_RAD;
    this.speedSpread = 0.1;
    this.radiusSpread = 0.035;
    this.radiusStart = 0.0;
    this.radiusFinish = 0.0;
    this.velocitySpread = 0;

}

//we need a way to keep track of our gui controllers
var controllers = [];
window.onload = function() {

    //instantiate our object
    var particleExplorer = new ParticleExplorer();

    //whether or not to autoplace
    var gui = new dat.GUI({
        autoPlace: false
    });

    //if not autoplacing, put gui in a custom container
    if (gui.autoPlace === false) {

        var customContainer = document.getElementById('my-gui-container');
        customContainer.appendChild(gui.domElement);
        gui.width = 400;

    }

    //add save settings ability (try not to use localstorage)
    gui.remember(particleExplorer);

    //get object keys
    var particleKeys = _.keys(particleExplorer);

    //for each key...
    _.each(particleKeys, function(key) {
        //add this key as a controller to the gui
        var controller = gui.add(particleExplorer, key);

       var  putInGroup = false;
        _.each(groups,function(group){
            if(key.indexOf(group)>-1){
                putInGroup=true;
            }
        })

        if (putInGroup===true) {
            controller.shouldGroup = true;
        } else {
            controller.shouldGroup = false;
        }

        //keep track of our controller
        controllers.push(controller);

        //hook into change events for this gui controller
        controller.onChange(function(value) {
            // Fires on every change, drag, keypress, etc.
            // writeDataToInterface(this.property, value, this.shouldGroup)
        });

        controller.onFinishChange(function(value) {
            // console.log('should group?', controller.shouldGroup)
                // Fires when a controller loses focus.
            writeDataToInterface(this.property, value, this.shouldGroup)
        });

    });

};

function writeDataToInterface(property, value, shouldGroup) {
    // console.log('property, value, shouldGroup',property, value, shouldGroup)
    var group = null;
    var groupProperty = null;
    var groupProperties = null;
    
    if (shouldGroup) {
        var separated = property.split("_");
        console.log(separated)
        group = separated[0];
        groupProperty = separated[1];
        var groupProperties = {}
        groupProperties[group] = {};
        groupProperties[group][groupProperty] = value
        // console.log(groupProperties)
    }

    var data = {
        type: "particleExplorer_update",
        shouldGroup: shouldGroup,
        groupProperties: groupProperties,
        singleProperty: {}
    }

    data['singleProperty'][property] = value

    var stringifiedData = JSON.stringify(data)

    // console.log('stringifiedData',stringifiedData)
    if (typeof EventBridge !== 'undefined') {
        EventBridge.emitWebEvent(
            stringifiedData
        );
    }

}