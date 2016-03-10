//
//  mouseLook.js
//  examples
//
//  Created by Sam Gondelman on 6/16/15
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var lastX = Reticle.getPosition().x;
var lastY = Reticle.getPosition().y;
var yawFromMouse = 0;
var pitchFromMouse = 0;

var yawSpeed = 0;
var pitchSpeed = 0;

var DEFAULT_PARAMETERS = {
    MOUSE_YAW_SCALE: -0.125,
    MOUSE_PITCH_SCALE: -0.125,
    MOUSE_SENSITIVITY: 0.5,

    // Damping frequency, adjust to change mouse look behavior
    W: 10,
}

var movementParameters = DEFAULT_PARAMETERS;

var mouseLook = (function () {

    var BUTTON_WIDTH = 50,
        BUTTON_HEIGHT = 50,
        BUTTON_ALPHA = 0.9,
        BUTTON_MARGIN = 8,
        active = false,
        keyboardID = 0;

    function onKeyPressEvent(event) {
        if (event.text == 'm') {
            active = !active;
            updateMapping();
        }
    }

    function findInput(name) {
        var availableInputs = Controller.getAvailableInputs(keyboardID);
        for (i = 0; i < availableInputs.length; i++) {
            if (availableInputs[i].inputName == name) {
                return availableInputs[i].input;
            }
        }
        // If the input isn't found, it will default to the first available input
        return availableInputs[0].input;
    }

    function findAction(name) {
        var actions = Controller.getAllActions();
        for (var i = 0; i < actions.length; i++) {
            if (actions[i].actionName == name) {
                return i;
            }
        }
        // If the action isn't found, it will default to the first available action
        return 0;
    }

    function updateMapping() {
        if (keyboardID != 0) {
            if (active) {
                // Turn mouselook on
                yawFromMouse = 0;
                pitchFromMouse = 0;
                yawSpeed = 0;
                pitchSpeed = 0;

                var a = findInput("A").channel;
                var d = findInput("D").channel;
                var left = findInput("Left").channel;
                var right = findInput("Right").channel;
                var shift = findInput("Shift").channel;

                for (var i = findAction("YAW_LEFT"); i <= findAction("YAW_RIGHT"); i++) {
                    var inputChannels = Controller.getAllActions()[i].inputChannels;
                    for (var j = 0; j < inputChannels.length; j++) {
                        var inputChannel = inputChannels[j];
                        // make a, d, left, and right strafe
                        if ((inputChannel.input.channel == a || inputChannel.input.channel == left) && inputChannel.modifier.device == 0) {
                            Controller.removeInputChannel(inputChannel);
                            inputChannel.action = findAction("LATERAL_LEFT");
                            Controller.addInputChannel(inputChannel);
                        } else if ((inputChannel.input.channel == d || inputChannel.input.channel == right) && inputChannel.modifier.device == 0) {
                            Controller.removeInputChannel(inputChannel);
                            inputChannel.action = findAction("LATERAL_RIGHT");
                            Controller.addInputChannel(inputChannel);
                        }
                    }
                }
                for (var i = findAction("LATERAL_LEFT"); i <= findAction("LATERAL_RIGHT"); i++) {
                    var inputChannels = Controller.getAllActions()[i].inputChannels;
                    for (var j = 0; j < inputChannels.length; j++) {
                        var inputChannel = inputChannels[j];
                        // make shift + a/d/left/right change yaw/pitch
                        if ((inputChannel.input.channel == a || inputChannel.input.channel == left) && inputChannel.modifier.channel == shift) {
                            Controller.removeInputChannel(inputChannel);
                            inputChannel.action = findAction("YAW_LEFT");
                            Controller.addInputChannel(inputChannel);
                        } else if ((inputChannel.input.channel == d || inputChannel.input.channel == right) && inputChannel.modifier.channel == shift) {
                            Controller.removeInputChannel(inputChannel);
                            inputChannel.action = findAction("YAW_RIGHT");
                            Controller.addInputChannel(inputChannel);
                        }
                    }
                }
            } else {
                Controller.resetDevice(keyboardID);
            }
        }
    }

    function onScriptUpdate(dt) {
        if (active && Window.hasFocus()) {
            var x = Reticle.getPosition().x;
            // I'm not sure why this + 0.5 is necessary?
            var y = Reticle.getPosition().y; + 0.5;

            yawFromMouse += ((x - lastX) * movementParameters.MOUSE_YAW_SCALE * movementParameters.MOUSE_SENSITIVITY);
            pitchFromMouse += ((y - lastY) * movementParameters.MOUSE_PITCH_SCALE * movementParameters.MOUSE_SENSITIVITY);
            pitchFromMouse = Math.max(-180, Math.min(180, pitchFromMouse));

            resetCursorPosition();

            // Here we use a linear damping model - http://en.wikipedia.org/wiki/Damping#Linear_damping
            // Because we are using a critically damped model (no oscillation), ζ = 1 and
            // so we derive the formula: acceleration = -(2 * w0 * v) - (w0^2 * x)
            var W = movementParameters.W;
            yawAccel = (W * W * yawFromMouse) - (2 * W * yawSpeed);
            pitchAccel = (W * W * pitchFromMouse) - (2 * W * pitchSpeed);

            yawSpeed += yawAccel * dt;
            var yawMove = yawSpeed * dt;
            var newOrientation = Quat.multiply(MyAvatar.orientation, Quat.fromVec3Degrees( { x: 0, y: yawMove, z: 0 } ));
            MyAvatar.orientation = newOrientation;
            yawFromMouse -= yawMove;

            pitchSpeed += pitchAccel * dt;
            var pitchMove = pitchSpeed * dt;
            var newPitch = MyAvatar.headPitch + pitchMove;
            MyAvatar.headPitch = newPitch;
            pitchFromMouse -= pitchMove;
        }
    }

    function resetCursorPosition() {
        var newX = Window.x + Window.innerWidth / 2.0;
        var newY = Window.y + Window.innerHeight / 2.0;
        Reticle.setPosition({ x: newX, y: newY});
        lastX = newX;
        lastY = newY;
    }

    function setUp() {
        keyboardID = Controller.findDevice("Keyboard");

        Controller.keyPressEvent.connect(onKeyPressEvent);

        Menu.menuItemEvent.connect(handleMenuEvent);

        Script.update.connect(onScriptUpdate);
    }

    function setupMenu() {
        Menu.addMenuItem({ menuName: "View", menuItemName: "Mouselook Mode", shortcutKey: "SHIFT+M", 
                        afterItem: "Mirror", isCheckable: true, isChecked: false });
    }

    setupMenu();

    function cleanupMenu() {
        Menu.removeMenuItem("View", "Mouselook Mode");
    }

    function handleMenuEvent(menuItem) {
        if (menuItem == "Mouselook Mode") {
            active = !active;
            updateMapping();
        }
    }

    function tearDown() {
        if (keyboardID != 0) {
               Controller.resetDevice(keyboardID);
        }
        cleanupMenu();
    }

    setUp();
    Script.scriptEnding.connect(tearDown);
}());