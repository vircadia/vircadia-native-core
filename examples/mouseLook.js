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

HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";

var lastX = Window.getCursorPositionX();
var lastY = Window.getCursorPositionY();
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

    var MOUSELOOK_BUTTON_URL = HIFI_PUBLIC_BUCKET + "images/tools/directory.svg",
        CURSORMODE_BUTTON_URL = HIFI_PUBLIC_BUCKET + "images/tools/marketplace.svg",
        BUTTON_WIDTH = 50,
        BUTTON_HEIGHT = 50,
        BUTTON_ALPHA = 0.9,
        BUTTON_MARGIN = 8,
        mouseLookButton,
        EDIT_TOOLBAR_BUTTONS = 10,  // Number of buttons in edit.js toolbar
        viewport,
        active = false,
        keyboardID = 0;

    function updateButtonPosition() {
        Overlays.editOverlay(mouseLookButton, {
            x: viewport.x - BUTTON_WIDTH - BUTTON_MARGIN,
            y: viewport.y - BUTTON_HEIGHT - BUTTON_MARGIN
        });
    }

    function onKeyPressEvent(event) {
    	if (event.text == 'm' && event.isMeta) {
    		active = !active;
    		updateMapping();
            Overlays.editOverlay(mouseLookButton, {
                imageURL: active ? MOUSELOOK_BUTTON_URL : CURSORMODE_BUTTON_URL
            });
    	}
    }

    function updateMapping() {
    	if (keyboardID != 0) {
    		if (active) {
                // Turn mouselook on
                yawFromMouse = 0;
                pitchFromMouse = 0;
                yawSpeed = 0;
                pitchSpeed = 0;

                // remove right click modifier from mouse pitch and yaw
                var rightClick = Controller.getAvailableInputs(keyboardID)[46].input.channel;
                var a = Controller.getAvailableInputs(keyboardID)[10].input.channel;
                var d = Controller.getAvailableInputs(keyboardID)[13].input.channel;
                var left = Controller.getAvailableInputs(keyboardID)[36].input.channel;
                var right = Controller.getAvailableInputs(keyboardID)[38].input.channel;
                var shift = Controller.getAvailableInputs(keyboardID)[41].input.channel;

                for (i = 6; i <= 9; i++) {
                    var inputChannels = Controller.getAllActions()[i].inputChannels;
                    for (j = 0; j < inputChannels.length; j++) {
                        var inputChannel = inputChannels[j];

                        // Get rid of right click modifier on pitch and yaw
                        // if (inputChannel.modifier.device == keyboardID &&
                        //     inputChannel.modifier.channel == rightClick) {
                        //     Controller.removeInputChannel(inputChannel);
                        //     inputChannel.modifier.device = 0;
                        //     inputChannel.modifier.channel = 0;
                        //     inputChannel.modifier.type = 0;
                        //     inputChannel.modifier.id = 0;
                        //     Controller.addInputChannel(inputChannel);
                        // }

                        // make a, d, left, and right strafe
                        if ((inputChannel.input.channel == a || inputChannel.input.channel == left) && inputChannel.modifier.device == 0) {
                            Controller.removeInputChannel(inputChannel);
                            inputChannel.action = 2;
                            Controller.addInputChannel(inputChannel);
                        } else if ((inputChannel.input.channel == d || inputChannel.input.channel == right) && inputChannel.modifier.device == 0) {
                            Controller.removeInputChannel(inputChannel);
                            inputChannel.action = 3;
                            Controller.addInputChannel(inputChannel);
                        }
                    }
                }
                // make shift + a/d/left/right change yaw/pitch
                for (i = 2; i <= 3; i++) {
                    var inputChannels = Controller.getAllActions()[i].inputChannels;
                    for (j = 0; j < inputChannels.length; j++) {
                        var inputChannel = inputChannels[j];
                        if ((inputChannel.input.channel == a || inputChannel.input.channel == left) && inputChannel.modifier.channel == shift) {
                            Controller.removeInputChannel(inputChannel);
                            inputChannel.action = 6;
                            Controller.addInputChannel(inputChannel);
                        } else if ((inputChannel.input.channel == d || inputChannel.input.channel == right) && inputChannel.modifier.channel == shift) {
                            Controller.removeInputChannel(inputChannel);
                            inputChannel.action = 7;
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
        var oldViewport = viewport;

        viewport = Controller.getViewportDimensions();

        if (viewport.x !== oldViewport.x || viewport.y !== oldViewport.y) {
            updateButtonPosition();
        }

        if (active && Window.hasFocus()) {
            var x = Window.getCursorPositionX();
            // I'm not sure why this + 0.5 is necessary?
            var y = Window.getCursorPositionY() + 0.5;

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
        var newX = Window.x + Window.innerWidth / 2;
        var newY = Window.y + Window.innerHeight / 2;
        Window.setCursorPosition(newX, newY);
        lastX = newX;
        lastY = newY;
    }

    function setUp() {
        viewport = Controller.getViewportDimensions();

        mouseLookButton = Overlays.addOverlay("image", {
            imageURL: CURSORMODE_BUTTON_URL,
            width: BUTTON_WIDTH,
            height: BUTTON_HEIGHT,
            x: viewport.x - BUTTON_WIDTH - BUTTON_MARGIN,
            y: viewport.y - BUTTON_MARGIN - BUTTON_HEIGHT,
            alpha: BUTTON_ALPHA,
            visible: true
        });

        updateButtonPosition();

        keyboardID = Controller.findDevice("Keyboard");

        Controller.keyPressEvent.connect(onKeyPressEvent);

        Menu.menuItemEvent.connect(handleMenuEvent);

        Script.update.connect(onScriptUpdate);
    }

    function setupMenu() {
        Menu.addMenuItem({ menuName: "View", menuItemName: "Mouselook Mode", shortcutKey: "META+M", 
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
            Overlays.editOverlay(mouseLookButton, {
                imageURL: active ? MOUSELOOK_BUTTON_URL : CURSORMODE_BUTTON_URL
            });
        }
    }

    function tearDown() {
        Overlays.deleteOverlay(mouseLookButton);
        if (keyboardID != 0) {
       		Controller.resetDevice(keyboardID);
        }
        cleanupMenu();
    }

    setUp();
    Script.scriptEnding.connect(tearDown);
}());