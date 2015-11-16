//
// kneel.js
// examples
//
// Created by Anthony Thibault on 11/9/2015
// Copyright 2015 High Fidelity, Inc.
//
// Distributed under the Apache License, Version 2.0.
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
// Example of how to play an animation on an avatar.
//

var buttonImageUrl = "https://s3.amazonaws.com/hifi-public/images/tools/kneel.svg";
var windowDimensions = Controller.getViewportDimensions();

var buttonWidth = 37;
var buttonHeight = 46;
var buttonPadding = 10;

var buttonPositionX = windowDimensions.x - buttonPadding - buttonWidth;
var buttonPositionY = (windowDimensions.y - buttonHeight) / 2 - (buttonHeight + buttonPadding);

var kneelDownImageOverlay = {
    x: buttonPositionX,
    y: buttonPositionY,
    width: buttonWidth,
    height: buttonHeight,
    subImage: { x: 0, y: buttonHeight, width: buttonWidth, height: buttonHeight },
    imageURL: buttonImageUrl,
    visible: true,
    alpha: 1.0
};

var standUpImageOverlay = {
    x: buttonPositionX,
    y: buttonPositionY,
    width: buttonWidth,
    height: buttonHeight,
    subImage: { x: buttonWidth, y: buttonHeight, width: buttonWidth, height: buttonHeight },
    imageURL: buttonImageUrl,
    visible: false,
    alpha: 1.0
};

var kneelDownButton = Overlays.addOverlay("image", kneelDownImageOverlay);
var standUpButton = Overlays.addOverlay("image", standUpImageOverlay);
var kneeling = false;

var KNEEL_ANIM_URL = "https://hifi-public.s3.amazonaws.com/ozan/anim/kneel/kneel.fbx";

function kneelDown() {
    kneeling = true;

    var playbackRate = 30;  // 30 fps is normal speed.
    var loopFlag = false;
    var startFrame = 0;
    var endFrame = 82;

    // This will completly override all motion from the default animation system
    // including inverse kinematics for hand and head controllers.
    MyAvatar.overrideAnimation(KNEEL_ANIM_URL, playbackRate, loopFlag, startFrame, endFrame);

    Overlays.editOverlay(kneelDownButton, { visible: false });
    Overlays.editOverlay(standUpButton, { visible: true });
}

function standUp() {
    kneeling = false;

    // this will restore all motion from the default animation system.
    // inverse kinematics will work again normally.
    MyAvatar.restoreAnimation();

    Overlays.editOverlay(standUpButton, { visible: false });
    Overlays.editOverlay(kneelDownButton, { visible: true });
}

Controller.mousePressEvent.connect(function (event) {
    var clickedOverlay = Overlays.getOverlayAtPoint({ x: event.x, y: event.y });
    if (clickedOverlay == kneelDownButton) {
        kneelDown();
    } else if (clickedOverlay == standUpButton) {
        standUp();
    }
});

Script.scriptEnding.connect(function() {
    Overlays.deleteOverlay(kneelDownButton);
    Overlays.deleteOverlay(standUpButton);
});
