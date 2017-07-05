"use strict";

//
//  vr-edit.js
//
//  Created by David Rowe on 27 Jun 2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function () {

    var APP_NAME = "VR EDIT",  // TODO: App name.
        APP_ICON_INACTIVE = "icons/tablet-icons/edit-i.svg",  // TODO: App icons.
        APP_ICON_ACTIVE = "icons/tablet-icons/edit-a.svg",
        tablet,
        button,
        isAppActive = false,

        VR_EDIT_SETTING = "io.highfidelity.isVREditing",  // Note: This constant is duplicated in utils.js.

        hands = [],
        LEFT_HAND = 0,
        RIGHT_HAND = 1,

        UPDATE_LOOP_TIMEOUT = 16,
        updateTimer = null,

        Hand,
        Laser,

        AVATAR_SELF_ID = "{00000000-0000-0000-0000-000000000001}";

    Laser = function (side) {
        // May intersect with entities or bounding box of other hand's selection.

        var hand,
            laserLine = null,
            laserSphere = null,

            searchDistance = 0.0,

            SEARCH_SPHERE_SIZE = 0.013,  // Per handControllerGrab.js multiplied by 1.2 per handControllerGrab.js.
            SEARCH_SPHERE_FOLLOW_RATE = 0.5,  // Per handControllerGrab.js.
            COLORS_GRAB_SEARCHING_HALF_SQUEEZE = { red: 10, green: 10, blue: 255 },  // Per handControllgerGrab.js.
            COLORS_GRAB_SEARCHING_FULL_SQUEEZE = { red: 250, green: 10, blue: 10 };  // Per handControllgerGrab.js.

        hand = side;
        laserLine = Overlays.addOverlay("line3d", {
            lineWidth: 5,
            alpha: 1.0,
            glow: 1.0,
            ignoreRayIntersection: true,
            drawInFront: true,
            parentID: AVATAR_SELF_ID,
            parentJointIndex: MyAvatar.getJointIndex(hand === LEFT_HAND
                ? "_CAMERA_RELATIVE_CONTROLLER_LEFTHAND"
                : "_CAMERA_RELATIVE_CONTROLLER_RIGHTHAND"),
            visible: false
        });
        laserSphere = Overlays.addOverlay("circle3d", {
            innerAlpha: 1.0,
            outerAlpha: 0.0,
            solid: true,
            ignoreRayIntersection: true,
            drawInFront: true,
            visible: false
        });

        function colorPow(color, power) {
            return {
                red: Math.pow(color.red / 255, power) * 255,
                green: Math.pow(color.green / 255, power) * 255,
                blue: Math.pow(color.blue / 255, power) * 255
            };
        }

        function updateLine(start, end, color) {
            Overlays.editOverlay(laserLine, {
                start: start,
                end: end,
                color: color,
                visible: true
            });
        }

        function updateSphere(location, size, color) {
            var rotation,
                brightColor;

            rotation = Quat.lookAt(location, Camera.getPosition(), Vec3.UP);
            brightColor = colorPow(color, 0.06);

            Overlays.editOverlay(laserSphere, {
                position: location,
                rotation: rotation,
                innerColor: brightColor,
                outerColor: color,
                outerRadius: size,
                visible: true
            });
        }

        function update(origin, direction, distance, isClicked) {
            var searchTarget,
                sphereSize,
                color;

            searchDistance = SEARCH_SPHERE_FOLLOW_RATE * searchDistance + (1.0 - SEARCH_SPHERE_FOLLOW_RATE) * distance;
            searchTarget = Vec3.sum(origin, Vec3.multiply(searchDistance, direction));
            sphereSize = SEARCH_SPHERE_SIZE * searchDistance;
            color = isClicked ? COLORS_GRAB_SEARCHING_FULL_SQUEEZE : COLORS_GRAB_SEARCHING_HALF_SQUEEZE;

            updateLine(origin, searchTarget, color);
            updateSphere(searchTarget, sphereSize, color);
        }

        function clear() {
            Overlays.editOverlay(laserLine, { visible: false });
            Overlays.editOverlay(laserSphere, { visible: false });
        }

        function destroy() {
            Overlays.deleteOverlay(laserLine);
            Overlays.deleteOverlay(laserSphere);
        }

        if (!this instanceof Laser) {
            return new Laser();
        }

        return {
            update: update,
            clear: clear,
            destroy: destroy
        };
    };

    Hand = function (side) {
        var hand,
            handController,
            controllerTrigger,
            controllerTriggerClicked,

            TRIGGER_ON_VALUE = 0.15,  // Per handControllerGrab.js.
            TRIGGER_OFF_VALUE = 0.1,  // Per handControllerGrab.js.
            GRAB_POINT_SPHERE_OFFSET = { x: 0.04, y: 0.13, z: 0.039 },  // Per HmdDisplayPlugin.cpp and controllers.js.

            PICK_MAX_DISTANCE = 500,  // Per handControllerGrab.js.
            PRECISION_PICKING = true,
            NO_INCLUDE_IDS = [],
            NO_EXCLUDE_IDS = [],
            VISIBLE_ONLY = true,

            isLaserOn = false,

            laser;

        hand = side;
        if (hand === LEFT_HAND) {
            handController = Controller.Standard.LeftHand;
            controllerTrigger = Controller.Standard.LT;
            controllerTriggerClicked = Controller.Standard.LTClick;
            GRAB_POINT_SPHERE_OFFSET.x = -GRAB_POINT_SPHERE_OFFSET.x;
        } else {
            handController = Controller.Standard.RightHand;
            controllerTrigger = Controller.Standard.RT;
            controllerTriggerClicked = Controller.Standard.RTClick;
        }

        laser = new Laser(hand);

        function update() {
            var wasLaserOn,
                handPose,
                handPosition,
                handOrientation,
                deltaOrigin,
                pickRay,
                intersection,
                distance;

            // Controller trigger.
            wasLaserOn = isLaserOn;
            isLaserOn = Controller.getValue(controllerTrigger) > (isLaserOn ? TRIGGER_OFF_VALUE : TRIGGER_ON_VALUE);
            if (!isLaserOn) {
                if (wasLaserOn) {
                    laser.clear();
                }
                return;
            }

            // Hand position and orientation.
            handPose = Controller.getPoseValue(handController);
            if (!handPose.valid) {
                isLaserOn = false;
                if (wasLaserOn) {
                    laser.clear();
                }
                return;
            }
            handPosition = Vec3.sum(Vec3.multiplyQbyV(MyAvatar.orientation, handPose.translation), MyAvatar.position);
            handOrientation = Quat.multiply(MyAvatar.orientation, handPose.rotation);

            // Entity intersection, if any.
            deltaOrigin = Vec3.multiplyQbyV(handOrientation, GRAB_POINT_SPHERE_OFFSET);
            pickRay = {
                origin: Vec3.sum(handPosition, deltaOrigin),  // Add a bit to ...
                direction: Quat.getUp(handOrientation),
                length: PICK_MAX_DISTANCE
            };
            intersection = Entities.findRayIntersection(pickRay, PRECISION_PICKING, NO_INCLUDE_IDS, NO_EXCLUDE_IDS,
                VISIBLE_ONLY);
            distance = intersection.intersects ? intersection.distance : PICK_MAX_DISTANCE;

            // Update laser.
            laser.update(pickRay.origin, pickRay.direction, distance, Controller.getValue(controllerTriggerClicked));
        }

        function destroy() {
            if (laser) {
                laser.destroy();
                laser = null;
            }
        }

        if (!this instanceof Hand) {
            return new Hand();
        }

        return {
            update: update,
            destroy: destroy
        };
    };


    function update() {
        // Main update loop.
        updateTimer = null;

        hands[LEFT_HAND].update();
        hands[RIGHT_HAND].update();

        updateTimer = Script.setTimeout(update, UPDATE_LOOP_TIMEOUT);
    }

    function updateHandControllerGrab() {
        // Communicate status to handControllerGrab.js.
        Settings.setValue(VR_EDIT_SETTING, isAppActive);
    }

    function onButtonClicked() {
        isAppActive = !isAppActive;
        updateHandControllerGrab();
        button.editProperties({ isActive: isAppActive });

        if (isAppActive) {
            update();
        } else {
            Script.clearTimeout(updateTimer);
            updateTimer = null;
        }
    }

    function setUp() {
        updateHandControllerGrab();

        tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
        if (!tablet) {
            return;
        }

        // Tablet/toolbar button.
        button = tablet.addButton({
            icon: APP_ICON_INACTIVE,
            activeIcon: APP_ICON_ACTIVE,
            text: APP_NAME,
            isActive: isAppActive
        });
        if (button) {
            button.clicked.connect(onButtonClicked);
        }

        // Hands, each with a laser, selection, etc.
        hands[LEFT_HAND] = new Hand(LEFT_HAND);
        hands[RIGHT_HAND] = new Hand(RIGHT_HAND);

        if (isAppActive) {
            update();
        }
    }

    function tearDown() {
        if (updateTimer) {
            Script.clearTimeout(updateTimer);
        }

        isAppActive = false;
        updateHandControllerGrab();

        if (!tablet) {
            return;
        }

        if (button) {
            button.clicked.disconnect(onButtonClicked);
            tablet.removeButton(button);
            button = null;
        }

        if (hands[LEFT_HAND]) {
            hands[LEFT_HAND].destroy();
            hands[LEFT_HAND] = null;
        }
        if (hands[RIGHT_HAND]) {
            hands[RIGHT_HAND].destroy();
            hands[RIGHT_HAND] = null;
        }

        tablet = null;
    }

    setUp();
    Script.scriptEnding.connect(tearDown);
}());
