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
var DOWN = { x: 0.0, y: -1.0, z: 0.0 };
var MAX_VOXEL_SCAN_DISTANCE = 20.0;
var MIN_FLYING_SPEED = 1.0;

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

var disabledOffsetT = 12;
var enabledOffsetT = 55 + 12;

var UI_BUFFER = 1;
var OFFSET_X = UI_BUFFER;
var OFFSET_Y = 200;
var BUTTON_WIDTH = 30;
var BUTTON_HEIGHT = 30;
var textX = OFFSET_X + BUTTON_WIDTH + UI_BUFFER;
var TEXT_HEIGHT = BUTTON_HEIGHT;
var TEXT_WIDTH = 210;

var speedometer = Overlays.addOverlay("text", {
        x: OFFSET_X,
        y: OFFSET_Y - BUTTON_HEIGHT,
        width: BUTTON_WIDTH + UI_BUFFER + TEXT_WIDTH,
        height: TEXT_HEIGHT,
        color: { red: 0, green: 0, blue: 0 },
        textColor: { red: 255, green: 0, blue: 0},
        topMargin: 4,
        leftMargin: 4,
        text: "Speed: 0.0"
    });
var speed = 0.0;
var lastPosition = MyAvatar.position;

for (i = 0; i < numberOfButtons; i++) {
    var offsetS = 12
    var offsetT = disabledOffsetT;

    buttons[i] = Overlays.addOverlay("image", {
                    x: OFFSET_X,
                    y: OFFSET_Y + (BUTTON_HEIGHT * i),
                    width: BUTTON_WIDTH,
                    height: BUTTON_HEIGHT,
                    subImage: { x: offsetS, y: offsetT, width: BUTTON_WIDTH, height: BUTTON_HEIGHT },
                    imageURL: "http://highfidelity-public.s3-us-west-1.amazonaws.com/images/testing-swatches.svg",
                    color: disabledColors[i],
                    alpha: 1,
                });

    labels[i] = Overlays.addOverlay("text", {
                    x: textX,
                    y: OFFSET_Y + (BUTTON_HEIGHT * i),
                    width: TEXT_WIDTH,
                    height: TEXT_HEIGHT,
                    color: { red: 0, green: 0, blue: 0},
                    textColor: { red: 255, green: 0, blue: 0},
                    topMargin: 4,
                    leftMargin: 4,
                    text: labelContents[i]
                });

    buttonStates[i] = false;
}

// avatar state
var velocity = { x: 0.0, y: 0.0, z: 0.0 };
var standing = false;

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
        Overlays.deleteOverlay(buttons[i]);
        Overlays.deleteOverlay(labels[i]);
    }
    Overlays.deleteOverlay(speedometer);
}
Script.scriptEnding.connect(scriptEnding);

function updateSpeedometerDisplay() {
    Overlays.editOverlay(speedometer, { text: "Speed: " + speed.toFixed(2) });
}

var multiple_timer = Script.setInterval(updateSpeedometerDisplay, 100);

// Our update() function is called at approximately 60fps, and we will use it to animate our various overlays
function update(deltaTime) {
    if (groupBits != MyAvatar.collisionGroups) {
        groupBits = MyAvatar.collisionGroups;
        updateButton(0, groupBits & COLLISION_GROUP_AVATARS);
        updateButton(1, groupBits & COLLISION_GROUP_VOXELS);
        updateButton(2, groupBits & COLLISION_GROUP_PARTICLES);
    }

    // measure speed
    var distance = Vec3.distance(MyAvatar.position, lastPosition);
    speed = 0.9 * speed + 0.1 * distance / deltaTime;
    lastPosition = MyAvatar.position;

    // scan for landing platform
    if (speed < MIN_FLYING_SPEED && !(MyAvatar.motionBehaviors & AVATAR_MOTION_OBEY_GRAVITY)) {
        ray = { origin: MyAvatar.position, direction: DOWN };
        var intersection = Voxels.findRayIntersection(ray);
        if (intersection.intersects) {
            var v = intersection.voxel;
            var maxCorner = Vec3.sum({ x: v.x, y: v.y, z: v.z }, {x: v.s, y: v.s, z: v.s });
            var distance = lastPosition.y - maxCorner.y;
            if (distance < MAX_VOXEL_SCAN_DISTANCE) {
                MyAvatar.motionBehaviors = AVATAR_MOTION_OBEY_GRAVITY;
            }
            //print("voxel corner = <" + v.x + ", " + v.y + ", " + v.z + "> " + "  scale = " + v.s + "  dt = " + deltaTime);
        }
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

