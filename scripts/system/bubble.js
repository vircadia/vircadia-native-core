"use strict";

//
//  bubble.js
//  scripts/system/
//
//  Created by Brad Hefta-Gaub on 11/18/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
/* global Toolbars, Script, Users, Overlays, AvatarList, Controller, Camera, getControllerWorldLocation */


(function () { // BEGIN LOCAL_SCOPE

    // grab the toolbar
    var toolbar = Toolbars.getToolbar("com.highfidelity.interface.toolbar.system");
    // Used for animating and disappearing the bubble
    var bubbleOverlayTimestamp;
    // Used for flashing the HUD button upon activation
    var bubbleButtonFlashState = false;
    // Used for flashing the HUD button upon activation
    var bubbleButtonTimestamp;
    // Affects bubble height
    const BUBBLE_HEIGHT_SCALE = 0.15;
    // The bubble model itself
    var bubbleOverlay = Overlays.addOverlay("model", {
        url: Script.resolvePath("assets/models/bubble-v12.fbx"), // If you'd like to change the model, modify this line (and the dimensions below)
        dimensions: { x: 1.0, y: 0.75, z: 1.0 },
        position: { x: MyAvatar.position.x, y: -MyAvatar.scale * 2 + MyAvatar.position.y + MyAvatar.scale * BUBBLE_HEIGHT_SCALE, z: MyAvatar.position.z },
        rotation: Quat.fromPitchYawRollDegrees(MyAvatar.bodyPitch, 0, MyAvatar.bodyRoll),
        scale: { x: 2, y: MyAvatar.scale * 0.5 + 0.5, z: 2 },
        visible: false,
        ignoreRayIntersection: true
    });
    // The bubble activation sound
    var bubbleActivateSound = SoundCache.getSound(Script.resolvePath("assets/sounds/bubble.wav"));
    // Is the update() function connected?
    var updateConnected = false;

    const BUBBLE_VISIBLE_DURATION_MS = 3000;
    const BUBBLE_RAISE_ANIMATION_DURATION_MS = 750;
    const BUBBLE_HUD_ICON_FLASH_INTERVAL_MS = 500;

    var ASSETS_PATH = Script.resolvePath("assets");
    var TOOLS_PATH = Script.resolvePath("assets/images/tools/");

    function buttonImageURL() {
        return TOOLS_PATH + 'bubble.svg';
    }

    // Hides the bubble model overlay and resets the button flash state
    function hideOverlays() {
        Overlays.editOverlay(bubbleOverlay, {
            visible: false
        });
        bubbleButtonFlashState = false;
    }

    // Make the bubble overlay visible, set its position, and play the sound
    function createOverlays() {
        Audio.playSound(bubbleActivateSound, {
            position: { x: MyAvatar.position.x, y: MyAvatar.position.y, z: MyAvatar.position.z },
            localOnly: true,
            volume: 0.4
        });
        hideOverlays();
        if (updateConnected === true) {
            updateConnected = false;
            Script.update.disconnect(update);
        }

        Overlays.editOverlay(bubbleOverlay, {
            position: { x: MyAvatar.position.x, y: -MyAvatar.scale * 2 + MyAvatar.position.y + MyAvatar.scale * BUBBLE_HEIGHT_SCALE, z: MyAvatar.position.z },
            rotation: Quat.fromPitchYawRollDegrees(MyAvatar.bodyPitch, 0, MyAvatar.bodyRoll),
            scale: { x: 2, y: MyAvatar.scale * 0.5 + 0.5, z: 2 },
            visible: true
        });
        bubbleOverlayTimestamp = Date.now();
        bubbleButtonTimestamp = bubbleOverlayTimestamp;
        Script.update.connect(update);
        updateConnected = true;
    }

    // Called from the C++ scripting interface to show the bubble overlay
    function enteredIgnoreRadius() {
        createOverlays();
    }

    // Used to set the state of the bubble HUD button
    function writeButtonProperties(parameter) {
        button.writeProperty('buttonState', parameter ? 0 : 1);
        button.writeProperty('defaultState', parameter ? 0 : 1);
        button.writeProperty('hoverState', parameter ? 2 : 3);
    }

    // The bubble script's update function
    update = function () {
        var timestamp = Date.now();
        var delay = (timestamp - bubbleOverlayTimestamp);
        var overlayAlpha = 1.0 - (delay / BUBBLE_VISIBLE_DURATION_MS);
        if (overlayAlpha > 0) {
            // Flash button
            if ((timestamp - bubbleButtonTimestamp) >= BUBBLE_VISIBLE_DURATION_MS) {
                writeButtonProperties(bubbleButtonFlashState);
                bubbleButtonTimestamp = timestamp;
                bubbleButtonFlashState = !bubbleButtonFlashState;
            }

            if (delay < BUBBLE_RAISE_ANIMATION_DURATION_MS) {
                Overlays.editOverlay(bubbleOverlay, {
                    // Quickly raise the bubble from the ground up
                    position: {
                        x: MyAvatar.position.x,
                        y: (-((BUBBLE_RAISE_ANIMATION_DURATION_MS - delay) / BUBBLE_RAISE_ANIMATION_DURATION_MS)) * MyAvatar.scale * 2 + MyAvatar.position.y + MyAvatar.scale * BUBBLE_HEIGHT_SCALE,
                        z: MyAvatar.position.z
                    },
                    rotation: Quat.fromPitchYawRollDegrees(MyAvatar.bodyPitch, 0, MyAvatar.bodyRoll),
                    scale: {
                        x: 2,
                        y: ((1 - ((BUBBLE_RAISE_ANIMATION_DURATION_MS - delay) / BUBBLE_RAISE_ANIMATION_DURATION_MS)) * MyAvatar.scale * 0.5 + 0.5),
                        z: 2
                    }
                });
            } else {
                // Keep the bubble in place for a couple seconds
                Overlays.editOverlay(bubbleOverlay, {
                    position: {
                        x: MyAvatar.position.x,
                        y: MyAvatar.position.y + MyAvatar.scale * BUBBLE_HEIGHT_SCALE,
                        z: MyAvatar.position.z
                    },
                    rotation: Quat.fromPitchYawRollDegrees(MyAvatar.bodyPitch, 0, MyAvatar.bodyRoll),
                    scale: {
                        x: 2,
                        y: MyAvatar.scale * 0.5 + 0.5,
                        z: 2
                    }
                });
            }
        } else {
            hideOverlays();
            if (updateConnected === true) {
                Script.update.disconnect(update);
                updateConnected = false;
            }
            var bubbleActive = Users.getIgnoreRadiusEnabled();
            writeButtonProperties(bubbleActive);
        }
    };

    // When the space bubble is toggled...
    function onBubbleToggled() {
        var bubbleActive = Users.getIgnoreRadiusEnabled();
        writeButtonProperties(bubbleActive);
        if (bubbleActive) {
            createOverlays();
        } else {
            hideOverlays();
            if (updateConnected === true) {
                Script.update.disconnect(update);
                updateConnected = false;
            }
        }
    }

    // Setup the bubble button and add it to the toolbar
    var button = toolbar.addButton({
        objectName: 'bubble',
        imageURL: buttonImageURL(),
        visible: true,
        alpha: 0.9
    });
    onBubbleToggled();

    button.clicked.connect(Users.toggleIgnoreRadius);
    Users.ignoreRadiusEnabledChanged.connect(onBubbleToggled);
    Users.enteredIgnoreRadius.connect(enteredIgnoreRadius);

    // Cleanup the toolbar button and overlays when script is stopped
    Script.scriptEnding.connect(function () {
        toolbar.removeButton('bubble');
        button.clicked.disconnect(Users.toggleIgnoreRadius);
        Users.ignoreRadiusEnabledChanged.disconnect(onBubbleToggled);
        Users.enteredIgnoreRadius.disconnect(enteredIgnoreRadius);
        Overlays.deleteOverlay(bubbleOverlay);
        bubbleButtonFlashState = false;
        if (updateConnected === true) {
            Script.update.disconnect(update);
        }
    });

}()); // END LOCAL_SCOPE
