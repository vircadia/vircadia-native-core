//
//  main.js

//  Created by James B. Pollack @imgntn on 9/26/2015
//  Copyright 2015 High Fidelity, Inc.

//  Web app side of the App - contains GUI.
//  This is an example of a new, easy way to do two way bindings between dynamically created GUI and in-world entities.  
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  todo: quaternion folders, color pickers, read-only properties, animation settings and other nested objects, scale gui width with window resizing  
/*global window, EventBridge, dat, convertBinaryToBoolean, listenForSettingsUpdates,createVec3Folder,createQuatFolder,writeVec3ToInterface,writeDataToInterface*/

var Settings = function() {
    return;
};

var controllers = [];
var folders = [];
var gui;
var settings = new Settings();
var updateInterval;
var UPDATE_ALL_FREQUENCY = 1000;

var keysToIgnore = [
    'script',
    'visible',
    'locked',
    'userData',
    'position',
    'dimensions',
    'rotation',
    'id',
    'description',
    'type',
    'created',
    'age',
    'ageAsText',
    'boundingBox',
    'naturalDimensions',
    'naturalPosition',
    'velocity',
    'gravity',
    'acceleration',
    'damping',
    'restitution',
    'friction',
    'density',
    'lifetime',
    'scriptTimestamp',
    'registrationPoint',
    'angularVelocity',
    'angularDamping',
    'ignoreForCollisions',
    'collisionsWillMove',
    'href',
    'actionData',
    'marketplaceID',
    'collisionSoundURL',
    'shapeType',
    'animationSettings',
    'animationFrameIndex',
    'animationIsPlaying',
    'sittingPoints',
    'originalTextures'
];

var individualKeys = [];
var vec3Keys = [];
var quatKeys = [];
var colorKeys = [];

function convertBinaryToBoolean(value) {
    if (value === 0) {
        return false;
    }
    return true;
}

function loadGUI() {
    console.log('loadGUI ');

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

        var shouldIgnore = _.contains(keysToIgnore, key);
        if (shouldIgnore) {
            return
        }
        var subKeys = _.keys(settings[key]);
        var hasX = _.contains(subKeys, 'x');
        var hasY = _.contains(subKeys, 'y');
        var hasZ = _.contains(subKeys, 'z');
        var hasW = _.contains(subKeys, 'w');
        var hasRed = _.contains(subKeys, 'red');
        var hasGreen = _.contains(subKeys, 'green');
        var hasBlue = _.contains(subKeys, 'blue');
        if ((hasX && hasY && hasZ) && hasW === false) {
            console.log(key + " is a vec3");
            vec3Keys.push(key);
        } else if (hasX && hasY && hasZ && hasW) {
            console.log(key + " is a quaternion");
            quatKeys.push(key)
        } else if (hasRed || hasGreen || hasBlue) {
            console.log(key + " is a color");
            colorKeys.push(key);

        } else {
            console.log(key + ' is a single key not an obj')
            individualKeys.push(key);

        }


    });

    individualKeys.sort();
    vec3Keys.sort();
    quatKeys.sort();
    colorKeys.sort();
    addIndividualKeys();
    addFolders();
    setInterval(manuallyUpdateDisplay, UPDATE_ALL_FREQUENCY);

}

function addIndividualKeys() {
    _.each(individualKeys, function(key) {

        var controller = gui.add(settings, key)

        //need to fix not being able to input values if constantly listening
        //.listen();
        //  keep track of our controller
        controllers.push(controller);

        //hook into change events for this gui controller
        controller.onChange(function(value) {
            // Fires on every change, drag, keypress, etc.
            writeDataToInterface(this.property, value);
        });
    })
}

function addFolders() {
    _.each(vec3Keys, function(key) {
        createVec3Folder(key);
    })
    _.each(quatKeys, function(key) {
        createQuatFolder(key);
    })
    _.each(colorKeys, function(key) {
        createColorFolder(key);
    })
}

function createVec3Folder(category) {
    var folder = gui.addFolder(category);
    folder.add(settings[category], 'x').step(0.1).onChange(function(value) {
        // Fires when a controller loses focus.
        var obj = {};
        obj[category] = {};
        obj[category][this.property] = value;
        obj[category].y = settings[category].y;
        obj[category].z = settings[category].z;
        writeVec3ToInterface(obj);
    });
    folder.add(settings[category], 'y').step(0.1).onChange(function(value) {
        // Fires when a controller loses focus.
        var obj = {};
        obj[category] = {};
        obj[category].x = settings[category].x;
        obj[category][this.property] = value;
        obj[category].z = settings[category].z;
        writeVec3ToInterface(obj);
    });
    folder.add(settings[category], 'z').step(0.1).onChange(function(value) {
        // Fires when a controller loses focus.
        var obj = {};
        obj[category] = {};
        obj[category].y = settings[category].y;
        obj[category].x = settings[category].x;
        obj[category][this.property] = value;
        writeVec3ToInterface(obj);
    });
    folder.open();
    folders.push(folder);
}

