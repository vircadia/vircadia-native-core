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
/*global window, alert, EventBridge, dat, convertBinaryToBoolean, listenForSettingsUpdates,createVec3Folder,createQuatFolder,writeVec3ToInterface,writeDataToInterface*/

var Settings = function() {
    this.exportSettings = function() {
        //copyExportSettingsToClipboard();
        showPreselectedPrompt();
    };
    this.importSettings = function() {
        importSettings();
    }
};

//two way bindings-aren't quite ready yet.  see bottom of file.
var AUTO_UPDATE = false;
var UPDATE_ALL_FREQUENCY = 100;

var controllers = [];
var colorControllers = [];
var folders = [];
var gui = null;
var settings = new Settings();
var updateInterval;

var currentInputField;
var storedController;
var keysToIgnore = [
    'importSettings',
    'exportSettings',
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
    'animationFPS',
    'sittingPoints',
    'originalTextures'
];

var individualKeys = [];
var vec3Keys = [];
var quatKeys = [];
var colorKeys = [];

window.onload = function() {
    if (typeof EventBridge !== 'undefined') {

        var stringifiedData = JSON.stringify({
            messageType: 'page_loaded'
        });

        EventBridge.emitWebEvent(
            stringifiedData
        );

        listenForSettingsUpdates();
        window.onresize = setGUIWidthToWindowWidth;
    } else {
        console.log('No event bridge, probably not in interface.');
    }

};


function loadGUI() {

    //whether or not to autoplace
    gui = new dat.GUI({
        autoPlace: false
    });

    //if not autoplacing, put gui in a custom container
    if (gui.autoPlace === false) {
        var customContainer = document.getElementById('my-gui-container');
        customContainer.appendChild(gui.domElement);

    }

    // presets for the GUI itself.   a little confusing and import/export is mostly what we want to do at the moment.
    // gui.remember(settings);

    var keys = _.keys(settings);

    _.each(keys, function(key) {

        var shouldIgnore = _.contains(keysToIgnore, key);
        if (shouldIgnore) {
            return;
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
            // console.log(key + " is a vec3");
            vec3Keys.push(key);
        } else if (hasX && hasY && hasZ && hasW) {
            // console.log(key + " is a quaternion");
            quatKeys.push(key);
        } else if (hasRed || hasGreen || hasBlue) {
            // console.log(key + " is a color");
            colorKeys.push(key);

        } else {
            // console.log(key + ' is a single key not an obj')
            individualKeys.push(key);
        }

    });

    individualKeys.sort();
    vec3Keys.sort();
    quatKeys.sort();
    colorKeys.sort();
    gui.add(settings, 'importSettings');
    gui.add(settings, 'exportSettings');
    addIndividualKeys();
    addFolders();
    if (AUTO_UPDATE) {
        setInterval(manuallyUpdateDisplay, UPDATE_ALL_FREQUENCY);
    }
 
    gui.width = window.innerWidth;

    //2-way
    //registerDOMElementsForListenerBlocking();
}

function addIndividualKeys() {
    _.each(individualKeys, function(key) {
        //temporary patch for not crashing when this goes below 0
        if(key.indexOf('emitRate')>-1){
             var controller = gui.add(settings, key).min(0);
        }
        else{
            var controller = gui.add(settings, key);
        }
        
        //need to fix not being able to input values if constantly listening
        //.listen();

        //  keep track of our controller
        controllers.push(controller);

        //hook into change events for this gui controller
        controller.onChange(function(value) {
            // Fires on every change, drag, keypress, etc.
            writeDataToInterface(this.property, value);
        });
    });
}

function addFolders() {
    _.each(colorKeys, function(key) {
        createColorPicker(key);
    });
    _.each(vec3Keys, function(key) {
        createVec3Folder(key);
    });
    _.each(quatKeys, function(key) {
        createQuatFolder(key);
    });

}

function createColorPicker(key) {
    var colorObject = settings[key];
    var colorArray = convertColorObjectToArray(colorObject);
    settings[key] = colorArray;
    var controller = gui.addColor(settings, key);
    controller.onChange(function(value) {
        var obj = {};
        obj[key] = convertColorArrayToObject(value);
        writeVec3ToInterface(obj);
    });

    return;
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
    folders.push(folder);
    folder.open();
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
    folders.push(folder);
    folder.open();
}

function convertColorObjectToArray(colorObject) {
    var colorArray = [];
    _.each(colorObject, function(singleColor) {
        colorArray.push(singleColor);
    })
    return colorArray
}

function convertColorArrayToObject(colorArray) {
    var colorObject = {
        red: colorArray[0],
        green: colorArray[1],
        blue: colorArray[2]
    }
    return colorObject
}

function writeDataToInterface(property, value) {
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
    var sendData = {
        messageType: "settings_update",
        updatedSettings: obj,
    };

    var stringifiedData = JSON.stringify(sendData);

    EventBridge.emitWebEvent(
        stringifiedData
    );
}


