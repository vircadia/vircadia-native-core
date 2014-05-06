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

// misc global constants
var NUMBER_OF_COLLISION_BUTTONS = 3;
var NUMBER_OF_BUTTONS = 4;
var DOWN = { x: 0.0, y: -1.0, z: 0.0 };
var MAX_VOXEL_SCAN_DISTANCE = 30.0;

// behavior transition thresholds
var MIN_FLYING_SPEED = 3.0;
var MIN_COLLISIONLESS_SPEED = 5.0;
var MAX_WALKING_SPEED = 30.0;
var MAX_COLLIDABLE_SPEED = 35.0;

// button URL and geometry/UI tuning
var BUTTON_IMAGE_URL = "http://highfidelity-public.s3-us-west-1.amazonaws.com/images/testing-swatches.svg";
var DISABLED_OFFSET_Y = 12;
var ENABLED_OFFSET_Y = 55 + 12;
var UI_BUFFER = 1;
var OFFSET_X = UI_BUFFER;
var OFFSET_Y = 200;
var BUTTON_WIDTH = 30;
var BUTTON_HEIGHT = 30;
var TEXT_OFFSET_X = OFFSET_X + BUTTON_WIDTH + UI_BUFFER;
var TEXT_HEIGHT = BUTTON_HEIGHT;
var TEXT_WIDTH = 210;

var MSEC_PER_SECOND = 1000;
var RAYCAST_EXPIRY_PERIOD = MSEC_PER_SECOND / 16;
var COLLISION_EXPIRY_PERIOD = 2 * MSEC_PER_SECOND;
var GRAVITY_ON_EXPIRY_PERIOD = MSEC_PER_SECOND / 2;
var GRAVITY_OFF_EXPIRY_PERIOD = MSEC_PER_SECOND / 8;

var dater = new Date();
var raycastExpiry = dater.getTime() + RAYCAST_EXPIRY_PERIOD;
var gravityOnExpiry = dater.getTime() + GRAVITY_ON_EXPIRY_PERIOD;
var gravityOffExpiry = dater.getTime() + GRAVITY_OFF_EXPIRY_PERIOD;
var collisionOnExpiry = dater.getTime() + COLLISION_EXPIRY_PERIOD;

// avatar state
var velocity = { x: 0.0, y: 0.0, z: 0.0 };
var standing = false;

// speedometer globals
var speed = 0.0;
var lastPosition = MyAvatar.position;
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

// collision group buttons
var buttons = new Array();
var labels = new Array();

var labelContents = new Array();
labelContents[0] = "Collide with Avatars";
labelContents[1] = "Collide with Voxels";
labelContents[2] = "Collide with Particles";
labelContents[3] = "Use local gravity";
var groupBits = 0;

var enabledColors = new Array();
enabledColors[0] = { red: 255, green: 0, blue: 0};
enabledColors[1] = { red: 0, green: 255, blue: 0};
enabledColors[2] = { red: 0, green: 0, blue: 255};
enabledColors[3] = { red: 255, green: 255, blue: 0};

var disabledColors = new Array();
disabledColors[0] = { red: 90, green: 75, blue: 75};
disabledColors[1] = { red: 75, green: 90, blue: 75};
disabledColors[2] = { red: 75, green: 75, blue: 90};
disabledColors[3] = { red: 90, green: 90, blue: 75};

var buttonStates = new Array();

