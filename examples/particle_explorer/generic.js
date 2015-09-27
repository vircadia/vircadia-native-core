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
var settings = new Settings();

function loadGUI() {
    console.log('loadGUI ')
        //instantiate our object

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
    gui.remember(settings);

    //get object keys
    var keys = _.keys(settings);

    //for each key...
    _.each(keys, function(key) {
        console.log('KEY KEY KEY' + key);
        //add this key as a controller to the gui
        var controller = gui.add(settings, key).listen();
        // the call below is potentially expensive but will enable two way binding.  needs testing to see how many it supports at once.
        //var controller = gui.add(particleExplorer, key).listen();
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

};

function writeDataToInterface(property, value) {
    console.log('WRITE SOME DATA TO INTERFACE')
    var settings = {};
    settings[property] = value;
    var data = {
        type: "settings_update",
        updatedSettings: settings,
    }

    var stringifiedData = JSON.stringify(data)

    // console.log('stringifiedData',stringifiedData)
    if (typeof EventBridge !== 'undefined') {
        EventBridge.emitWebEvent(
            stringifiedData
        );
    }

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
        console.log('GOT MESSAGE FROM EVENT BRIDGE')
        data = JSON.parse(data);
        if (data.messageType === 'initial_settings') {
            console.log('INITIAL SETTINGS FROM INTERFACE:::' + JSON.stringify(data.initialSettings))
            var initialSettings = data.initialSettings;
            _.each(initialSettings, function(value, key) {
                console.log('key  ' + key);
                console.log('value  ' + JSON.stringify(value));
                settings[key] = {};
                settings[key] = value;
            })
            loadGUI();
        }
        if (data.messageType === 'settings_update') {
            console.log('SETTINGS UPDATE FROM INTERFACE:::' + JSON.stringify(data.updatedSettings))
            var initialSettings = data.updatedSettings;
            _.each(initialSettings, function(value, key) {
                console.log('setting,value', setting, value)
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