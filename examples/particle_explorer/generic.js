//
//  generic.js
//  
//
//  Created by James B. Pollack @imgntnon 9/26/2015
//  Copyright 2014 High Fidelity, Inc.
//
//  Web app side of the App - contains GUI.
//  This is an example of a new, easy way to do two way bindings between dynamically created GUI and in-world entities.  
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  todo: folders, color pickers, animation settings, scale gui width with window resizing  
//

var Settings = function() {
    return;
}

//we need a way to keep track of our gui controllers
var controllers = [];

var gui;
var settings = new Settings();


function convertBinaryToBoolean(value) {
    if (value === 0) {
        return false
    } else {
        return true
    }
}

function loadGUI() {
    console.log('loadGUI ')
        //instantiate our object

    //whether or not to autoplace
    gui = new dat.GUI({
        autoPlace: false
    });

    //if not autoplacing, put gui in a custom container
    if (gui.autoPlace === false) {

        var customContainer = document.getElementById('my-gui-container');
        customContainer.appendChild(gui.domElement);
        gui.width = 400;

    }

    //add save settings ability (try not to use localstorage)
    gui.remember(settings);

    //get object keys
    var keys = _.keys(settings);

    //for each key...
    _.each(keys, function(key) {

        // if(key==='dimensions'){
        //     createVec3Folder(key)
        // }
        // else{
        //     return
        // }
        // //FOR NOW TO SHOW THAT IT WORKS RESTRICT TO A FEW PROPERTIES.  NEED TO ADD SUPPORT FOR VEC3, QUAT, COLORS
        if (key.indexOf('name') !== -1 || key.indexOf('description') !== -1 || key.indexOf('visible') !== -1 || key.indexOf('collisionsWillMove') !== -1) {


            // we aren't getting checkboxes for collisionsWillMove because it comes across the wire as a 1, not a true

            if (key.indexOf('visible') !== -1) {
                settings.visible = convertBinaryToBoolean(settings.visible)

            }
            if (key.indexOf('collisionsWillMove') !== -1) {
                settings.collisionsWillMove = convertBinaryToBoolean(settings.collisionsWillMove)

            }
        } else {
            return
        }

        console.log('key:::' + key)
            //add this key as a controller to the gui

        var controller = gui.add(settings, key).listen();
        // the call below is potentially expensive but will enable two way binding.  needs testing to see how many it supports at once.
        //keep track of our controller
        controllers.push(controller);

        //hook into change events for this gui controller
        controller.onChange(function(value) {
            // Fires on every change, drag, keypress, etc.
        });

        controller.onFinishChange(function(value) {
            // Fires when a controller loses focus.
            writeDataToInterface(this.property, value)
        });

    });

    //after all the keys make folders
    var folder = gui.addFolder('dimensions');
    folder.add(settings.dimensions, 'x').min(0).listen().onFinishChange(function(value) {
        // Fires when a controller loses focus.
        var obj = {};
        obj.dimensions = {};
        obj.dimensions[this.property] = value;
        obj.dimensions.y = settings.dimensions.y;
        obj.dimensions.z = settings.dimensions.z;
        writeVec3ToInterface(obj)
    });
    folder.add(settings.dimensions, 'y').min(0).listen().onFinishChange(function(value) {
        // Fires when a controller loses focus.
        var obj = {};
        obj.dimensions = {};
        obj.dimensions[this.property] = value;
        obj.dimensions.x = settings.dimensions.x;
        obj.dimensions.z = settings.dimensions.z;

        writeVec3ToInterface(obj)
    });
    folder.add(settings.dimensions, 'z').min(0).listen().onFinishChange(function(value) {
        // Fires when a controller loses focus.
        var obj = {};
        obj.dimensions = {};
        obj.dimensions[this.property] = value;
        obj.dimensions.x = settings.dimensions.x;
        obj.dimensions.y = settings.dimensions.y;
        writeVec3ToInterface(obj)
    });
    folder.open();
};

function writeDataToInterface(property, value) {
    console.log('WRITE SOME DATA TO INTERFACE' + property + ":::" + value)
    var data = {};
    data[property] = value;
    var sendData = {
        messageType: "settings_update",
        updatedSettings: data,
    }

    var stringifiedData = JSON.stringify(sendData)

    EventBridge.emitWebEvent(
        stringifiedData
    );


}

function writeVec3ToInterface(obj) {
    print('VEC3 UPDATE TO INTERFACE FROM SETTINGS')
    var sendData = {
        messageType: "settings_update",
        updatedSettings: obj,
    }

    var stringifiedData = JSON.stringify(sendData)

    EventBridge.emitWebEvent(
        stringifiedData
    );


}

window.onload = function() {
    console.log('GUI PAGE LOADED');

    if (typeof EventBridge !== 'undefined') {
        console.log('WE HAVE AN EVENT BRIDGE, SEND PAGE LOADED EVENT ')

        var stringifiedData = JSON.stringify({
            messageType: 'page_loaded'
        })

        console.log('SEND PAGE LOADED EVENT FROM GUI TO INTERFACE ')

        EventBridge.emitWebEvent(
            stringifiedData
        );
        listenForSettingsUpdates();
    } else {
        console.log('No event bridge, probably not in interface.')
    }

}

function listenForSettingsUpdates() {
    console.log('GUI IS LISTENING FOR MESSAGES FROM INTERFACE')
    EventBridge.scriptEventReceived.connect(function(data) {
        data = JSON.parse(data);

        if (data.messageType === 'object_update') {

            _.each(data.objectSettings, function(value, key) {

                settings[key] = value;

                if (key.indexOf('visible') !== -1) {
                    settings.visible = convertBinaryToBoolean(settings.visible)

                }
                if (key.indexOf('collisionsWillMove') !== -1) {
                    settings.collisionsWillMove = convertBinaryToBoolean(settings.collisionsWillMove)

                }

            })
            manuallyUpdateDisplay();
        }
        if (data.messageType === 'initial_settings') {
            console.log('INITIAL SETTINGS FROM INTERFACE:::' + JSON.stringify(data.initialSettings))

            _.each(data.initialSettings, function(value, key) {
                console.log('key  ' + key);
                console.log('value  ' + JSON.stringify(value));
                settings[key] = {};
                settings[key] = value;
            })
            loadGUI();
        }
        if (data.messageType === 'settings_update') {
            console.log('SETTINGS UPDATE FROM INTERFACE:::' + JSON.stringify(data.updatedSettings))
            _.each(data.updatedSettings, function(value, key) {
                console.log('setting,value', setting, value)
                settings = {}

                settings[key] = value;
            })


        }

    });

}


function manuallyUpdateDisplay(gui) {
    // Iterate over all controllers
    for (var i in gui.__controllers) {
        gui.__controllers[i].updateDisplay();
    }
}