for (i = 0; i < NUMBER_OF_BUTTONS; i++) {
    var offsetS = 12
    var offsetT = DISABLED_OFFSET_Y;

    buttons[i] = Overlays.addOverlay("image", {
                    x: OFFSET_X,
                    y: OFFSET_Y + (BUTTON_HEIGHT * i),
                    width: BUTTON_WIDTH,
                    height: BUTTON_HEIGHT,
                    subImage: { x: offsetS, y: offsetT, width: BUTTON_WIDTH, height: BUTTON_HEIGHT },
                    imageURL: BUTTON_IMAGE_URL,
                    color: disabledColors[i],
                    alpha: 1,
                });

    labels[i] = Overlays.addOverlay("text", {
                    x: TEXT_OFFSET_X,
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


// functions

function updateButton(i, enabled) {
    var offsetY = DISABLED_OFFSET_Y;
    var buttonColor = disabledColors[i];
    if (enabled) {
        offsetY = ENABLED_OFFSET_Y;
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
    if (groupBits != MyAvatar.collisionGroups) {
        MyAvatar.collisionGroups = groupBits;
    }

    Overlays.editOverlay(buttons[i], { subImage: { y: offsetY } } );
    Overlays.editOverlay(buttons[i], { color: buttonColor } );
    buttonStates[i] = enabled;
}


// When our script shuts down, we should clean up all of our overlays
function scriptEnding() {
    for (i = 0; i < NUMBER_OF_BUTTONS; i++) {
        Overlays.deleteOverlay(buttons[i]);
        Overlays.deleteOverlay(labels[i]);
    }
    Overlays.deleteOverlay(speedometer);
}
Script.scriptEnding.connect(scriptEnding);


function updateSpeedometerDisplay() {
    Overlays.editOverlay(speedometer, { text: "Speed: " + speed.toFixed(2) });
}
Script.setInterval(updateSpeedometerDisplay, 100);

function disableArtificialGravity() {
    MyAvatar.motionBehaviors = MyAvatar.motionBehaviors & ~AVATAR_MOTION_OBEY_LOCAL_GRAVITY;
    updateButton(3, false);
}
// call this immediately so that avatar doesn't fall before voxel data arrives
// Ideally we would only do this on LOGIN, not when starting the script
// in the middle of a session.
disableArtificialGravity();

function enableArtificialGravity() {
    // NOTE: setting the gravity automatically sets the AVATAR_MOTION_OBEY_LOCAL_GRAVITY behavior bit.
    MyAvatar.gravity = DOWN;
    updateButton(3, true);
    // also enable collisions with voxels
    groupBits |= COLLISION_GROUP_VOXELS;
    updateButton(1, groupBits & COLLISION_GROUP_VOXELS);
}

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
    speed = 0.8 * speed + 0.2 * distance / deltaTime;
    lastPosition = MyAvatar.position;

    dater = new Date();
    var now = dater.getTime();

    // transition gravity
    if (raycastExpiry < now) {
        // scan for landing platform
        ray = { origin: MyAvatar.position, direction: DOWN };
        var intersection = Voxels.findRayIntersection(ray);
        // NOTE: it is possible for intersection.intersects to be false when it should be true
        // (perhaps the raycast failed to lock the octree thread?).  To workaround this problem
        // we only transition on repeated failures.

        if (intersection.intersects) {
            // compute distance to voxel
            var v = intersection.voxel;
            var maxCorner = Vec3.sum({ x: v.x, y: v.y, z: v.z }, {x: v.s, y: v.s, z: v.s });
            var distance = lastPosition.y - maxCorner.y;

            if (distance < MAX_VOXEL_SCAN_DISTANCE) {
                if (speed < MIN_FLYING_SPEED && 
                    gravityOnExpiry < now &&
                    !(MyAvatar.motionBehaviors & AVATAR_MOTION_OBEY_LOCAL_GRAVITY)) {
                        enableArtificialGravity();
                }
                if (speed < MAX_WALKING_SPEED) {
                    gravityOffExpiry = now + GRAVITY_OFF_EXPIRY_PERIOD;
                } else if (gravityOffExpiry < now && MyAvatar.motionBehaviors & AVATAR_MOTION_OBEY_LOCAL_GRAVITY) {
                    disableArtificialGravity();
                }
            } else {
                // distance too far
                if (gravityOffExpiry < now && MyAvatar.motionBehaviors & AVATAR_MOTION_OBEY_LOCAL_GRAVITY) {
                    disableArtificialGravity();
                }
                gravityOnExpiry = now + GRAVITY_ON_EXPIRY_PERIOD;
            }
        } else {
            // no intersection
            if (gravityOffExpiry < now && MyAvatar.motionBehaviors & AVATAR_MOTION_OBEY_LOCAL_GRAVITY) {
                disableArtificialGravity();
            }
            gravityOnExpiry = now + GRAVITY_ON_EXPIRY_PERIOD;
        }
    }
    if (speed > MAX_WALKING_SPEED && gravityOffExpiry < now) {
        if (MyAvatar.motionBehaviors & AVATAR_MOTION_OBEY_LOCAL_GRAVITY) {
            // turn off gravity
            MyAvatar.motionBehaviors = MyAvatar.motionBehaviors & ~AVATAR_MOTION_OBEY_LOCAL_GRAVITY;
            updateButton(3, false);
        }
        gravityOnExpiry = now + GRAVITY_ON_EXPIRY_PERIOD;
    }

    // transition collidability with voxels
    if (speed < MIN_COLLISIONLESS_SPEED) {
        if (collisionOnExpiry < now && !(MyAvatar.collisionGroups & COLLISION_GROUP_VOXELS)) {
            // TODO: check to make sure not already colliding
            // enable collision with voxels
            groupBits |= COLLISION_GROUP_VOXELS;
            updateButton(1, groupBits & COLLISION_GROUP_VOXELS);
        }
    } else {
        collisionOnExpiry = now + COLLISION_EXPIRY_PERIOD;
    }
    if (speed > MAX_COLLIDABLE_SPEED) {
        if (MyAvatar.collisionGroups & COLLISION_GROUP_VOXELS) {
            // disable collisions with voxels
            groupBits &= ~COLLISION_GROUP_VOXELS;
            updateButton(1, groupBits & COLLISION_GROUP_VOXELS);
        }
    }
}
Script.update.connect(update);

// we also handle click detection in our mousePressEvent()
function mousePressEvent(event) {
    var clickedOverlay = Overlays.getOverlayAtPoint({x: event.x, y: event.y});
    for (i = 0; i < NUMBER_OF_COLLISION_BUTTONS; i++) {
        if (clickedOverlay == buttons[i]) {
            var enabled = !(buttonStates[i]);
            updateButton(i, enabled);
        }
    }
}
Controller.mousePressEvent.connect(mousePressEvent);

