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

/* jslint bitwise: true */

/* global Script, Quat, MyAvatar, HMD, Controller, Messages*/

(function() { // BEGIN LOCAL_SCOPE

    var TWO_SECONDS_INTERVAL = 2000;
    var FLYING_MAPPING_NAME = 'Hifi-Flying-Dev-' + Math.random();
    var DRIVING_MAPPING_NAME = 'Hifi-Driving-Dev-' + Math.random();

    var flyingMapping = null;
    var drivingMapping = null;

    var TURN_RATE = 1000;
    var isDisabled = false;

    var previousFlyingState = MyAvatar.getFlyingEnabled();
    var previousDrivingState = false;

    function rotate180() {
        var newOrientation = Quat.multiply(MyAvatar.orientation, Quat.angleAxis(180, {
            x: 0,
            y: 1,
            z: 0
        }));
        MyAvatar.orientation = newOrientation;
    }

    var inFlipTurn = false;

    function registerBasicMapping() {

        drivingMapping = Controller.newMapping(DRIVING_MAPPING_NAME);
        drivingMapping.from(Controller.Standard.LY).to(function(value) {
            if (isDisabled) {
                return;
            }

            if (value === 1 && Controller.Hardware.OculusTouch !== undefined) {
                rotate180();
            } else if (Controller.Hardware.Vive !== undefined) {
                if (value > 0.75 && inFlipTurn === false) {
                    inFlipTurn = true;
                    rotate180();
                    Script.setTimeout(function() {
                        inFlipTurn = false;
                    }, TURN_RATE);
                }
            }
            return;
        });

        flyingMapping = Controller.newMapping(FLYING_MAPPING_NAME);
        flyingMapping.from(Controller.Standard.RY).to(function(value) {
            if (isDisabled) {
                return;
            }

            if (value === 1 && Controller.Hardware.OculusTouch !== undefined) {
                rotate180();
            } else if (Controller.Hardware.Vive !== undefined) {
                if (value > 0.75 && inFlipTurn === false) {
                    inFlipTurn = true;
                    rotate180();
                    Script.setTimeout(function() {
                        inFlipTurn = false;
                    }, TURN_RATE);
                }
            }
            return;
        });
    }

    function scriptEnding() {
        Controller.disableMapping(FLYING_MAPPING_NAME);
        Controller.disableMapping(DRIVING_MAPPING_NAME);
    }

    Script.scriptEnding.connect(scriptEnding);

    registerBasicMapping();

    Script.setTimeout(function() {
        if (MyAvatar.useAdvanceMovementControls) {
            Controller.disableMapping(DRIVING_MAPPING_NAME);
        } else {
            Controller.enableMapping(DRIVING_MAPPING_NAME);
        }

        if (MyAvatar.getFlyingEnabled()) {
            Controller.disableMapping(FLYING_MAPPING_NAME);
        } else {
            Controller.enableMapping(FLYING_MAPPING_NAME);
        }
    }, 100);


    HMD.displayModeChanged.connect(function(isHMDMode) {
        if (isHMDMode) {
            if (Controller.Hardware.Vive !== undefined || Controller.Hardware.OculusTouch !== undefined) {
                if (MyAvatar.useAdvancedMovementControls) {
                    Controller.disableMapping(DRIVING_MAPPING_NAME);
                } else {
                    Controller.enableMapping(DRIVING_MAPPING_NAME);
                }

                if (MyAvatar.getFlyingEnabled()) {
                    Controller.disableMapping(FLYING_MAPPING_NAME);
                } else {
                    Controller.enableMapping(FLYING_MAPPING_NAME);
                }

            }
        }
    });


    function update() {
        if ((Controller.Hardware.Vive !== undefined || Controller.Hardware.OculusTouch !== undefined) && HMD.active) {
            var flying = MyAvatar.getFlyingEnabled();
            var driving = MyAvatar.useAdvancedMovementControls;

            if (flying !== previousFlyingState) {
                if (flying) {
                    Controller.disableMapping(FLYING_MAPPING_NAME);
                } else {
                    Controller.enableMapping(FLYING_MAPPING_NAME);
                }

                previousFlyingState = flying;
            }

            if (driving !== previousDrivingState) {
                if (driving) {
                    Controller.disableMapping(DRIVING_MAPPING_NAME);
                } else {
                    Controller.enableMapping(DRIVING_MAPPING_NAME);
                }
                previousDrivingState = driving;
            }
        }
        Script.setTimeout(update, TWO_SECONDS_INTERVAL);
    }

    Script.setTimeout(update, TWO_SECONDS_INTERVAL);

    var HIFI_ADVANCED_MOVEMENT_DISABLER_CHANNEL = 'Hifi-Advanced-Movement-Disabler';
    function handleMessage(channel, message, sender) {
        if (channel === HIFI_ADVANCED_MOVEMENT_DISABLER_CHANNEL) {
            if (message === 'disable') {
                isDisabled = true;
            } else if (message === 'enable') {
                isDisabled = false;
            }
        }
    }

    Messages.subscribe(HIFI_ADVANCED_MOVEMENT_DISABLER_CHANNEL);
    Messages.messageReceived.connect(handleMessage);

}()); // END LOCAL_SCOPE
