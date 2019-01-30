//
//  scripts/system/libraries/handTouch.js
//
//  Created by Luis Cuenca on 12/29/17
//  Copyright 2017 High Fidelity, Inc.
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* jslint bitwise: true */

/* global Script, Overlays, Controller, Vec3, MyAvatar, Entities, RayPick
*/

(function () {

    var LEAP_MOTION_NAME = "LeapMotion";
    var handTouchEnabled = true;
    var leapMotionEnabled = Controller.getRunningInputDeviceNames().indexOf(LEAP_MOTION_NAME) >= 0;
    var MSECONDS_AFTER_LOAD = 2000;
    var updateFingerWithIndex = 0;
    var untouchableEntities = [];

    // Keys to access finger data
    var fingerKeys = ["pinky", "ring", "middle", "index", "thumb"];

    // Additionally close the hands to achieve a grabbing effect
    var grabPercent = { left: 0, right: 0 };

    var Palm = function() {
        this.position = {x: 0, y: 0, z: 0};
        this.perpendicular = {x: 0, y: 0, z: 0};
        this.distance = 0;
        this.fingers = {
            pinky: {x: 0, y: 0, z: 0},
            middle: {x: 0, y: 0, z: 0},
            ring: {x: 0, y: 0, z: 0},
            thumb: {x: 0, y: 0, z: 0},
            index: {x: 0, y: 0, z: 0}
        };
        this.set = false;
    };

    var palmData = {
        left: new Palm(),
        right: new Palm()
    };

    var handJointNames = {left: "LeftHand", right: "RightHand"};

    // Store which fingers are touching - if all false restate the default poses
    var isTouching = {
        left: {
            pinky: false,
            middle: false,
            ring: false,
            thumb: false,
            index: false
        }, right: {
            pinky: false,
            middle: false,
            ring: false,
            thumb: false,
            index: false
        }
    };

    // frame count for transition to default pose

    var countToDefault = {
        left: 0,
        right: 0
    };

    // joint data for open pose
    var dataOpen = {
        left: {
            pinky: [
                {x: -0.0066, y: -0.0224, z: -0.2174, w: 0.9758},
                {x: 0.0112, y: 0.0001, z: 0.0093, w: 0.9999},
                {x: -0.0346, y: 0.0003, z: -0.0073, w: 0.9994}
            ],
            ring: [
                {x: -0.0029, y: -0.0094, z: -0.1413, w: 0.9899},
                {x: 0.0112, y: 0.0001, z: 0.0059, w: 0.9999},
                {x: -0.0346, y: 0.0002, z: -0.006, w: 0.9994}
            ],
            middle: [
                {x: -0.0016, y: 0, z: -0.0286, w: 0.9996},
                {x: 0.0112, y: -0.0001, z: -0.0063, w: 0.9999},
                {x: -0.0346, y: -0.0003, z: 0.0073, w: 0.9994}
            ],
            index: [
                {x: -0.0016, y: 0.0001, z: 0.0199, w: 0.9998},
                {x: 0.0112, y: 0, z: 0.0081, w: 0.9999},
                {x: -0.0346, y: 0.0008, z: -0.023, w: 0.9991}
            ],
            thumb: [
                {x: 0.0354, y: 0.0363, z: 0.3275, w: 0.9435},
                {x: -0.0945, y: 0.0938, z: 0.0995, w: 0.9861},
                {x: -0.0952, y: 0.0718, z: 0.1382, w: 0.9832}
            ]
        }, right: {
            pinky: [
                {x: -0.0034, y: 0.023, z: 0.1051, w: 0.9942},
                {x: 0.0106, y: -0.0001, z: -0.0091, w: 0.9999},
                {x: -0.0346, y: -0.0003, z: 0.0075, w: 0.9994}
            ],
            ring: [
                {x: -0.0013, y: 0.0097, z: 0.0311, w: 0.9995},
                {x: 0.0106, y: -0.0001, z: -0.0056, w: 0.9999},
                {x: -0.0346, y: -0.0002, z: 0.0061, w: 0.9994}
            ],
            middle: [
                {x: -0.001, y: 0, z: 0.0285, w: 0.9996},
                {x: 0.0106, y: 0.0001, z: 0.0062, w: 0.9999},
                {x: -0.0346, y: 0.0003, z: -0.0074, w: 0.9994}
            ],
            index: [
                {x: -0.001, y: 0, z: -0.0199, w: 0.9998},
                {x: 0.0106, y: -0.0001, z: -0.0079, w: 0.9999},
                {x: -0.0346, y: -0.0008, z: 0.0229, w: 0.9991}
            ],
            thumb: [
                {x: 0.0355, y: -0.0363, z: -0.3263, w: 0.9439},
                {x: -0.0946, y: -0.0938, z: -0.0996, w: 0.9861},
                {x: -0.0952, y: -0.0719, z: -0.1376, w: 0.9833}
            ]
        }
    };

    // joint data for close pose
    var dataClose = {
        left: {
            pinky: [
                {x: 0.5878, y: -0.1735, z: -0.1123, w: 0.7821},
                {x: 0.5704, y: 0.0053, z: 0.0076, w: 0.8213},
                {x: 0.6069, y: -0.0044, z: -0.0058, w: 0.7947}
            ],
            ring: [
                {x: 0.5761, y: -0.0989, z: -0.1025, w: 0.8048},
                {x: 0.5332, y: 0.0032, z: 0.005, w: 0.846},
                {x: 0.5773, y: -0.0035, z: -0.0049, w: 0.8165}
            ],
            middle: [
                {x: 0.543, y: -0.0469, z: -0.0333, w: 0.8378},
                {x: 0.5419, y: -0.0034, z: -0.0053, w: 0.8404},
                {x: 0.5015, y: 0.0037, z: 0.0063, w: 0.8651}
            ],
            index: [
                {x: 0.3051, y: -0.0156, z: -0.014, w: 0.9521},
                {x: 0.6414, y: 0.0051, z: 0.0063, w: 0.7671},
                {x: 0.5646, y: -0.013, z: -0.019, w: 0.8251}
            ],
            thumb: [
                {x: 0.313, y: -0.0348, z: 0.3192, w: 0.8938},
                {x: 0, y: 0, z: -0.37, w: 0.929},
                {x: 0, y: 0, z: -0.2604, w: 0.9655}
            ]
        }, right: {
            pinky: [
                {x: 0.5881, y: 0.1728, z: 0.1114, w: 0.7823},
                {x: 0.5704, y: -0.0052, z: -0.0075, w: 0.8213},
                {x: 0.6069, y: 0.0046, z: 0.006, w: 0.7947}
            ],
            ring: [
                {x: 0.5729, y: 0.1181, z: 0.0898, w: 0.8061},
                {x: 0.5332, y: -0.003, z: -0.0048, w: 0.846},
                {x: 0.5773, y: 0.0035, z: 0.005, w: 0.8165}
            ],
            middle: [
                {x: 0.543, y: 0.0468, z: 0.0332, w: 0.8378},
                {x: 0.5419, y: 0.0034, z: 0.0052, w: 0.8404},
                {x: 0.5047, y: -0.0037, z: -0.0064, w: 0.8632}
            ],
            index: [
                {x: 0.306, y: -0.0076, z: -0.0584, w: 0.9502},
                {x: 0.6409, y: -0.005, z: -0.006, w: 0.7675},
                {x: 0.5646, y: 0.0129, z: 0.0189, w: 0.8251}
            ],
            thumb: [
                {x: 0.313, y: 0.0352, z: -0.3181, w: 0.8942},
                {x: 0, y: 0, z: 0.3698, w: 0.9291},
                {x: 0, y: 0, z: 0.2609, w: 0.9654}
            ]
        }
    };

    // snapshot for the default pose
    var dataDefault = {
        left: {
            pinky: [{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0}],
            middle: [{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0}],
            ring: [{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0}],
            thumb: [{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0}],
            index: [{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0}],
            set: false
        },
        right: {
            pinky: [{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0}],
            middle: [{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0}],
            ring: [{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0}],
            thumb: [{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0}],
            index: [{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0}],
            set: false
        }
    };

    // joint data for the current frame
    var dataCurrent = {
        left: {
            pinky: [{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0}],
            middle: [{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0}],
            ring: [{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0}],
            thumb: [{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0}],
            index: [{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0}]
        },
        right: {
            pinky: [{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0}],
            middle: [{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0}],
            ring: [{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0}],
            thumb: [{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0}],
            index: [{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0}]
        }
    };

    // interpolated values on joint data to smooth movement
    var dataDelta = {
        left: {
            pinky: [{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0}],
            middle: [{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0}],
            ring: [{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0}],
            thumb: [{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0}],
            index: [{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0}]
        },
        right: {
            pinky: [{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0}],
            middle: [{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0}],
            ring: [{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0}],
            thumb: [{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0}],
            index: [{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0},{x: 0, y: 0, z: 0, w: 0}]
        }
    };

    // Acquire an updated value per hand every 5 frames when finger is touching (faster in)
    var touchAnimationSteps = 5;

    // Acquire an updated value per hand every 20 frames when finger is returning to default position (slower out)
    var defaultAnimationSteps = 10;

    // Debugging info
    var showSphere = false;
    var showLines = false;

    // This get setup on creation
    var linesCreated = false;
    var sphereCreated = false;

    // Register object with API Debugger
    var varsToDebug = {
        scriptLoaded: false,
        toggleDebugSphere: function() {
            showSphere = !showSphere;
            if (showSphere && !sphereCreated) {
                createDebugSphere();
                sphereCreated = true;
            }
        },
        toggleDebugLines: function() {
            showLines = !showLines;
            if (showLines && !linesCreated) {
                createDebugLines();
                linesCreated = true;
            }
        },
        fingerPercent: {
            left: {
                pinky: 0.38,
                middle: 0.38,
                ring: 0.38,
                thumb: 0.38,
                index: 0.38
            } ,
            right: {
                pinky: 0.38,
                middle: 0.38,
                ring: 0.38,
                thumb: 0.38,
                index: 0.38
            }
        },
        triggerValues: {
            leftTriggerValue: 0,
            leftTriggerClicked: 0,
            rightTriggerValue: 0,
            rightTriggerClicked: 0,
            leftSecondaryValue: 0,
            rightSecondaryValue: 0
        },
        palmData: {
            left: new Palm(),
            right: new Palm()
        },
        offset: {x: 0, y: 0, z: 0},
        avatarLoaded: false
    };

    // Add/Subtract the joint data - per finger joint
    function addVals(val1, val2, sign) {
        var val = [];
        if (val1.length !== val2.length) {
            return;
        }
        for (var i = 0; i < val1.length; i++) {
            val.push({x: 0, y: 0, z: 0, w: 0});
            val[i].x = val1[i].x + sign*val2[i].x;
            val[i].y = val1[i].y + sign*val2[i].y;
            val[i].z = val1[i].z + sign*val2[i].z;
            val[i].w = val1[i].w + sign*val2[i].w;
        }
        return val;
    }

    // Multiply/Divide  the joint data - per finger joint
    function multiplyValsBy(val1, num) {
        var val = [];
        for (var i = 0; i < val1.length; i++) {
            val.push({x: 0, y: 0, z: 0, w: 0});
            val[i].x = val1[i].x * num;
            val[i].y = val1[i].y * num;
            val[i].z = val1[i].z * num;
            val[i].w = val1[i].w * num;
        }
        return val;
    }

    // Calculate the finger lengths by adding its joint lengths
    function getJointDistances(jointNamesArray) {
        var result = {distances: [], totalDistance: 0};
        for (var i = 1; i < jointNamesArray.length; i++) {
            var index0 = MyAvatar.getJointIndex(jointNamesArray[i-1]);
            var index1 = MyAvatar.getJointIndex(jointNamesArray[i]);
            var pos0 = MyAvatar.getJointPosition(index0);
            var pos1 = MyAvatar.getJointPosition(index1);
            var distance = Vec3.distance(pos0, pos1);
            result.distances.push(distance);
            result.totalDistance += distance;
        }
        return result;
    }

    function dataRelativeToWorld(side, dataIn, dataOut) {
        var handJoint = handJointNames[side];
        var jointIndex = MyAvatar.getJointIndex(handJoint);
        var worldPosHand = MyAvatar.jointToWorldPoint({x: 0, y: 0, z: 0}, jointIndex);

        dataOut.position = MyAvatar.jointToWorldPoint(dataIn.position, jointIndex);
        var localPerpendicular = side === "right" ? {x: 0.2, y: 0, z: 1} : {x: -0.2, y: 0, z: 1};
        dataOut.perpendicular = Vec3.normalize(
            Vec3.subtract(MyAvatar.jointToWorldPoint(localPerpendicular, jointIndex), worldPosHand)
        );
        dataOut.distance = dataIn.distance;
        for (var i = 0; i < fingerKeys.length; i++) {
            var finger = fingerKeys[i];
            dataOut.fingers[finger] = MyAvatar.jointToWorldPoint(dataIn.fingers[finger], jointIndex);
        }
    }

    function dataRelativeToHandJoint(side, dataIn, dataOut) {
        var handJoint = handJointNames[side];
        var jointIndex = MyAvatar.getJointIndex(handJoint);
        var worldPosHand = MyAvatar.jointToWorldPoint({x: 0, y: 0, z: 0}, jointIndex);

        dataOut.position = MyAvatar.worldToJointPoint(dataIn.position, jointIndex);
        dataOut.perpendicular = MyAvatar.worldToJointPoint(Vec3.sum(worldPosHand, dataIn.perpendicular), jointIndex);
        dataOut.distance = dataIn.distance;
        for (var i = 0; i < fingerKeys.length; i++) {
            var finger = fingerKeys[i];
            dataOut.fingers[finger] = MyAvatar.worldToJointPoint(dataIn.fingers[finger], jointIndex);
        }
    }

    // Calculate touch field; Sphere at the center of the palm,
    // perpendicular vector from the palm plane and origin of the the finger rays
    function estimatePalmData(side) {
        // Return data object
        var data = new Palm();

        var jointOffset = { x: 0, y: 0, z: 0 };

        var upperSide = side[0].toUpperCase() + side.substring(1);
        var jointIndexHand = MyAvatar.getJointIndex(upperSide + "Hand");

        // Store position of the hand joint
        var worldPosHand = MyAvatar.jointToWorldPoint(jointOffset, jointIndexHand);
        var minusWorldPosHand = {x: -worldPosHand.x, y: -worldPosHand.y, z: -worldPosHand.z};

        // Data for finger rays
        var directions = {pinky: undefined, middle: undefined, ring: undefined, thumb: undefined, index: undefined};
        var positions = {pinky: undefined, middle: undefined, ring: undefined, thumb: undefined, index: undefined};

        var thumbLength = 0;
        var weightCount = 0;

        // Calculate palm center
        var handJointWeight = 1;
        var fingerJointWeight = 2;

        var palmCenter = {x: 0, y: 0, z: 0};
        palmCenter = Vec3.sum(worldPosHand, palmCenter);

        weightCount += handJointWeight;

        for (var i = 0; i < fingerKeys.length; i++) {
            var finger = fingerKeys[i];
            var jointSuffixes = 4; // Get 4 joint names with suffix numbers (0, 1, 2, 3)
            var jointNames = getJointNames(side, finger, jointSuffixes);
            var fingerLength = getJointDistances(jointNames).totalDistance;

            var jointIndex = MyAvatar.getJointIndex(jointNames[0]);
            positions[finger] = MyAvatar.jointToWorldPoint(jointOffset, jointIndex);
            directions[finger] = Vec3.normalize(Vec3.sum(positions[finger], minusWorldPosHand));
            data.fingers[finger] = Vec3.sum(positions[finger], Vec3.multiply(fingerLength, directions[finger]));
            if (finger !== "thumb") {
                // finger joints have double the weight than the hand joint
                // This would better position the palm estimation

                palmCenter = Vec3.sum(Vec3.multiply(fingerJointWeight, positions[finger]), palmCenter);
                weightCount += fingerJointWeight;
            } else {
                thumbLength = fingerLength;
            }
        }

        // perpendicular change direction depending on the side
        data.perpendicular = (side === "right") ?
            Vec3.normalize(Vec3.cross(directions.index, directions.pinky)):
            Vec3.normalize(Vec3.cross(directions.pinky, directions.index));

        data.position = Vec3.multiply(1.0/weightCount, palmCenter);

        if (side === "right") {
            varsToDebug.offset = MyAvatar.worldToJointPoint(worldPosHand, jointIndexHand);
        }

        var palmDistanceMultiplier = 1.55; // 1.55 based on test/error for the sphere radius that best fits the hand
        data.distance = palmDistanceMultiplier*Vec3.distance(data.position, positions.index);

        // move back thumb ray origin
        var thumbBackMultiplier = 0.2;
        data.fingers.thumb = Vec3.sum(
            data.fingers.thumb, Vec3.multiply( -thumbBackMultiplier * thumbLength, data.perpendicular));

        // return getDataRelativeToHandJoint(side, data);
        dataRelativeToHandJoint(side, data, palmData[side]);
        palmData[side].set = true;
    }

    // Register GlobalDebugger for API Debugger
    Script.registerValue("GlobalDebugger", varsToDebug);

    // store the rays for the fingers - only for debug purposes
    var fingerRays = {
        left: {
            pinky: undefined,
            middle: undefined,
            ring: undefined,
            thumb: undefined,
            index: undefined
        },
        right: {
            pinky: undefined,
            middle: undefined,
            ring: undefined,
            thumb: undefined,
            index: undefined
        }
    };

    // Create debug overlays - finger rays + palm rays + spheres
    var palmRay, sphereHand;

    function createDebugLines() {
        for (var i = 0; i < fingerKeys.length; i++) {
            fingerRays.left[fingerKeys[i]] = Overlays.addOverlay("line3d", {
                color: { red: 0, green: 0, blue: 255 },
                start: { x: 0, y: 0, z: 0 },
                end: { x: 0, y: 1, z: 0 },
                visible: showLines
            });
            fingerRays.right[fingerKeys[i]] = Overlays.addOverlay("line3d", {
                color: { red: 0, green: 0, blue: 255 },
                start: { x: 0, y: 0, z: 0 },
                end: { x: 0, y: 1, z: 0 },
                visible: showLines
            });
        }

        palmRay = {
            left: Overlays.addOverlay("line3d", {
                color: { red: 255, green: 0, blue: 0 },
                start: { x: 0, y: 0, z: 0 },
                end: { x: 0, y: 1, z: 0 },
                visible: showLines
            }),
            right: Overlays.addOverlay("line3d", {
                color: { red: 255, green: 0, blue: 0 },
                start: { x: 0, y: 0, z: 0 },
                end: { x: 0, y: 1, z: 0 },
                visible: showLines
            })
        };
        linesCreated = true;
    }

    function createDebugSphere() {
        sphereHand = {
            right: Overlays.addOverlay("sphere", {
                position: MyAvatar.position,
                color: { red: 0, green: 255, blue: 0 },
                scale: { x: 0.01, y: 0.01, z: 0.01 },
                visible: showSphere
            }),
            left: Overlays.addOverlay("sphere", {
                position: MyAvatar.position,
                color: { red: 0, green: 255, blue: 0 },
                scale: { x: 0.01, y: 0.01, z: 0.01 },
                visible: showSphere
            })
        };
        sphereCreated = true;
    }

    function acquireDefaultPose(side) {
        for (var i = 0; i < fingerKeys.length; i++) {
            var finger = fingerKeys[i];
            var jointSuffixes = 3; // We need rotation of the 0, 1 and 2 joints
            var names = getJointNames(side, finger, jointSuffixes);
            for (var j = 0; j < names.length; j++) {
                var index = MyAvatar.getJointIndex(names[j]);
                var rotation = MyAvatar.getJointRotation(index);
                dataDefault[side][finger][j] = dataCurrent[side][finger][j] = rotation;
            }
        }
        dataDefault[side].set = true;
    }

    var rayPicks = {
        left: {
            pinky: undefined,
            middle: undefined,
            ring: undefined,
            thumb: undefined,
            index: undefined
        },
        right: {
            pinky: undefined,
            middle: undefined,
            ring: undefined,
            thumb: undefined,
            index: undefined
        }
    };

    var dataFailed = {
        left: {
            pinky: 0,
            middle: 0,
            ring: 0,
            thumb: 0,
            index: 0
        },
        right: {
            pinky: 0,
            middle: 0,
            ring: 0,
            thumb: 0,
            index: 0
        }
    };

    function clearRayPicks(side) {
        for (var i = 0; i < fingerKeys.length; i++) {
            var finger = fingerKeys[i];
            if (rayPicks[side][finger] !== undefined) {
                RayPick.removeRayPick(rayPicks[side][finger]);
                rayPicks[side][finger] = undefined;
            }
        }
    }

    function createRayPicks(side) {
        var data = palmData[side];
        clearRayPicks(side);
        for (var i = 0; i < fingerKeys.length; i++) {
            var finger = fingerKeys[i];
            var LOOKUP_DISTANCE_MULTIPLIER = 1.5;
            var dist = LOOKUP_DISTANCE_MULTIPLIER*data.distance;
            var checkOffset = {
                x: data.perpendicular.x * dist,
                y: data.perpendicular.y * dist,
                z: data.perpendicular.z * dist
            };

            var checkPoint = Vec3.sum(data.position, Vec3.multiply(2, checkOffset));
            var sensorToWorldScale = MyAvatar.getSensorToWorldScale();

            var origin = data.fingers[finger];

            var direction = Vec3.normalize(Vec3.subtract(checkPoint, origin));

            origin = Vec3.multiply(1/sensorToWorldScale, origin);

            rayPicks[side][finger] = RayPick.createRayPick(
                {
                    "enabled": false,
                    "joint": handJointNames[side],
                    "posOffset": origin,
                    "dirOffset": direction,
                    "filter": RayPick.PICK_ENTITIES
                }
            );

            RayPick.setPrecisionPicking(rayPicks[side][finger], true);
        }
    }

    function activateNextRay(side, index) {
        var nextIndex = (index < fingerKeys.length-1) ? index + 1 : 0;
        for (var i = 0; i < fingerKeys.length; i++) {
            var finger = fingerKeys[i];
            if (i === nextIndex) {
                RayPick.enableRayPick(rayPicks[side][finger]);
            } else {
                RayPick.disableRayPick(rayPicks[side][finger]);
            }
        }
    }

    function updateSphereHand(side) {
        var data = new Palm();
        dataRelativeToWorld(side, palmData[side], data);
        varsToDebug.palmData[side] = palmData[side];

        var palmPoint = data.position;
        var LOOKUP_DISTANCE_MULTIPLIER = 1.5;
        var dist = LOOKUP_DISTANCE_MULTIPLIER*data.distance;

        // Situate the debugging overlays
        var checkOffset = {
            x: data.perpendicular.x * dist,
            y: data.perpendicular.y * dist,
            z: data.perpendicular.z * dist
        };

        var spherePos = Vec3.sum(palmPoint, checkOffset);
        var checkPoint = Vec3.sum(palmPoint, Vec3.multiply(2, checkOffset));

        if (showLines) {
            Overlays.editOverlay(palmRay[side], {
                start: palmPoint,
                end: checkPoint,
                visible: showLines
            });
            for (var i = 0; i < fingerKeys.length; i++) {
                Overlays.editOverlay(fingerRays[side][fingerKeys[i]], {
                    start: data.fingers[fingerKeys[i]],
                    end: checkPoint,
                    visible: showLines
                });
            }
        }

        if (showSphere) {
            Overlays.editOverlay(sphereHand[side], {
                position: spherePos,
                scale: {
                    x: 2*dist,
                    y: 2*dist,
                    z: 2*dist
                },
                visible: showSphere
            });
        }

        // Update the intersection of only one finger at a time
        var finger = fingerKeys[updateFingerWithIndex];
        var nearbyEntities = Entities.findEntities(spherePos, dist);
        // Filter the entities that are allowed to be touched
        var touchableEntities = nearbyEntities.filter(function (id) {
            return untouchableEntities.indexOf(id) == -1;
        });
        var intersection;
        if (rayPicks[side][finger] !== undefined) {
            intersection = RayPick.getPrevRayPickResult(rayPicks[side][finger]);
        }

        var animationSteps = defaultAnimationSteps;
        var newFingerData = dataDefault[side][finger];
        var isAbleToGrab = false;
        if (touchableEntities.length > 0) {
            RayPick.setIncludeItems(rayPicks[side][finger], touchableEntities);

            if (intersection === undefined) {
                return;
            }

            var percent = 0; // Initialize
            isAbleToGrab = intersection.intersects && intersection.distance < LOOKUP_DISTANCE_MULTIPLIER*dist;
            if (isAbleToGrab && !getTouching(side)) {
                acquireDefaultPose(side); // take a snapshot of the default pose before touch starts
                newFingerData = dataDefault[side][finger]; // assign default pose to finger data
            }
            // Store if this finger is touching something
            isTouching[side][finger] = isAbleToGrab;
            if (isAbleToGrab) {
                // update the open/close percentage for this finger
                var FINGER_REACT_MULTIPLIER = 2.8;

                percent = intersection.distance/(FINGER_REACT_MULTIPLIER*dist);

                var THUMB_FACTOR = 0.2;
                var FINGER_FACTOR = 0.05;

                // Amount of grab coefficient added to the fingers - thumb is higher
                var grabMultiplier = finger === "thumb" ? THUMB_FACTOR : FINGER_FACTOR;
                percent += grabMultiplier * grabPercent[side];

                // Calculate new interpolation data
                var totalDistance = addVals(dataClose[side][finger], dataOpen[side][finger], -1);
                // Assign close/open ratio to finger to simulate touch
                newFingerData = addVals(dataOpen[side][finger], multiplyValsBy(totalDistance, percent), 1);
                animationSteps = touchAnimationSteps;
            }
            varsToDebug.fingerPercent[side][finger] = percent;

        }
        if (!isAbleToGrab) {
            dataFailed[side][finger] = dataFailed[side][finger] === 0 ? 1 : 2;
        } else {
            dataFailed[side][finger] = 0;
        }
        // If it only fails once it will not update increments
        if (dataFailed[side][finger] !== 1) {
            // Calculate animation increments
            dataDelta[side][finger] =
                multiplyValsBy(addVals(newFingerData, dataCurrent[side][finger], -1), 1.0/animationSteps);
        }
    }

    // Recreate the finger joint names
    function getJointNames(side, finger, count) {
        var names = [];
        for (var i = 1; i < count+1; i++) {
            var name = side[0].toUpperCase()+side.substring(1)+"Hand"+finger[0].toUpperCase()+finger.substring(1)+(i);
            names.push(name);
        }
        return names;
    }

    // Capture the controller values
    var leftTriggerPress = function (value) {
        varsToDebug.triggerValues.leftTriggerValue = value;
        // the value for the trigger increments the hand-close percentage
        grabPercent.left = value;
    };

    var leftTriggerClick = function (value) {
        varsToDebug.triggerValues.leftTriggerClicked = value;
    };

    var rightTriggerPress = function (value) {
        varsToDebug.triggerValues.rightTriggerValue = value;
        // the value for the trigger increments the hand-close percentage
        grabPercent.right = value;
    };

    var rightTriggerClick = function (value) {
        varsToDebug.triggerValues.rightTriggerClicked = value;
    };

    var leftSecondaryPress = function (value) {
        varsToDebug.triggerValues.leftSecondaryValue = value;
    };

    var rightSecondaryPress = function (value) {
        varsToDebug.triggerValues.rightSecondaryValue = value;
    };

    var MAPPING_NAME = "com.highfidelity.handTouch";
    var mapping = Controller.newMapping(MAPPING_NAME);
    mapping.from([Controller.Standard.RT]).peek().to(rightTriggerPress);
    mapping.from([Controller.Standard.RTClick]).peek().to(rightTriggerClick);
    mapping.from([Controller.Standard.LT]).peek().to(leftTriggerPress);
    mapping.from([Controller.Standard.LTClick]).peek().to(leftTriggerClick);

    mapping.from([Controller.Standard.RB]).peek().to(rightSecondaryPress);
    mapping.from([Controller.Standard.LB]).peek().to(leftSecondaryPress);
    mapping.from([Controller.Standard.LeftGrip]).peek().to(leftSecondaryPress);
    mapping.from([Controller.Standard.RightGrip]).peek().to(rightSecondaryPress);

    Controller.enableMapping(MAPPING_NAME);

    if (showLines && !linesCreated) {
        createDebugLines();
        linesCreated = true;
    }

    if (showSphere && !sphereCreated) {
        createDebugSphere();
        sphereCreated = true;
    }

    function getTouching(side) {
        var animating = false;
        for (var i = 0; i < fingerKeys.length; i++) {
            var finger = fingerKeys[i];
            animating = animating || isTouching[side][finger];
        }
        return animating; // return false only if none of the fingers are touching
    }

    function reEstimatePalmData() {
        ["right", "left"].forEach(function(side) {
            estimatePalmData(side);
        });
    }

    function recreateRayPicks() {
        ["right", "left"].forEach(function(side) {
            createRayPicks(side);
        });
    }

    function cleanUp() {
        ["right", "left"].forEach(function (side) {
            if (linesCreated) {
                Overlays.deleteOverlay(palmRay[side]);
            }
            if (sphereCreated) {
                Overlays.deleteOverlay(sphereHand[side]);
            }
            clearRayPicks(side);
            for (var i = 0; i < fingerKeys.length; i++) {
                var finger = fingerKeys[i];
                var jointSuffixes = 3; // We need to clear the joints 0, 1 and 2 joints
                var names = getJointNames(side, finger, jointSuffixes);
                for (var j = 0; j < names.length; j++) {
                    var index = MyAvatar.getJointIndex(names[j]);
                    MyAvatar.clearJointData(index);
                }
                if (linesCreated) {
                    Overlays.deleteOverlay(fingerRays[side][finger]);
                }
            }
        });
    }

    MyAvatar.shouldDisableHandTouchChanged.connect(function (shouldDisable) {
        if (shouldDisable) {
            if (handTouchEnabled) {
                cleanUp();
            }
        } else {
            if (!handTouchEnabled) {
                reEstimatePalmData();
                recreateRayPicks();
            }
        }
        handTouchEnabled = !shouldDisable;
    });

    Controller.inputDeviceRunningChanged.connect(function (deviceName, isEnabled) {
        if (deviceName == LEAP_MOTION_NAME) {
            leapMotionEnabled = isEnabled;
        }
    });

    MyAvatar.disableHandTouchForIDChanged.connect(function (entityID, disable) {
        var entityIndex = untouchableEntities.indexOf(entityID);
        if (disable) {
            if (entityIndex == -1) {
                untouchableEntities.push(entityID);
            }
        } else {
            if (entityIndex != -1) {
                untouchableEntities.splice(entityIndex, 1);
            }
        }
    });

    MyAvatar.onLoadComplete.connect(function () {
        // Sometimes the rig is not ready when this signal is trigger
        console.log("avatar loaded");
        Script.setTimeout(function() {
            reEstimatePalmData();
            recreateRayPicks();
        }, MSECONDS_AFTER_LOAD);
    });

    MyAvatar.sensorToWorldScaleChanged.connect(function() {
        reEstimatePalmData();
    });

    Script.scriptEnding.connect(function () {
        cleanUp();
    });

    Script.update.connect(function () {

        if (!handTouchEnabled || leapMotionEnabled) {
            return;
        }

        // index of the finger that needs to be updated this frame
        updateFingerWithIndex = (updateFingerWithIndex < fingerKeys.length-1) ? updateFingerWithIndex + 1 : 0;

        ["right", "left"].forEach(function(side) {

            if (!palmData[side].set) {
                reEstimatePalmData();
                recreateRayPicks();
            }

            // recalculate the base data
            updateSphereHand(side);
            activateNextRay(side, updateFingerWithIndex);

            // this vars manage the transition to default pose
            var isHandTouching = getTouching(side);
            countToDefault[side] = isHandTouching ? 0 : countToDefault[side] + 1;

            for (var i = 0; i < fingerKeys.length; i++) {
                var finger = fingerKeys[i];
                var jointSuffixes = 3; // We need to update rotation of the 0, 1 and 2 joints
                var names = getJointNames(side, finger, jointSuffixes);

                // Add the animation increments
                dataCurrent[side][finger] = addVals(dataCurrent[side][finger], dataDelta[side][finger], 1);

                // update every finger joint
                for (var j = 0; j < names.length; j++) {
                    var index = MyAvatar.getJointIndex(names[j]);
                    // if no finger is touching restate the default poses
                    if (isHandTouching || (dataDefault[side].set &&
                    countToDefault[side] < fingerKeys.length*touchAnimationSteps)) {
                        var quatRot = dataCurrent[side][finger][j];
                        MyAvatar.setJointRotation(index, quatRot);
                    } else {
                        MyAvatar.clearJointData(index);
                    }
                }
            }
        });
    });
}());
