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
/* global Script, Users, Overlays, AvatarList, Controller, Camera, getControllerWorldLocation, UserActivityLogger */

(function () { // BEGIN LOCAL_SCOPE
    var button;
    // Used for animating and disappearing the bubble
    var bubbleOverlayTimestamp;
    // Used for rate limiting the bubble sound
    var lastBubbleSoundTimestamp = 0;
    // Affects bubble height
    var BUBBLE_HEIGHT_SCALE = 0.15;
    // The bubble model itself
    var bubbleOverlay = Overlays.addOverlay("model", {
        url: Script.resolvePath("assets/models/Bubble-v14.fbx"), // If you'd like to change the model, modify this line (and the dimensions below)
        dimensions: { x: MyAvatar.sensorToWorldScale, y: 0.75 * MyAvatar.sensorToWorldScale, z: MyAvatar.sensorToWorldScale },
        position: { x: MyAvatar.position.x, y: -MyAvatar.scale * 2 + MyAvatar.position.y + MyAvatar.scale * BUBBLE_HEIGHT_SCALE, z: MyAvatar.position.z },
        rotation: Quat.multiply(MyAvatar.orientation, Quat.fromVec3Degrees({x: 0.0, y: 180.0, z: 0.0})),
        scale: { x: 2 , y: MyAvatar.scale * 0.5 + 0.5, z: 2  },
        visible: false,
        ignoreRayIntersection: true
    });
    // The bubble activation sound
    var bubbleActivateSound = SoundCache.getSound(Script.resolvePath("assets/sounds/bubble.wav"));
    // Is the update() function connected?
    var updateConnected = false;

    var BUBBLE_VISIBLE_DURATION_MS = 3000;
    var BUBBLE_RAISE_ANIMATION_DURATION_MS = 750;
    var BUBBLE_SOUND_RATE_LIMIT_MS = 15000;

    // Hides the bubble model overlay
    function hideOverlays() {
        Overlays.editOverlay(bubbleOverlay, {
            visible: false
        });
    }

    //create a menu item in "Setings" to toggle the bubble/shield HUD button 
    var menuItemName = "HUD Shield Button";
    Menu.addMenuItem({
        menuName: "Settings",
        menuItemName: menuItemName,
        isCheckable: true,
        isChecked: AvatarInputs.showBubbleTools
    });
    Menu.menuItemEvent.connect(onToggleHudShieldButton);
    AvatarInputs.showBubbleToolsChanged.connect(showBubbleToolsChanged);

    function onToggleHudShieldButton(menuItem) {
        if (menuItem === menuItemName) {
            AvatarInputs.setShowBubbleTools(Menu.isOptionChecked(menuItem));
        };
    }

    function showBubbleToolsChanged(show) {
        Menu.setIsOptionChecked(menuItemName, show);
    }

    // Make the bubble overlay visible, set its position, and play the sound
    function createOverlays() {
        var nowTimestamp = Date.now();
        if (nowTimestamp - lastBubbleSoundTimestamp >= BUBBLE_SOUND_RATE_LIMIT_MS) {
            Audio.playSound(bubbleActivateSound, {
                position: { x: MyAvatar.position.x, y: MyAvatar.position.y, z: MyAvatar.position.z },
                localOnly: true,
                volume: 0.2
            });
            lastBubbleSoundTimestamp = nowTimestamp;
        }
        hideOverlays();
        if (updateConnected === true) {
            updateConnected = false;
            Script.update.disconnect(update);
        }

        Overlays.editOverlay(bubbleOverlay, {
            dimensions: { 
                x: MyAvatar.sensorToWorldScale, 
                y: 0.75 * MyAvatar.sensorToWorldScale, 
                z: MyAvatar.sensorToWorldScale 
            },
            position: { 
                x: MyAvatar.position.x, 
                y: -MyAvatar.scale * 2 + MyAvatar.position.y + MyAvatar.scale * BUBBLE_HEIGHT_SCALE, 
                z: MyAvatar.position.z 
            },
            rotation: Quat.multiply(MyAvatar.orientation, Quat.fromVec3Degrees({x: 0.0, y: 180.0, z: 0.0})),
            scale: { 
                x: 2 , 
                y: MyAvatar.scale * 0.5  + 0.5 , 
                z: 2  
            },
            visible: true
        });
        bubbleOverlayTimestamp = nowTimestamp;
        Script.update.connect(update);
        updateConnected = true;
    }

    // Called from the C++ scripting interface to show the bubble overlay
    function enteredIgnoreRadius() {
        createOverlays();
        UserActivityLogger.privacyShieldActivated();
    }

    // Used to set the state of the bubble HUD button
    function writeButtonProperties(parameter) {
        button.editProperties({isActive: parameter});
    }

    // The bubble script's update function
    function update() {
        var timestamp = Date.now();
        var delay = (timestamp - bubbleOverlayTimestamp);
        var overlayAlpha = 1.0 - (delay / BUBBLE_VISIBLE_DURATION_MS);
        if (overlayAlpha > 0) {
            if (delay < BUBBLE_RAISE_ANIMATION_DURATION_MS) {
                Overlays.editOverlay(bubbleOverlay, {
                    dimensions: { 
                        x: MyAvatar.sensorToWorldScale, 
                        y: 0.75 * MyAvatar.sensorToWorldScale, 
                        z: MyAvatar.sensorToWorldScale 
                    },
                    // Quickly raise the bubble from the ground up
                    position: {
                        x: MyAvatar.position.x,
                        y: (-((BUBBLE_RAISE_ANIMATION_DURATION_MS - delay) / BUBBLE_RAISE_ANIMATION_DURATION_MS)) * MyAvatar.scale * 2 + MyAvatar.position.y + MyAvatar.scale * BUBBLE_HEIGHT_SCALE,
                        z: MyAvatar.position.z
                    },
                    rotation: Quat.multiply(MyAvatar.orientation, Quat.fromVec3Degrees({x: 0.0, y: 180.0, z: 0.0})),
                    scale: {
                        x: 2 ,
                        y: ((1 - ((BUBBLE_RAISE_ANIMATION_DURATION_MS - delay) / BUBBLE_RAISE_ANIMATION_DURATION_MS)) * MyAvatar.scale * 0.5 + 0.5),
                        z: 2 
                    }
                });
            } else {
                // Keep the bubble in place for a couple seconds
                Overlays.editOverlay(bubbleOverlay, {
                    dimensions: { 
                        x: MyAvatar.sensorToWorldScale, 
                        y: 0.75 * MyAvatar.sensorToWorldScale, 
                        z: MyAvatar.sensorToWorldScale 
                    },            
                    position: {
                        x: MyAvatar.position.x,
                        y: MyAvatar.position.y + MyAvatar.scale * BUBBLE_HEIGHT_SCALE,
                        z: MyAvatar.position.z
                    },
                    rotation: Quat.multiply(MyAvatar.orientation, Quat.fromVec3Degrees({x: 0.0, y: 180.0, z: 0.0})),
                    scale: {
                        x: 2,
                        y: MyAvatar.scale * 0.5  + 0.5 ,
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
        }
    }

    // When the space bubble is toggled...
    // NOTE: the c++ calls this with just the first param -- we added a second
    // just for not logging the initial state of the bubble when we startup.
    function onBubbleToggled(enabled, doNotLog) {
        writeButtonProperties(enabled);
        if (doNotLog !== true) {
            UserActivityLogger.privacyShieldToggled(enabled);
        }
        if (enabled) {
            createOverlays();
        } else {
            hideOverlays();
            if (updateConnected === true) {
                Script.update.disconnect(update);
                updateConnected = false;
            }
        }
    }

    // Setup the bubble button
    var buttonName = "SHIELD";
    var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
    button = tablet.addButton({
        icon: "icons/tablet-icons/bubble-i.svg",
        activeIcon: "icons/tablet-icons/bubble-a.svg",
        text: buttonName,
        sortOrder: 4
    });

    onBubbleToggled(Users.getIgnoreRadiusEnabled(), true); // pass in true so we don't log this initial one in the UserActivity table

    button.clicked.connect(Users.toggleIgnoreRadius);
    Users.ignoreRadiusEnabledChanged.connect(onBubbleToggled);
    Users.enteredIgnoreRadius.connect(enteredIgnoreRadius);

    // Cleanup the tablet button and overlays when script is stopped
    Script.scriptEnding.connect(function () {
        Menu.menuItemEvent.disconnect(onToggleHudShieldButton);
        AvatarInputs.showBubbleToolsChanged.disconnect(showBubbleToolsChanged);
        Menu.removeMenuItem("Settings", menuItemName);
        button.clicked.disconnect(Users.toggleIgnoreRadius);
        if (tablet) {
            tablet.removeButton(button);
        }
        Users.ignoreRadiusEnabledChanged.disconnect(onBubbleToggled);
        Users.enteredIgnoreRadius.disconnect(enteredIgnoreRadius);
        Overlays.deleteOverlay(bubbleOverlay);
        if (updateConnected === true) {
            Script.update.disconnect(update);
        }
    });

}()); // END LOCAL_SCOPE