function createQuatFolder(category) {
    var folder = gui.addFolder(category);
    folder.add(settings[category], 'x').step(0.1).onChange(function(value) {
        // Fires when a controller loses focus.
        var obj = {};
        obj[category] = {};
        obj[category][this.property] = value;
        obj[category].y = settings[category].y;
        obj[category].z = settings[category].z;
        obj[category].w = settings[category].w;
        writeVec3ToInterface(obj);
    });
    folder.add(settings[category], 'y').step(0.1).onChange(function(value) {
        // Fires when a controller loses focus.
        var obj = {};
        obj[category] = {};
        obj[category].x = settings[category].x;
        obj[category][this.property] = value;
        obj[category].z = settings[category].z;
        obj[category].w = settings[category].w;
        writeVec3ToInterface(obj);
    });
    folder.add(settings[category], 'z').step(0.1).onChange(function(value) {
        // Fires when a controller loses focus.
        var obj = {};
        obj[category] = {};
        obj[category].x = settings[category].x;
        obj[category].y = settings[category].y;
        obj[category][this.property] = value;
        obj[category].w = settings[category].w;
        writeVec3ToInterface(obj);
    });
    folder.add(settings[category], 'w').step(0.1).onChange(function(value) {
        // Fires when a controller loses focus.
        var obj = {};
        obj[category] = {};
        obj[category].x = settings[category].x;
        obj[category].y = settings[category].y;
        obj[category].z = settings[category].z;
        obj[category][this.property] = value;
        writeVec3ToInterface(obj);
    });
    folder.open();
    folders.push(folder);
}

function createColorFolder(category) {
    console.log('CREATING COLOR FOLDER', category)
    var folder = gui.addFolder(category);
    folder.add(settings[category], 'red').min(0).max(255).step(1).onChange(function(value) {
        // Fires when a controller loses focus.
        var obj = {};
        obj[category] = {};
        obj[category][this.property] = value;
        obj[category].green = settings[category].green;
        obj[category].blue = settings[category].blue;
        writeVec3ToInterface(obj);
    });
    folder.add(settings[category], 'green').min(0).max(255).step(1).onChange(function(value) {
        // Fires when a controller loses focus.
        var obj = {};
        obj[category] = {};

        obj[category].red = settings[category].red;
        obj[category][this.property] = value;
        obj[category].blue = settings[category].blue;
        writeVec3ToInterface(obj);
    });
    folder.add(settings[category], 'blue').min(0).max(255).step(1).onChange(function(value) {
        // Fires when a controller loses focus.
        var obj = {};
        obj[category] = {};
        obj[category].red = settings[category].red;
        obj[category].green = settings[category].green;

        obj[category][this.property] = value;
        writeVec3ToInterface(obj);
    });
    folder.open();
    folders.push(folder);
}

function writeDataToInterface(property, value) {
    console.log('WRITE SOME DATA TO INTERFACE' + property + ":::" + value);
    var data = {};
    data[property] = value;
    var sendData = {
        messageType: "settings_update",
        updatedSettings: data,
    };

    var stringifiedData = JSON.stringify(sendData);

    EventBridge.emitWebEvent(
        stringifiedData
    );


}

function writeVec3ToInterface(obj) {
    console.log('VEC3 UPDATE TO INTERFACE FROM SETTINGS');
    var sendData = {
        messageType: "settings_update",
        updatedSettings: obj,
    };

    var stringifiedData = JSON.stringify(sendData);

    EventBridge.emitWebEvent(
        stringifiedData
    );


}

window.onload = function() {
    console.log('GUI PAGE LOADED');

    if (typeof EventBridge !== 'undefined') {
        console.log('WE HAVE AN EVENT BRIDGE, SEND PAGE LOADED EVENT ');

        var stringifiedData = JSON.stringify({
            messageType: 'page_loaded'
        });

        console.log('SEND PAGE LOADED EVENT FROM GUI TO INTERFACE ');

        EventBridge.emitWebEvent(
            stringifiedData
        );

        listenForSettingsUpdates();
    } else {
        console.log('No event bridge, probably not in interface.');
    }

};

function listenForSettingsUpdates() {
    console.log('GUI IS LISTENING FOR MESSAGES FROM INTERFACE');
    EventBridge.scriptEventReceived.connect(function(data) {
        data = JSON.parse(data);

        if (data.messageType === 'object_update') {
            _.each(data.objectSettings, function(value, key) {
                settings[key] = value;
            });


        }
        if (data.messageType === 'initial_settings') {
            console.log('INITIAL SETTINGS FROM INTERFACE:::' + JSON.stringify(data.initialSettings));

            _.each(data.initialSettings, function(value, key) {
                console.log('key  ' + key);
                console.log('value  ' + JSON.stringify(value));
                settings[key] = {};
                settings[key] = value;
            });

            loadGUI();
        }
        // if (data.messageType === 'settings_update') {
        //     console.log('SETTINGS UPDATE FROM INTERFACE:::' + JSON.stringify(data.updatedSettings));
        //     _.each(data.updatedSettings, function(value, key) {
        //         settings[key] = value;
        //     });
        // }

    });

}


function manuallyUpdateDisplay() {
    // Iterate over all controllers
    var i;
    for (i in gui.__controllers) {
        gui.__controllers[i].updateDisplay();
    }
}

function removeContainerDomElement() {
    var elem = document.getElementById("my-gui-container");
    elem.parentNode.removeChild(elem);
}