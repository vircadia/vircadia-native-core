//
//  hmdControls.js
//  examples
//
//  Created by Sam Gondelman on 6/17/15
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var MOVE_DISTANCE = 0.3;
var PITCH_INCREMENT = 0.5; // degrees
var YAW_INCREMENT = 0.5; // degrees
var VR_YAW_INCREMENT = 15.0; // degrees
var BOOM_SPEED = 0.5;
var THRESHOLD = 0.2;

var CAMERA_UPDATE_TIME = 0.5;
var yawTimer = CAMERA_UPDATE_TIME;

var shifted = false;
var SHIFT_UPDATE_TIME = 0.5;
var shiftTimer = SHIFT_UPDATE_TIME;
var SHIFT_MAG = 4.0;

var warpActive = false;
var WARP_UPDATE_TIME = .5;
var warpTimer = WARP_UPDATE_TIME;

var warpPosition = { x: 0, y: 0, z: 0 };

var WARP_SPHERE_SIZE = 1;
var warpSphere = Overlays.addOverlay("sphere", {
    position: { x: 0, y: 0, z: 0 },
    size: WARP_SPHERE_SIZE,
    color: { red: 0, green: 255, blue: 0 },
    alpha: 1.0,
    solid: true,
    visible: false,
});

var WARP_LINE_HEIGHT = 10;
var warpLine = Overlays.addOverlay("line3d", {
    start: { x: 0, y: 0, z:0 },
    end: { x: 0, y: 0, z: 0 },
    color: { red: 0, green: 255, blue: 255},
    alpha: 1,
    lineWidth: 5,
    visible: false,
});

