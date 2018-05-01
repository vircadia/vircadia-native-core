"use strict";

// Created by james b. pollack @imgntn on 8/18/2016
// Copyright 2016 High Fidelity, Inc.
//
// advanced movements settings are in individual controller json files
// what we do is check the status of the 'advance movement' checkbox when you enter HMD mode
// if 'advanced movement' is checked...we give you the defaults that are in the json.
// if 'advanced movement' is not checked... we override the advanced controls with basic ones.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

(function() { // BEGIN LOCAL_SCOPE

var mappingName, basicMapping, isChecked;

var TURN_RATE = 1000;
var isDisabled = false;
var previousSetting = MyAvatar.useAdvancedMovementControls;
if (previousSetting === false) {
    previousSetting = false;
    isChecked = false;
}

if (previousSetting === true) {
    previousSetting = true;
    isChecked = true;
}

function rotate180() {
    var newOrientation = Quat.multiply(MyAvatar.orientation, Quat.angleAxis(180, {
        x: 0,
        y: 1,
        z: 0
    }))
    MyAvatar.orientation = newOrientation
}

var inFlipTurn = false;

function registerBasicMapping() {
    mappingName = 'Hifi-AdvancedMovement-Dev-' + Math.random();
    basicMapping = Controller.newMapping(mappingName);
    basicMapping.from(Controller.Standard.LY).to(function(value) {
        if (isDisabled) {
            return;
        }
        var stick = Controller.getValue(Controller.Standard.LS);
        if (value === 1 && Controller.Hardware.OculusTouch !== undefined) {
            rotate180();
        } else if (Controller.Hardware.Vive !== undefined) {
            if (value > 0.75 && inFlipTurn === false) {
                inFlipTurn = true;
                rotate180();
                Script.setTimeout(function() {
                    inFlipTurn = false;
                }, TURN_RATE)
            }
        }
        return;
    });
    basicMapping.from(Controller.Standard.RY).to(function(value) {
        if (isDisabled) {
            return;
        }
        var stick = Controller.getValue(Controller.Standard.RS);
        if (value === 1 && Controller.Hardware.OculusTouch !== undefined) {
            rotate180();
        } else if (Controller.Hardware.Vive !== undefined) {
            if (value > 0.75 && inFlipTurn === false) {
                inFlipTurn = true;
                rotate180();
                Script.setTimeout(function() {
                    inFlipTurn = false;
                }, TURN_RATE)
            }
        }
        return;
    })
}


function enableMappings() {
    Controller.enableMapping(mappingName);
}

function disableMappings() {
    Controller.disableMapping(mappingName);
}

function scriptEnding() {
    disableMappings();
}

Script.scriptEnding.connect(scriptEnding);

registerBasicMapping();

Script.setTimeout(function() {
    if (previousSetting === true) {
        disableMappings();
    } else {
        enableMappings();
    }
}, 100)


HMD.displayModeChanged.connect(function(isHMDMode) {
    if (isHMDMode) {
        if (Controller.Hardware.Vive !== undefined || Controller.Hardware.OculusTouch !== undefined) {
            if (isChecked === true) {
                disableMappings();
            } else if (isChecked === false) {
                enableMappings();
            }

        }
    }
});


var HIFI_ADVANCED_MOVEMENT_DISABLER_CHANNEL = 'Hifi-Advanced-Movement-Disabler';
function handleMessage(channel, message, sender) {
    if (channel === HIFI_ADVANCED_MOVEMENT_DISABLER_CHANNEL) {
        if (message === 'disable') {
            isDisabled = true;
        } else if (message === 'enable') {
            isDisabled = false;
        } else if (message === 'enable_mappings') {
            print("enable mappings")
            enableMappings();
        } else if (message === 'disable_mappings') {
            disableMappings();
        }
    }
}

Messages.subscribe(HIFI_ADVANCED_MOVEMENT_DISABLER_CHANNEL);
Messages.messageReceived.connect(handleMessage);

}()); // END LOCAL_SCOPE
