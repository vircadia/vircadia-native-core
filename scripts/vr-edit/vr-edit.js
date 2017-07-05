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

        Highlights,
        Selection,
        Laser,
        Hand,

        AVATAR_SELF_ID = "{00000000-0000-0000-0000-000000000001}",
        NULL_UUID = "{00000000-0000-0000-0000-000000000000}";

    Highlights = function () {
        // Draws highlights on selected entities.

        var overlays = [],
            HIGHLIGHT_COLOR = { red: 240, green: 240, blue: 0 },
            HIGHLIGHT_ALPHA = 0.8;

        function maybeAddOverlay(index) {
            if (index >= overlays.length) {
                overlays.push(Overlays.addOverlay("cube", {
                    color: HIGHLIGHT_COLOR,
                    alpha: HIGHLIGHT_ALPHA,
                    solid: false,
                    drawInFront: true,
                    ignoreRayIntersection: true,
                    visible: false
                }));
            }
        }

        function editOverlay(index, details) {
            Overlays.editOverlay(overlays[index], {
                position: details.position,
                registrationPoint: details.registrationPoint,
                rotation: details.rotation,
                dimensions: details.dimensions,
                visible: true
            });
        }

        function display(selection) {
            var i,
                length;

            // Add/edit overlay.
            for (i = 0, length = selection.length; i < length; i += 1) {
                maybeAddOverlay(i);
                editOverlay(i, selection[i]);
            }

            // Delete extra overlays.
            for (i = overlays.length - 1, length = selection.length; i >= length; i -= 1) {
                Overlays.deleteOverlay(overlays[i]);
                overlays.splice(i, 1);
            }
        }

        function clear() {
            var i,
                length;

            for (i = 0, length = overlays.length; i < length; i += 1) {
                Overlays.deleteOverlay(overlays[i]);
            }
            overlays = [];
        }

        function destroy() {
            clear();
        }

        if (!this instanceof Highlights) {
            return new Highlights();
        }

        return {
            display: display,
            clear: clear,
            destroy: destroy
        };
    };

    Selection = function () {
        // Manages set of selected entities. Currently supports just one set of linked entities.

        var selection = [],
            selectedEntityID = null,
            selectionPosition = null,
            selectionOrientation,
            rootEntityID;

        function traverseEntityTree(id, result) {
            // Recursively traverses tree of entities and their children, gather IDs and properties.
            var children,
                properties,
                SELECTION_PROPERTIES = ["position", "registrationPoint", "rotation", "dimensions"],
                i,
                length;

            properties = Entities.getEntityProperties(id, SELECTION_PROPERTIES);
            result.push({
                id: id,
                position: properties.position,
                registrationPoint: properties.registrationPoint,
                rotation: properties.rotation,
                dimensions: properties.dimensions
            });

            children = Entities.getChildrenIDs(id);
            for (i = 0, length = children.length; i < length; i += 1) {
                traverseEntityTree(children[i], result);
            }
        }

        function select(entityID) {
            var entityProperties,
                PARENT_PROPERTIES = ["parentID", "position", "rotation"];

            // Find root parent.
            rootEntityID = entityID;
            entityProperties = Entities.getEntityProperties(rootEntityID, PARENT_PROPERTIES);
            while (entityProperties.parentID !== NULL_UUID) {
                rootEntityID = entityProperties.parentID;
                entityProperties = Entities.getEntityProperties(rootEntityID, PARENT_PROPERTIES);
            }

            // Selection position and orientation is that of the root entity.
            selectionPosition = entityProperties.position;
            selectionOrientation = entityProperties.rotation;

            // Find all children.
            selection = [];
            traverseEntityTree(rootEntityID, selection);

            selectedEntityID = entityID;
        }

        function getSelection() {
            return selection;
        }

        function getPositionAndOrientation() {
            return {
                position: selectionPosition,
                orientation: selectionOrientation
            };
        }

        function setPositionAndOrientation(position, orientation) {
            selectionPosition = position;
            selectionOrientation = orientation;
            Entities.editEntity(rootEntityID, {
                position: position,
                rotation: orientation
            });
        }

        function clear() {
            selection = [];
            selectedEntityID = null;
            rootEntityID = null;
        }

        function destroy() {
            clear();
        }

        if (!this instanceof Selection) {
            return new Selection();
        }

        return {
            select: select,
            selection: getSelection,
            getPositionAndOrientation: getPositionAndOrientation,
            setPositionAndOrientation: setPositionAndOrientation,
            clear: clear,
            destroy: destroy
        };
    };

    Laser = function (side) {
        // Draws hand lasers.
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
        // Hand controller input.
        // Each hand has a laser, an entity selection, and entity highlighter.

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
            laseredEntityID = null,

            isEditing = false,
            initialHandPosition,
            initialHandOrientationInverse,
            initialHandToSelectionVector,
            initialSelectionOrientation,
            editingDistance,

            laser,
            selection,
            highlights;

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
        selection = new Selection();
        highlights = new Highlights();

        function startEditing(handPose) {
            var selectionPositionAndOrientation;

            initialHandPosition = Vec3.sum(Vec3.multiplyQbyV(MyAvatar.orientation, handPose.translation), MyAvatar.position);
            initialHandOrientationInverse = Quat.inverse(Quat.multiply(MyAvatar.orientation, handPose.rotation));

            selectionPositionAndOrientation = selection.getPositionAndOrientation();
            initialHandToSelectionVector = Vec3.subtract(selectionPositionAndOrientation.position, initialHandPosition);
            initialSelectionOrientation = selectionPositionAndOrientation.orientation;

            isEditing = true;
        }

        function applyEdit(handPose) {
            var handPosition,
                handOrientation,
                deltaOrientation,
                selectionPosition,
                selectionOrientation;

            handPosition = Vec3.sum(Vec3.multiplyQbyV(MyAvatar.orientation, handPose.translation), MyAvatar.position);
            handOrientation = Quat.multiply(MyAvatar.orientation, handPose.rotation);

            deltaOrientation = Quat.multiply(handOrientation, initialHandOrientationInverse);
            selectionPosition = Vec3.sum(handPosition, Vec3.multiplyQbyV(deltaOrientation, initialHandToSelectionVector));
            selectionOrientation = Quat.multiply(deltaOrientation, initialSelectionOrientation);

            selection.setPositionAndOrientation(selectionPosition, selectionOrientation);
        }

        function stopEditing() {
            isEditing = false;
        }

        function update() {
            var wasLaserOn,
                handPose,
                handPosition,
                handOrientation,
                deltaOrigin,
                pickRay,
                intersection,
                distance,
                isTriggerClicked;

            // Controller trigger.
            wasLaserOn = isLaserOn;
            isLaserOn = Controller.getValue(controllerTrigger) > (isLaserOn ? TRIGGER_OFF_VALUE : TRIGGER_ON_VALUE);
            if (!isLaserOn) {
                if (wasLaserOn) {
                    laser.clear();
                    selection.clear();
                    highlights.clear();
                    laseredEntityID = null;
                }
                return;
            }

            // Hand position and orientation.
            handPose = Controller.getPoseValue(handController);
            if (!handPose.valid) {
                isLaserOn = false;
                if (wasLaserOn) {
                    laser.clear();
                    selection.clear();
                    highlights.clear();
                    laseredEntityID = null;
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
            distance = isEditing ? editingDistance : (intersection.intersects ? intersection.distance : PICK_MAX_DISTANCE);

            // Laser, hover, edit.
            isTriggerClicked = Controller.getValue(controllerTriggerClicked);
            laser.update(pickRay.origin, pickRay.direction, distance, isTriggerClicked);

            if (isTriggerClicked) {
                if (isEditing) {
                    // Perform edit.
                    applyEdit(handPose);
                } else if (intersection.intersects) {
                    // Start editing.
                    if (intersection.entityID !== laseredEntityID) {
                        laseredEntityID = intersection.entityID;
                        selection.select(laseredEntityID);
                    } else {
                        highlights.clear();
                    }
                    editingDistance = distance;
                    startEditing(handPose);
                }
            } else {
                if (isEditing) {
                    // Stop editing.
                    stopEditing();
                    laseredEntityID = null;  // Force highlighting entities at their new position.
                }

                if (intersection.intersects) {
                    // Hover entities.
                    if (intersection.entityID !== laseredEntityID) {
                        laseredEntityID = intersection.entityID;
                        selection.select(laseredEntityID);
                        highlights.display(selection.selection());
                    }
                } else {
                    // Unhover entities.
                    if (laseredEntityID) {
                        selection.clear();
                        highlights.clear();
                        laseredEntityID = null;
                    }
                }

            }
        }

        function destroy() {
            if (laser) {
                laser.destroy();
                laser = null;
            }
            if (selection) {
                selection.destroy();
                selection = null;
            }
            if (highlights) {
                highlights.destroy();
                highlights = null;
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
        // Communicate app status to handControllerGrab.js.
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
