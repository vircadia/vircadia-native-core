//
// tPose.js
// examples
//
// Created by Anthony Thibault on 12/10/2015
// Copyright 2015 High Fidelity, Inc.
//
// Distributed under the Apache License, Version 2.0.
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
// Example of how to put the avatar into it's default tpose.
//

// TODO: CHANGE
var buttonImageUrl = "https://s3.amazonaws.com/hifi-public/images/tools/tpose.svg";
var windowDimensions = Controller.getViewportDimensions();

var buttonWidth = 37;
var buttonHeight = 46;
var buttonPadding = 10;

var buttonPositionX = windowDimensions.x - buttonPadding - buttonWidth;
var buttonPositionY = (windowDimensions.y - buttonHeight) / 2 - (buttonHeight + buttonPadding);

var tPoseEnterImageOverlay = {
    x: buttonPositionX,
    y: buttonPositionY,
    width: buttonWidth,
    height: buttonHeight,
    subImage: { x: 0, y: buttonHeight, width: buttonWidth, height: buttonHeight },
    imageURL: buttonImageUrl,
    visible: true,
    alpha: 1.0
};

var tPoseExitImageOverlay = {
    x: buttonPositionX,
    y: buttonPositionY,
    width: buttonWidth,
    height: buttonHeight,
    subImage: { x: buttonWidth, y: buttonHeight, width: buttonWidth, height: buttonHeight },
    imageURL: buttonImageUrl,
    visible: false,
    alpha: 1.0
};

var tPoseEnterButton = Overlays.addOverlay("image", tPoseEnterImageOverlay);
var tPoseExitButton = Overlays.addOverlay("image", tPoseExitImageOverlay);
var tPose = false;

function enterDefaultPose() {
    tPose = true;
    var i, l = MyAvatar.getJointNames().length;
    var rot, trans;
    for (i = 0; i < l; i++) {
        rot = MyAvatar.getDefaultJointRotation(i);
        trans = MyAvatar.getDefaultJointTranslation(i);
        MyAvatar.setJointData(i, rot, trans);
    }
    Overlays.editOverlay(tPoseEnterButton, { visible: false });
    Overlays.editOverlay(tPoseExitButton, { visible: true });
}

function exitDefaultPose() {
    tPose = false;
    MyAvatar.clearJointsData();
    Overlays.editOverlay(tPoseEnterButton, { visible: true });
    Overlays.editOverlay(tPoseExitButton, { visible: false });
}

Controller.mousePressEvent.connect(function (event) {
    var clickedOverlay = Overlays.getOverlayAtPoint({ x: event.x, y: event.y });
    if (clickedOverlay == tPoseEnterButton) {
        enterDefaultPose();
    } else if (clickedOverlay == tPoseExitButton) {
        exitDefaultPose();
    }
});

Script.scriptEnding.connect(function() {
    Overlays.deleteOverlay(tPoseEnterButton);
    Overlays.deleteOverlay(tPoseExitButton);
});