function listenForSettingsUpdates() {
    // console.log('GUI IS LISTENING FOR MESSAGES FROM INTERFACE' + window.innerWidth);
    EventBridge.scriptEventReceived.connect(function(data) {
        data = JSON.parse(data);

        if (data.messageType === 'object_update') {
            _.each(data.objectSettings, function(value, key) {
                //  settings[key] = value;
            });
        }

        if (data.messageType === 'initial_settings') {
            console.log('window.innerWidth initial settings incoming ')
                // console.log('INITIAL SETTINGS FROM INTERFACE:::' + JSON.stringify(data.initialSettings));
            _.each(data.initialSettings, function(value, key) {
                settings[key] = {};
                settings[key] = value;
            });

            loadGUI();
        }

    });

}

function manuallyUpdateDisplay() {
    // Iterate over all controllers
    // this is expensive, write a method for indiviudal controllers and use it when the value is different than a cached value, perhaps.
    var i;
    for (i in gui.__controllers) {
        gui.__controllers[i].updateDisplay();
    }
}

function setGUIWidthToWindowWidth() {
    if (gui !== null) {
        gui.width = window.innerWidth;
    }
}
function handleInputKeyPress(e) {
    if (e.keyCode === 13) {
        importSettings();
    }
    return false;
}

function importSettings() {
    var importInput = document.getElementById('importer-input');
    try {
        var importedSettings = JSON.parse(importInput.value);

        var keys = _.keys(importedSettings);
        _.each(keys, function(key) {
            var shouldIgnore = _.contains(keysToIgnore, key);
            if (shouldIgnore) {
                return;
            }
            settings[key] = importedSettings[key];
        });

        writeVec3ToInterface(settings);

        manuallyUpdateDisplay();
    } catch (e) {
        alert('Not properly formatted JSON');
    }

}

function prepareSettingsForExport() {
    var keys = _.keys(settings);

    var exportSettings = {};

    _.each(keys, function(key) {
        var shouldIgnore = _.contains(keysToIgnore, key);
        if (shouldIgnore) {
            return;
        }

        if (key.indexOf('color') > -1) {
            var colorObject = convertColorArrayToObject(settings[key]);
            settings[key] = colorObject
        }

        exportSettings[key] = settings[key];
    })
    return JSON.stringify(exportSettings);
}

function showPreselectedPrompt() {
    window.prompt("Ctrl-C to copy, then Enter.", prepareSettingsForExport());
}

function copyExportSettingsToClipboard() {
    var text = prepareSettingsForExport();
    var copyElement = document.createElement('input');
    copyElement.setAttribute('type', 'text');
    copyElement.setAttribute('value', text);
    copyElement = document.body.appendChild(copyElement);
    copyElement.select();
    try {
        document.execCommand('copy');
    } catch (e) {
        copyElement.remove();
        console.log("document.execCommand('copy'); is not supported");
        prompt('Copy the text below. (ctrl c, enter)', text);
    } finally {
        if (typeof e == 'undefined') {
            copyElement.remove();
        }
    }
    console.log('should have clipped text')
};

function removeContainerDomElement() {
    var elem = document.getElementById("my-gui-container");
    elem.parentNode.removeChild(elem);
}

function removeListenerFromGUI(key) {
    _.each(gui.__listening, function(controller, index) {
        if (controller.property === key) {
            storedController = controller;
            gui.__listening.splice(index, 1);
        }
    });
}

//the section below is to try to work at achieving two way bindings;
function addListenersBackToGUI(event) {
    gui.__listening.push(storedController);
    storedController = null;
}

function registerDOMElementsForListenerBlocking() {
    console.log('gui.__controllers length::: ' + gui.__controllers.length)

    _.each(gui.__controllers, function(controller) {
        var input = controller.domElement.childNodes[0];
        input.addEventListener('focus', function(event) {
            console.log('INPUT ELEMENT GOT FOCUS!' + controller.property);
            removeListenerFromGUI(controller.property);
        });
    })

    _.each(gui.__controllers, function(controller) {
        var input = controller.domElement.childNodes[0];
        input.addEventListener('blur', function(event) {
            console.log('INPUT ELEMENT GOT BLUR!' + controller.property);
            addListenersBackToGUI();
        });
    })

    // _.each(gui.__folders, function(folder) {
    //     _.each(folder.__controllers, function(controller) {
    //         var input = controller.__input;
    //         input.addEventListener('focus', function(event) {
    //             console.log('FOLDER ELEMENT GOT FOCUS!' + controller.property);
    //         });
    //     })
    // });
}

///utility method for converting weird collisionWillMove type propertyies from binary to new Boolean()
//
// function convertBinaryToBoolean(value) {
//     if (value === 0) {
//         return false;
//     }
//     return true;
// }