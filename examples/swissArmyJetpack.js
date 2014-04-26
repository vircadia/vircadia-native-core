//
//  swissArmyJetpack.js
//  examples
//
//  Created by Andrew Meadows 2014.04.24
//  Copyright 2014 High Fidelity, Inc.
//
//  This is a work in progress.  It will eventually be able to move the avatar around,
//  toggle collision groups, modify avatar movement options, and other stuff (maybe trigger animations).
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var numberOfButtons = 3;

var enabledColors = new Array();
enabledColors[0] = { red: 255, green: 0, blue: 0};
enabledColors[1] = { red: 0, green: 255, blue: 0};
enabledColors[2] = { red: 0, green: 0, blue: 255};

var disabledColors = new Array();
disabledColors[0] = { red: 90, green: 75, blue: 75};
disabledColors[1] = { red: 75, green: 90, blue: 75};
disabledColors[2] = { red: 75, green: 90, blue: 90};

var buttons = new Array();
var labels = new Array();

var labelContents = new Array();
labelContents[0] = "Collide with Avatars";
labelContents[1] = "Collide with Voxels";
labelContents[2] = "Collide with Particles";
var groupBits = 0;

var buttonStates = new Array();

var disabledOffsetT = 0;
var enabledOffsetT = 55;

var buttonX = 50;
var buttonY = 200;
var buttonWidth = 30;
var buttonHeight = 54;
var textX = buttonX + buttonWidth + 10;

for (i = 0; i < numberOfButtons; i++) {
    var offsetS = 12
    var offsetT = disabledOffsetT;

    buttons[i] = Overlays.addOverlay("image", {
                    //x: buttonX + (buttonWidth * i),
                    x: buttonX,
                    y: buttonY + (buttonHeight * i),
                    width: buttonWidth,
                    height: buttonHeight,
                    subImage: { x: offsetS, y: offsetT, width: buttonWidth, height: buttonHeight },
                    imageURL: "http://highfidelity-public.s3-us-west-1.amazonaws.com/images/testing-swatches.svg",
                    color: disabledColors[i],
                    alpha: 1,
                });

    labels[i] = Overlays.addOverlay("text", {
                    x: textX,
                    y: buttonY + (buttonHeight * i) + 12,
                    width: 150,
                    height: 50,
                    color: { red: 0, green: 0, blue: 0},
                    textColor: { red: 255, green: 0, blue: 0},
                    topMargin: 4,
                    leftMargin: 4,
                    text: labelContents[i]
                });

    buttonStates[i] = false;
}

function updateButton(i, enabled) {
    var offsetY = disabledOffsetT;
    var buttonColor = disabledColors[i];
    groupBits
    if (enabled) {
        offsetY = enabledOffsetT;
        buttonColor = enabledColors[i];
        if (i == 0) {
            groupBits |= COLLISION_GROUP_AVATARS;
        } else if (i == 1) {
            groupBits |= COLLISION_GROUP_VOXELS;
        } else if (i == 2) {
            groupBits |= COLLISION_GROUP_PARTICLES;
        }
    } else {
        if (i == 0) {
            groupBits &= ~COLLISION_GROUP_AVATARS;
        } else if (i == 1) {
            groupBits &= ~COLLISION_GROUP_VOXELS;
        } else if (i == 2) {
            groupBits &= ~COLLISION_GROUP_PARTICLES;
        }
    }
    MyAvatar.collisionGroups = groupBits;

    Overlays.editOverlay(buttons[i], { subImage: { y: offsetY } } );
    Overlays.editOverlay(buttons[i], { color: buttonColor } );
    buttonStates[i] = enabled;
}

// When our script shuts down, we should clean up all of our overlays
function scriptEnding() {
    for (i = 0; i < numberOfButtons; i++) {
        print("adebug deleting overlay " + i);
        Overlays.deleteOverlay(buttons[i]);
        Overlays.deleteOverlay(labels[i]);
    }
}
Script.scriptEnding.connect(scriptEnding);


// Our update() function is called at approximately 60fps, and we will use it to animate our various overlays
function update(deltaTime) {
    if (groupBits != MyAvatar.collisionGroups) {
        groupBits = MyAvatar.collisionGroups;
        updateButton(0, groupBits & COLLISION_GROUP_AVATARS);
        updateButton(1, groupBits & COLLISION_GROUP_VOXELS);
        updateButton(2, groupBits & COLLISION_GROUP_PARTICLES);
    }
}
Script.update.connect(update);


// we also handle click detection in our mousePressEvent()
function mousePressEvent(event) {
    var clickedOverlay = Overlays.getOverlayAtPoint({x: event.x, y: event.y});
    for (i = 0; i < numberOfButtons; i++) {
        if (clickedOverlay == buttons[i]) {
            var enabled = !(buttonStates[i]);
            updateButton(i, enabled);
        }
    }
}
Controller.mousePressEvent.connect(mousePressEvent);