var hmdControls = (function () {

	function onActionEvent(action, state) {
        if (state < THRESHOLD) {
            if (action == 6 || action == 7) {
                yawTimer = CAMERA_UPDATE_TIME;
            } else if (action == 8 || action == 9) {
                pitchTimer = CAMERA_UPDATE_TIME;
            }
            return;
        }
        switch (action) {
            case 0: // backward
                var direction = Quat.getFront(Camera.getOrientation());
                direction = Vec3.multiply(-1, direction);
                direction = Vec3.multiply(Vec3.normalize(direction), shifted ? SHIFT_MAG * MOVE_DISTANCE : MOVE_DISTANCE);
                MyAvatar.position = Vec3.sum(MyAvatar.position, direction);
                break;
            case 1: // forward
                var direction = Quat.getFront(Camera.getOrientation());
                direction = Vec3.multiply(Vec3.normalize(direction), shifted ? SHIFT_MAG * MOVE_DISTANCE : MOVE_DISTANCE);
                MyAvatar.position = Vec3.sum(MyAvatar.position, direction);
                break;
            case 2: // left
                var direction = Quat.getRight(Camera.getOrientation());
                direction = Vec3.multiply(-1, direction);
                direction = Vec3.multiply(Vec3.normalize(direction), shifted ? SHIFT_MAG * MOVE_DISTANCE : MOVE_DISTANCE);
                MyAvatar.position = Vec3.sum(MyAvatar.position, direction);
                break;
            case 3: // right
                var direction = Quat.getRight(Camera.getOrientation());
                direction = Vec3.multiply(Vec3.normalize(direction), shifted ? SHIFT_MAG * MOVE_DISTANCE : MOVE_DISTANCE);
                MyAvatar.position = Vec3.sum(MyAvatar.position, direction);
                break;
            case 4: // down
                var direction = Quat.getUp(Camera.getOrientation());
                direction = Vec3.multiply(-1, direction);
                direction = Vec3.multiply(Vec3.normalize(direction), shifted ? SHIFT_MAG * MOVE_DISTANCE : MOVE_DISTANCE);
                MyAvatar.position = Vec3.sum(MyAvatar.position, direction);
                break;
            case 5: // up
                var direction = Quat.getUp(Camera.getOrientation());
                direction = Vec3.multiply(Vec3.normalize(direction), shifted ? SHIFT_MAG * MOVE_DISTANCE : MOVE_DISTANCE);
                MyAvatar.position = Vec3.sum(MyAvatar.position, direction);
                break;
            case 6: // yaw left
                if (yawTimer < 0.0 && Menu.isOptionChecked("Enable VR Mode")) {
                    MyAvatar.bodyYaw = MyAvatar.bodyYaw + (shifted ? SHIFT_MAG * VR_YAW_INCREMENT : VR_YAW_INCREMENT);
                    yawTimer = CAMERA_UPDATE_TIME;
                } else if (!Menu.isOptionChecked("Enable VR Mode")) {
                    MyAvatar.bodyYaw = MyAvatar.bodyYaw + (shifted ? SHIFT_MAG * YAW_INCREMENT : YAW_INCREMENT);
                }
                break;
            case 7: // yaw right
                if (yawTimer < 0.0 && Menu.isOptionChecked("Enable VR Mode")) {
                    MyAvatar.bodyYaw = MyAvatar.bodyYaw - (shifted ? SHIFT_MAG * VR_YAW_INCREMENT : VR_YAW_INCREMENT);
                    yawTimer = CAMERA_UPDATE_TIME;
                } else if (!Menu.isOptionChecked("Enable VR Mode")) {
                    MyAvatar.bodyYaw = MyAvatar.bodyYaw - (shifted ? SHIFT_MAG * YAW_INCREMENT : YAW_INCREMENT);
                }
                break;
            case 8: // pitch down
                if (!Menu.isOptionChecked("Enable VR Mode")) {
                    MyAvatar.headPitch = Math.max(-180, Math.min(180, MyAvatar.headPitch - (shifted ? SHIFT_MAG * PITCH_INCREMENT : PITCH_INCREMENT)));
                }
                break;
            case 9: // pitch up
                if (!Menu.isOptionChecked("Enable VR Mode")) {
                    MyAvatar.headPitch = Math.max(-180, Math.min(180, MyAvatar.headPitch + (shifted ? SHIFT_MAG * PITCH_INCREMENT : PITCH_INCREMENT)));
                }
                break;
            case 12: // shift
                if (shiftTimer < 0.0) {
                    shifted = !shifted;
                    shiftTimer = SHIFT_UPDATE_TIME;
                }
                break;
            case 13: // action1 = start/end warp
                if (warpTimer < 0.0) {
                    warpActive = !warpActive;
                    if (!warpActive) {
                        finishWarp();
                    }
                    warpTimer = WARP_UPDATE_TIME;
                }
                break;
            case 14: // action2 = cancel warp
                warpActive = false;
                Overlays.editOverlay(warpSphere, {
                    visible: false,
                });
                Overlays.editOverlay(warpLine, {
                    visible: false,
                });
            default:
                break;
        }
    }

    function update(dt) {
        if (yawTimer >= 0.0) {
            yawTimer = yawTimer - dt;
        }
        if (shiftTimer >= 0.0) {
            shiftTimer = shiftTimer - dt;
        }
        if (warpTimer >= 0.0) {
            warpTimer = warpTimer - dt;
        }

        if (warpActive) {
            updateWarp();
        }
    }

    function updateWarp() {
        var look = Quat.getFront(Camera.getOrientation());
        var pitch = Math.asin(look.y);

        // Get relative to looking straight down
        pitch += Math.PI / 2;

        // Scale up
        pitch *= 2;
        var distance = pitch * pitch * pitch;

        var warpDirection = Vec3.normalize({ x: look.x, y: 0, z: look.z });
        warpPosition = Vec3.multiply(warpDirection, distance);
        warpPosition = Vec3.sum(MyAvatar.position, warpPosition);

        // Commented out until ray picking can be fixed
        // var pickRay = {
        //     origin: Vec3.sum(warpPosition, WARP_PICK_OFFSET),
        //     direction: { x: 0, y: -1, z: 0 }
        // };

        // var intersection = Entities.findRayIntersection(pickRay);

        // if (intersection.intersects && intersection.distance < WARP_PICK_MAX_DISTANCE) {
        //     // Warp 1 meter above the object - this is an approximation
        //     // TODO Get the actual offset to the Avatar's feet and plant them to
        //     // the object.
        //     warpPosition = Vec3.sum(intersection.intersection, { x: 0, y: 1, z:0 });
        // }

        // Adjust overlays to match warp position
        Overlays.editOverlay(warpSphere, {
            position: warpPosition,
            visible: true,
        });
        Overlays.editOverlay(warpLine, {
            start: warpPosition,
            end: Vec3.sum(warpPosition, { x: 0, y: WARP_LINE_HEIGHT, z: 0 }),
            visible: true,
        });
    }

    function finishWarp() {
        Overlays.editOverlay(warpSphere, {
            visible: false,
        });
        Overlays.editOverlay(warpLine, {
            visible: false,
        });
        MyAvatar.position = warpPosition;
    }

	function setUp() {
		Controller.captureActionEvents();

        Controller.actionEvent.connect(onActionEvent);

        Script.update.connect(update);
    }

    function tearDown() {
    	Controller.releaseActionEvents();
    }

    setUp();
    Script.scriptEnding.connect(tearDown);
}());