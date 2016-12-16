//
//  controllerDisplayManager.js
//
//  Created by Anthony J. Thibault on 10/20/16
//  Originally created by Ryan Huffman on 9/21/2016
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/* globals ControllerDisplayManager:true createControllerDisplay deleteControllerDisplay
   VIVE_CONTROLLER_CONFIGURATION_LEFT VIVE_CONTROLLER_CONFIGURATION_RIGHT */
/* eslint indent: ["error", 4, { "outerIIFEBody": 0 }] */

(function () {

Script.include("controllerDisplay.js");
Script.include("viveControllerConfiguration.js");
Script.include("touchControllerConfiguration.js?" + Date.now());

var HIDE_CONTROLLERS_ON_EQUIP = false;

HMD.requestShowHandControllers();

//
// Management of controller display
//
ControllerDisplayManager = function() {
    var self = this;
    var controllerLeft = null;
    var controllerRight = null;
    var controllerCheckerIntervalID = null;

    this.setLeftVisible = function(visible) {
        if (controllerLeft) {
            controllerLeft.setVisible(visible);
        }
    };

    this.setRightVisible = function(visible) {
        if (controllerRight) {
            controllerRight.setVisible(visible);
        }
    };

    function updateControllers() {
        if (HMD.active && HMD.shouldShowHandControllers()) {
            var leftConfig = null;
            var rightConfig = null;

            if ("Vive" in Controller.Hardware) {
                leftConfig = VIVE_CONTROLLER_CONFIGURATION_LEFT;
                rightConfig = VIVE_CONTROLLER_CONFIGURATION_RIGHT;
            }

            if ("OculusTouch" in Controller.Hardware) {
                leftConfig = TOUCH_CONTROLLER_CONFIGURATION_LEFT;
                rightConfig = TOUCH_CONTROLLER_CONFIGURATION_RIGHT;
            }

            if (leftConfig !== null && rightConfig !== null) {
                print("Loading controllers");
                if (controllerLeft === null) {
                    controllerLeft = createControllerDisplay(leftConfig);
                    controllerLeft.setVisible(true);
                }
                if (controllerRight === null) {
                    controllerRight = createControllerDisplay(rightConfig);
                    controllerRight.setVisible(true);
                }
                // We've found the controllers, we no longer need to look for active controllers
                if (controllerCheckerIntervalID) {
                    Script.clearInterval(controllerCheckerIntervalID);
                    controllerCheckerIntervalID = null;
                }
            } else {
                self.deleteControllerDisplays();
                if (!controllerCheckerIntervalID) {
                    controllerCheckerIntervalID = Script.setInterval(updateControllers, 1000);
                }
            }
        } else {
            // We aren't in HMD mode, we no longer need to look for active controllers
            if (controllerCheckerIntervalID) {
                Script.clearInterval(controllerCheckerIntervalID);
                controllerCheckerIntervalID = null;
            }
            self.deleteControllerDisplays();
        }
    }

    var handleMessages = function(channel, message, sender) {
        var i, data, name, visible;
        if (!controllerLeft && !controllerRight) {
            return;
        }

        if (sender === MyAvatar.sessionUUID) {
            if (channel === 'Controller-Display') {
                data = JSON.parse(message);
                name = data.name;
                visible = data.visible;
                if (controllerLeft) {
                    if (name in controllerLeft.annotations) {
                        for (i = 0; i < controllerLeft.annotations[name].length; ++i) {
                            Overlays.editOverlay(controllerLeft.annotations[name][i], { visible: visible });
                        }
                    }
                }
                if (controllerRight) {
                    if (name in controllerRight.annotations) {
                        for (i = 0; i < controllerRight.annotations[name].length; ++i) {
                            Overlays.editOverlay(controllerRight.annotations[name][i], { visible: visible });
                        }
                    }
                }
            } else if (channel === 'Controller-Display-Parts') {
                data = JSON.parse(message);
                for (name in data) {
                    visible = data[name];
                    if (controllerLeft) {
                        controllerLeft.setPartVisible(name, visible);
                    }
                    if (controllerRight) {
                        controllerRight.setPartVisible(name, visible);
                    }
                }
            } else if (channel === 'Controller-Set-Part-Layer') {
                data = JSON.parse(message);
                for (name in data) {
                    var layer = data[name];
                    if (controllerLeft) {
                        controllerLeft.setLayerForPart(name, layer);
                    }
                    if (controllerRight) {
                        controllerRight.setLayerForPart(name, layer);
                    }
                }
            } else if (channel === 'Hifi-Object-Manipulation') {
                if (HIDE_CONTROLLERS_ON_EQUIP) {
                    data = JSON.parse(message);
                    visible = data.action !== 'equip';
                    if (data.joint === "LeftHand") {
                        self.setLeftVisible(visible);
                    } else if (data.joint === "RightHand") {
                        self.setRightVisible(visible);
                    }
                }
            }
        }
    };

    Messages.messageReceived.connect(handleMessages);

    this.deleteControllerDisplays = function() {
        if (controllerLeft) {
            deleteControllerDisplay(controllerLeft);
            controllerLeft = null;
        }
        if (controllerRight) {
            deleteControllerDisplay(controllerRight);
            controllerRight = null;
        }
    };

    this.destroy = function() {
        Messages.messageReceived.disconnect(handleMessages);

        HMD.displayModeChanged.disconnect(updateControllers);
        HMD.shouldShowHandControllersChanged.disconnect(updateControllers);

        self.deleteControllerDisplays();
    };

    HMD.displayModeChanged.connect(updateControllers);
    HMD.shouldShowHandControllersChanged.connect(updateControllers);

    updateControllers();
};

var controllerDisplayManager = new ControllerDisplayManager();

Script.scriptEnding.connect(function () {
    controllerDisplayManager.destroy();
});

}());
