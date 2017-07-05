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

        VR_EDIT_SETTING = "io.highfidelity.isVREditing";  // Note: This constant is duplicated in utils.js.

        hands = [],
        LEFT_HAND = 0,
        RIGHT_HAND = 1,

        UPDATE_LOOP_TIMEOUT = 16,
        updateTimer = null,

        Hand;

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

            isLaserOn = false;

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
                    // Clear laser
                }
                return;
            }

            // Hand position and orientation.
            handPose = Controller.getPoseValue(handController);
            if (!handPose.valid) {
                isLaserOn = false;
                if (wasLaserOn) {
                    // Clear laser
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
        }

        function destroy() {
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
