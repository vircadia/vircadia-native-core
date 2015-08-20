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

var MOVE_DISTANCE = 2.0;
var PITCH_INCREMENT = 0.5; // degrees
var pitchChange = 0; // degrees
var YAW_INCREMENT = 0.5; // degrees
var VR_YAW_INCREMENT = 15.0; // degrees
var yawChange = 0;
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

var velocity = { x: 0, y: 0, z: 0 };
var VERY_LONG_TIME = 1000000.0;

var active = HMD.active;
var prevVRMode = HMD.active;

var hmdControls = (function () {

    function onKeyPressEvent(event) {
        if (event.text == 'g' && event.isMeta) {
            active = !active;
        }
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

	function onActionEvent(action, state) {
        if (!active) {
            return;
        }
        if (state < THRESHOLD) {
            if (action == findAction("YAW_LEFT") || action == findAction("YAW_RIGHT")) {
                yawTimer = CAMERA_UPDATE_TIME;
            } else if (action == findAction("PITCH_UP") || action == findAction("PITCH_DOWN")) {
                pitchTimer = CAMERA_UPDATE_TIME;
            }
            return;
        }
        switch (action) {
            case findAction("LONGITUDINAL_BACKWARD"):
                var direction = {x: 0.0, y: 0.0, z:1.0};
                direction = Vec3.multiply(Vec3.normalize(direction), shifted ? SHIFT_MAG * MOVE_DISTANCE : MOVE_DISTANCE);
                velocity = Vec3.sum(velocity, direction);
                break;
            case findAction("LONGITUDINAL_FORWARD"):
                var direction = {x: 0.0, y: 0.0, z:-1.0};
                direction = Vec3.multiply(Vec3.normalize(direction), shifted ? SHIFT_MAG * MOVE_DISTANCE : MOVE_DISTANCE);
                velocity = Vec3.sum(velocity, direction);
                break;
            case findAction("LATERAL_LEFT"):
                var direction = {x:-1.0, y: 0.0, z: 0.0}
                direction = Vec3.multiply(Vec3.normalize(direction), shifted ? SHIFT_MAG * MOVE_DISTANCE : MOVE_DISTANCE);
                velocity = Vec3.sum(velocity, direction);
                break;
            case findAction("LATERAL_RIGHT"):
                var direction = {x:1.0, y: 0.0, z: 0.0};
                direction = Vec3.multiply(Vec3.normalize(direction), shifted ? SHIFT_MAG * MOVE_DISTANCE : MOVE_DISTANCE);
                velocity = Vec3.sum(velocity, direction);
                break;
            case findAction("VERTICAL_DOWN"):
                var direction = {x: 0.0, y: -1.0, z: 0.0};
                direction = Vec3.multiply(Vec3.normalize(direction), shifted ? SHIFT_MAG * MOVE_DISTANCE : MOVE_DISTANCE);
                velocity = Vec3.sum(velocity, direction);
                break;
            case findAction("VERTICAL_UP"):
                var direction = {x: 0.0, y: 1.0, z: 0.0};
                direction = Vec3.multiply(Vec3.normalize(direction), shifted ? SHIFT_MAG * MOVE_DISTANCE : MOVE_DISTANCE);
                velocity = Vec3.sum(velocity, direction);
                break;
            case findAction("YAW_LEFT"):
                if (yawTimer < 0.0 && HMD.active) {
                    yawChange = yawChange + (shifted ? SHIFT_MAG * VR_YAW_INCREMENT : VR_YAW_INCREMENT);
                    yawTimer = CAMERA_UPDATE_TIME;
                } else if (!HMD.active) {
                    yawChange = yawChange + (shifted ? SHIFT_MAG * YAW_INCREMENT : YAW_INCREMENT);
                }
                break;
            case findAction("YAW_RIGHT"):
                if (yawTimer < 0.0 && HMD.active) {
                    yawChange = yawChange - (shifted ? SHIFT_MAG * VR_YAW_INCREMENT : VR_YAW_INCREMENT);
                    yawTimer = CAMERA_UPDATE_TIME;
                } else if (!HMD.active) {
                    yawChange = yawChange - (shifted ? SHIFT_MAG * YAW_INCREMENT : YAW_INCREMENT);
                }
                break;
            case findAction("PITCH_DOWN"):
                if (!HMD.active) {
                    pitchChange = pitchChange - (shifted ? SHIFT_MAG * PITCH_INCREMENT : PITCH_INCREMENT);
                }
                break;
            case findAction("PITCH_UP"):
                if (!HMD.active) {
                    pitchChange = pitchChange + (shifted ? SHIFT_MAG * PITCH_INCREMENT : PITCH_INCREMENT);
                }
                break;
            case findAction("SHIFT"): // speed up
                if (shiftTimer < 0.0) {
                    shifted = !shifted;
                    shiftTimer = SHIFT_UPDATE_TIME;
                }
                break;
            case findAction("ACTION1"): // start/end warp
                if (warpTimer < 0.0) {
                    warpActive = !warpActive;
                    if (!warpActive) {
                        finishWarp();
                    }
                    warpTimer = WARP_UPDATE_TIME;
                }
                break;
            case findAction("ACTION2"): // cancel warp
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
        if (prevVRMode != HMD.active) {
            active = HMD.active;
            prevVRMode = HMD.active;
        }

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

        if (active) {
            Controller.captureActionEvents();

            MyAvatar.bodyYaw = MyAvatar.bodyYaw + yawChange;
            MyAvatar.headPitch = Math.max(-180, Math.min(180, MyAvatar.headPitch + pitchChange));
            yawChange = 0;
            pitchChange = 0;

            MyAvatar.motorVelocity = velocity;
            MyAvatar.motorTimescale = 0.0;
            velocity = { x: 0, y: 0, z: 0 };
        } else {
            Controller.releaseActionEvents();
            yawChange = 0;
            pitchChange = 0;
            MyAvatar.motorVelocity = {x:0.0, y:0.0, z:0.0}
            MyAvatar.motorTimescale = VERY_LONG_TIME;
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
        Controller.keyPressEvent.connect(onKeyPressEvent);

        Controller.actionEvent.connect(onActionEvent);

        Script.update.connect(update);
    }

    function tearDown() {
    	Controller.releaseActionEvents();
        MyAvatar.motorVelocity = {x:0.0, y:0.0, z:0.0}
        MyAvatar.motorTimescale = VERY_LONG_TIME;
    }

    setUp();
    Script.scriptEnding.connect(tearDown);
}());