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

        // Application state
        isAppActive = false,
        isScaleWithHandles = false,

        VR_EDIT_SETTING = "io.highfidelity.isVREditing",  // Note: This constant is duplicated in utils.js.

        hands = [],
        LEFT_HAND = 0,
        RIGHT_HAND = 1,

        UPDATE_LOOP_TIMEOUT = 16,
        updateTimer = null,

        Highlights,
        Handles,
        Selection,
        Laser,
        Hand,

        AVATAR_SELF_ID = "{00000000-0000-0000-0000-000000000001}",
        NULL_UUID = "{00000000-0000-0000-0000-000000000000}",
        HALF_TREE_SCALE = 16384;

    if (typeof Vec3.min !== "function") {
        Vec3.min = function (a, b) {
            return { x: Math.min(a.x, b.x), y: Math.min(a.y, b.y), z: Math.min(a.z, b.z) };
        };
    }

    if (typeof Vec3.max !== "function") {
        Vec3.max = function (a, b) {
            return { x: Math.max(a.x, b.x), y: Math.max(a.y, b.y), z: Math.max(a.z, b.z) };
        };
    }

    if (typeof Entities.rootOf !== "function") {
        Entities.rootOf = function (entityID) {
            var rootEntityID,
                entityProperties,
                PARENT_PROPERTIES = ["parentID"];
            rootEntityID = entityID;
            entityProperties = Entities.getEntityProperties(rootEntityID, PARENT_PROPERTIES);
            while (entityProperties.parentID !== NULL_UUID) {
                rootEntityID = entityProperties.parentID;
                entityProperties = Entities.getEntityProperties(rootEntityID, PARENT_PROPERTIES);
            }
            return rootEntityID;
        };
    }

    Highlights = function (hand) {
        // Draws highlights on selected entities.

        var handOverlay,
            entityOverlays = [],
            GRAB_HIGHLIGHT_COLOR = { red: 240, green: 240, blue: 0 },
            SCALE_HIGHLIGHT_COLOR = { red: 0, green: 240, blue: 240 },
            HAND_HIGHLIGHT_ALPHA = 0.35,
            ENTITY_HIGHLIGHT_ALPHA = 0.8,
            HAND_HIGHLIGHT_DIMENSIONS = { x: 0.2, y: 0.2, z: 0.2 },
            HAND_HIGHLIGHT_OFFSET = { x: 0.0, y: 0.11, z: 0.02 };

        handOverlay = Overlays.addOverlay("sphere", {
            dimensions: HAND_HIGHLIGHT_DIMENSIONS,
            parentID: AVATAR_SELF_ID,
            parentJointIndex: MyAvatar.getJointIndex(hand === LEFT_HAND
                ? "_CONTROLLER_LEFTHAND"
                : "_CONTROLLER_RIGHTHAND"),
            localPosition: HAND_HIGHLIGHT_OFFSET,
            alpha: HAND_HIGHLIGHT_ALPHA,
            solid: true,
            drawInFront: true,
            ignoreRayIntersection: true,
            visible: false
        });

        function maybeAddEntityOverlay(index) {
            if (index >= entityOverlays.length) {
                entityOverlays.push(Overlays.addOverlay("cube", {
                    alpha: ENTITY_HIGHLIGHT_ALPHA,
                    solid: false,
                    drawInFront: true,
                    ignoreRayIntersection: true,
                    visible: false
                }));
            }
        }

        function editEntityOverlay(index, details, overlayColor) {
            var offset = Vec3.multiplyQbyV(details.rotation,
                Vec3.multiplyVbyV(Vec3.subtract(Vec3.HALF, details.registrationPoint), details.dimensions));

            Overlays.editOverlay(entityOverlays[index], {
                parentID: details.id,
                position: Vec3.sum(details.position, offset),
                rotation: details.rotation,
                dimensions: details.dimensions,
                color: overlayColor,
                visible: true
            });
        }

        function display(handSelected, selection, isScale) {
            var overlayColor = isScale ? SCALE_HIGHLIGHT_COLOR : GRAB_HIGHLIGHT_COLOR,
                i,
                length;

            // Show/hide hand overlay.
            Overlays.editOverlay(handOverlay, {
                color: overlayColor,
                visible: handSelected
            });

            // Add/edit entity overlay.
            for (i = 0, length = selection.length; i < length; i += 1) {
                maybeAddEntityOverlay(i);
                editEntityOverlay(i, selection[i], overlayColor);
            }

            // Delete extra entity overlays.
            for (i = entityOverlays.length - 1, length = selection.length; i >= length; i -= 1) {
                Overlays.deleteOverlay(entityOverlays[i]);
                entityOverlays.splice(i, 1);
            }
        }

        function clear() {
            var i,
                length;

            // Hide hand overlay.
            Overlays.editOverlay(handOverlay, { visible: false });

            // Delete entity overlays.
            for (i = 0, length = entityOverlays.length; i < length; i += 1) {
                Overlays.deleteOverlay(entityOverlays[i]);
            }
            entityOverlays = [];
        }

        function destroy() {
            clear();
            Overlays.deleteOverlay(handOverlay);
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

    Handles = function () {
        var boundingBoxOverlay,
            cornerIndexes = [],
            cornerHandleOverlays = [],
            faceHandleOverlays = [],
            BOUNDING_BOX_COLOR = { red: 0, green: 240, blue: 240 },
            BOUNDING_BOX_ALPHA = 0.8,
            HANDLE_NORMAL_COLOR = { red: 0, green: 240, blue: 240 },
            HANDLE_HOVER_COLOR = { red: 0, green: 255, blue: 120 },
            HANDLE_ALPHA = 0.7,
            NUM_CORNERS = 8,
            NUM_CORNER_HANDLES = 2,
            CORNER_HANDLE_OVERLAY_DIMENSIONS = { x: 0.1, y: 0.1, z: 0.1 },
            CORNER_HANDLE_OVERLAY_AXES,
            NUM_FACE_HANDLES = 6,
            FACE_HANDLE_OVERLAY_DIMENSIONS = { x: 0.1, y: 0.12, z: 0.1 },
            FACE_HANDLE_OVERLAY_AXES,
            FACE_HANDLE_OVERLAY_OFFSETS,
            FACE_HANDLE_OVERLAY_ROTATIONS,
            ZERO_ROTATION = Quat.fromVec3Radians(Vec3.ZERO),
            DISTANCE_MULTIPLIER_MULTIPLIER = 0.5,
            hoveredOverlayID = null,
            i;

        CORNER_HANDLE_OVERLAY_AXES = [
            // Ordered such that items 4 apart are opposite corners - used in display().
            { x: -0.5, y: -0.5, z: -0.5 },
            { x: -0.5, y: -0.5, z:  0.5 },
            { x: -0.5, y:  0.5, z: -0.5 },
            { x: -0.5, y:  0.5, z:  0.5 },
            { x:  0.5, y:  0.5, z:  0.5 },
            { x:  0.5, y:  0.5, z: -0.5 },
            { x:  0.5, y: -0.5, z:  0.5 },
            { x:  0.5, y: -0.5, z: -0.5 }
        ];

        FACE_HANDLE_OVERLAY_AXES = [
            { x: -0.5, y:    0, z:    0 },
            { x:  0.5, y:    0, z:    0 },
            { x:    0, y: -0.5, z:    0 },
            { x:    0, y:  0.5, z:    0 },
            { x:    0, y:    0, z: -0.5 },
            { x:    0, y:    0, z:  0.5 }
        ];

        FACE_HANDLE_OVERLAY_OFFSETS = {
            x: FACE_HANDLE_OVERLAY_DIMENSIONS.y,
            y: FACE_HANDLE_OVERLAY_DIMENSIONS.y,
            z: FACE_HANDLE_OVERLAY_DIMENSIONS.y
        };

        FACE_HANDLE_OVERLAY_ROTATIONS = [
            Quat.fromVec3Degrees({ x:   0, y: 0, z:  90 }),
            Quat.fromVec3Degrees({ x:   0, y: 0, z: -90 }),
            Quat.fromVec3Degrees({ x: 180, y: 0, z:   0 }),
            Quat.fromVec3Degrees({ x:   0, y: 0, z:   0 }),
            Quat.fromVec3Degrees({ x: -90, y: 0, z:   0 }),
            Quat.fromVec3Degrees({ x:  90, y: 0, z:   0 })
        ];

        boundingBoxOverlay = Overlays.addOverlay("cube", {
            color: BOUNDING_BOX_COLOR,
            alpha: BOUNDING_BOX_ALPHA,
            solid: false,
            drawInFront: true,
            ignoreRayIntersection: true,
            visible: false
        });

        for (i = 0; i < NUM_CORNER_HANDLES; i += 1) {
            cornerHandleOverlays[i] = Overlays.addOverlay("sphere", {
                color: HANDLE_NORMAL_COLOR,
                alpha: HANDLE_ALPHA,
                solid: true,
                drawInFront: true,
                ignoreRayIntersection: false,
                visible: false
            });
        }

        for (i = 0; i < NUM_FACE_HANDLES; i += 1) {
            faceHandleOverlays[i] = Overlays.addOverlay("shape", {
                shape: "Cone",
                color: HANDLE_NORMAL_COLOR,
                alpha: HANDLE_ALPHA,
                solid: true,
                drawInFront: true,
                ignoreRayIntersection: false,
                visible: false
            });
        }

        function display(rootEntityID, boundingBox, isMultiple) {
            var boundingBoxDimensions = boundingBox.dimensions,
                boundingBoxCenter = boundingBox.center,
                boundingBoxLocalCenter = boundingBox.localCenter,
                boundingBoxOrientation = boundingBox.orientation,
                cameraPosition,
                boundingBoxVector,
                distanceMultiplier,
                cameraUp,
                cornerPosition,
                cornerVector,
                crossProductScale,
                maxCrossProductScale,
                rightCornerIndex,
                leftCornerIndex,
                cornerHandleDimensions,
                faceHandleDimensions,
                faceHandleOffsets,
                i;

            // Selection bounding box.
            Overlays.editOverlay(boundingBoxOverlay, {
                parentID: rootEntityID,
                localPosition: boundingBoxLocalCenter,
                localRotation: ZERO_ROTATION,
                dimensions: boundingBoxDimensions,
                visible: true
            });

            // Somewhat maintain general angular size of scale handles per bounding box center but make more distance ones 
            // display smaller in order to give comfortable depth cue.
            cameraPosition = Camera.position;
            boundingBoxVector = Vec3.subtract(boundingBox.center, Camera.position);
            distanceMultiplier = DISTANCE_MULTIPLIER_MULTIPLIER
                * Vec3.dot(Quat.getForward(Camera.orientation), boundingBoxVector)
                / Math.sqrt(Vec3.length(boundingBoxVector));

            // Corner scale handles.
            // At right-most and opposite corners of bounding box.
            cameraUp = Quat.getUp(Camera.orientation);
            maxCrossProductScale = 0;
            for (i = 0; i < NUM_CORNERS; i += 1) {
                cornerPosition = Vec3.sum(boundingBoxCenter,
                    Vec3.multiplyQbyV(boundingBoxOrientation,
                        Vec3.multiplyVbyV(CORNER_HANDLE_OVERLAY_AXES[i], boundingBoxDimensions)));
                cornerVector = Vec3.subtract(cornerPosition, cameraPosition);
                crossProductScale = Vec3.dot(Vec3.cross(cornerVector, boundingBoxVector), cameraUp);
                if (crossProductScale > maxCrossProductScale) {
                    maxCrossProductScale = crossProductScale;
                    rightCornerIndex = i;
                }
            }
            leftCornerIndex = (rightCornerIndex + 4) % NUM_CORNERS;
            cornerIndexes[0] = leftCornerIndex;
            cornerIndexes[1] = rightCornerIndex;
            cornerHandleDimensions = Vec3.multiply(distanceMultiplier, CORNER_HANDLE_OVERLAY_DIMENSIONS);
            for (i = 0; i < NUM_CORNER_HANDLES; i += 1) {
                Overlays.editOverlay(cornerHandleOverlays[i], {
                    parentID: rootEntityID,
                    localPosition: Vec3.sum(boundingBoxLocalCenter,
                        Vec3.multiplyVbyV(CORNER_HANDLE_OVERLAY_AXES[cornerIndexes[i]], boundingBoxDimensions)),
                    dimensions: cornerHandleDimensions,
                    visible: true
                });
            }

            // Face scale handles.
            // Only valid for a single entity because for multiple entities, some may be at an angle relative to the root entity
            // which would necessitate a (non-existent) shear transform be applied to them when scaling a face of the set.
            if (!isMultiple) {
                faceHandleDimensions = Vec3.multiply(distanceMultiplier, FACE_HANDLE_OVERLAY_DIMENSIONS);
                faceHandleOffsets = Vec3.multiply(distanceMultiplier, FACE_HANDLE_OVERLAY_OFFSETS);
                for (i = 0; i < NUM_FACE_HANDLES; i += 1) {
                    Overlays.editOverlay(faceHandleOverlays[i], {
                        parentID: rootEntityID,
                        localPosition: Vec3.sum(boundingBoxLocalCenter,
                            Vec3.multiplyVbyV(FACE_HANDLE_OVERLAY_AXES[i], Vec3.sum(boundingBoxDimensions, faceHandleOffsets))),
                        localRotation: FACE_HANDLE_OVERLAY_ROTATIONS[i],
                        dimensions: faceHandleDimensions,
                        visible: true
                    });
                }
            } else {
                for (i = 0; i < NUM_FACE_HANDLES; i += 1) {
                    Overlays.editOverlay(faceHandleOverlays[i], { visible: false });
                }
            }
        }

        function hover(overlayID) {
            if (overlayID !== hoveredOverlayID) {
                if (hoveredOverlayID !== null) {
                    Overlays.editOverlay(hoveredOverlayID, { color: HANDLE_NORMAL_COLOR });
                    hoveredOverlayID = null;
                }

                if (overlayID !== null
                        && (faceHandleOverlays.indexOf(overlayID) !== -1 || cornerHandleOverlays.indexOf(overlayID) !== -1)) {
                    Overlays.editOverlay(overlayID, { color: HANDLE_HOVER_COLOR });
                    hoveredOverlayID = overlayID;
                }
            }
        }

        function clear() {
            var i;

            Overlays.editOverlay(boundingBoxOverlay, { visible: false });
            for (i = 0; i < NUM_CORNER_HANDLES; i += 1) {
                Overlays.editOverlay(cornerHandleOverlays[i], { visible: false });
            }
            for (i = 0; i < NUM_FACE_HANDLES; i += 1) {
                Overlays.editOverlay(faceHandleOverlays[i], { visible: false });
            }
        }

        function destroy() {
            clear();
            Overlays.deleteOverlay(boundingBoxOverlay);
        }

        if (!this instanceof Handles) {
            return new Handles();
        }

        return {
            display: display,
            hover: hover,
            clear: clear,
            destroy: destroy
        };
    };

    Selection = function () {
        // Manages set of selected entities. Currently supports just one set of linked entities.
        var selection = [],
            selectedEntityID = null,
            rootEntityID = null,
            rootPosition,
            rootOrientation,
            scaleCenter,
            scaleRootOffset,
            scaleRootOrientation,
            ENTITY_TYPE = "entity";

        function traverseEntityTree(id, result) {
            // Recursively traverses tree of entities and their children, gather IDs and properties.
            var children,
                properties,
                SELECTION_PROPERTIES = ["position", "registrationPoint", "rotation", "dimensions", "localPosition"],
                i,
                length;

            properties = Entities.getEntityProperties(id, SELECTION_PROPERTIES);
            result.push({
                id: id,
                position: properties.position,
                localPosition: properties.localPosition,
                registrationPoint: properties.registrationPoint,
                rotation: properties.rotation,
                dimensions: properties.dimensions
            });

            children = Entities.getChildrenIDs(id);
            for (i = 0, length = children.length; i < length; i += 1) {
                if (Entities.getNestableType(children[i]) === ENTITY_TYPE) {
                    traverseEntityTree(children[i], result);
                }
            }
        }

        function select(entityID) {
            var entityProperties,
                PARENT_PROPERTIES = ["parentID", "position", "rotation"];

            // Find root parent.
            rootEntityID = Entities.rootOf(entityID);

            // Selection position and orientation is that of the root entity.
            entityProperties = Entities.getEntityProperties(rootEntityID, PARENT_PROPERTIES);
            rootPosition = entityProperties.position;
            rootOrientation = entityProperties.rotation;

            // Find all children.
            selection = [];
            traverseEntityTree(rootEntityID, selection);

            selectedEntityID = entityID;
        }

        function getRootEntityID() {
            return rootEntityID;
        }

        function getSelection() {
            return selection;
        }

        function count() {
            return selection.length;
        }

        function getBoundingBox() {
            var center,
                localCenter,
                orientation,
                inverseOrientation,
                dimensions,
                min,
                max,
                i,
                j,
                length,
                registration,
                position,
                rotation,
                corners = [],
                NUM_CORNERS = 8;

            if (selection.length === 1) {
                if (Vec3.equal(selection[0].registrationPoint, Vec3.HALF)) {
                    center = rootPosition;
                } else {
                    center = Vec3.sum(rootPosition,
                        Vec3.multiplyQbyV(rootOrientation,
                            Vec3.multiplyVbyV(selection[0].dimensions,
                                Vec3.subtract(Vec3.HALF, selection[0].registrationPoint))));
                }
                localCenter = Vec3.multiplyQbyV(Quat.inverse(rootOrientation), Vec3.subtract(center, rootPosition));
                orientation = rootOrientation;
                dimensions = selection[0].dimensions;
            } else if (selection.length > 1) {
                // Find min & max x, y, z values of entities' dimension box corners in root entity coordinate system.
                // Note: Don't use entities' bounding boxes because they're in world coordinates and may make the calculated
                // bounding box be larger than necessary.
                min = Vec3.multiplyVbyV(Vec3.subtract(Vec3.ZERO, selection[0].registrationPoint), selection[0].dimensions);
                max = Vec3.multiplyVbyV(Vec3.subtract(Vec3.ONE, selection[0].registrationPoint), selection[0].dimensions);
                inverseOrientation = Quat.inverse(rootOrientation);
                for (i = 1, length = selection.length; i < length; i += 1) {

                    registration = selection[i].registrationPoint;
                    corners[0] = { x: -registration.x, y: -registration.y, z: -registration.z };
                    corners[1] = { x: -registration.x, y: -registration.y, z: 1.0 - registration.z };
                    corners[2] = { x: -registration.x, y: 1.0 - registration.y, z: -registration.z };
                    corners[3] = { x: -registration.x, y: 1.0 - registration.y, z: 1.0 - registration.z };
                    corners[4] = { x: 1.0 - registration.x, y: -registration.y, z: -registration.z };
                    corners[5] = { x: 1.0 - registration.x, y: -registration.y, z: 1.0 - registration.z };
                    corners[6] = { x: 1.0 - registration.x, y: 1.0 - registration.y, z: -registration.z };
                    corners[7] = { x: 1.0 - registration.x, y: 1.0 - registration.y, z: 1.0 - registration.z };

                    position = selection[i].position;
                    rotation = selection[i].rotation;
                    dimensions = selection[i].dimensions;

                    for (j = 0; j < NUM_CORNERS; j += 1) {
                        // Corner position in world coordinates.
                        corners[j] = Vec3.sum(position, Vec3.multiplyQbyV(rotation, Vec3.multiplyVbyV(corners[j], dimensions)));
                        // Corner position in root entity coordinates.
                        corners[j] = Vec3.multiplyQbyV(inverseOrientation, Vec3.subtract(corners[j], rootPosition));
                        // Update min & max.
                        min = Vec3.min(corners[j], min);
                        max = Vec3.max(corners[j], max);
                    }
                }

                // Calculate bounding box.
                center = Vec3.sum(rootPosition,
                    Vec3.multiplyQbyV(rootOrientation, Vec3.multiply(0.5, Vec3.sum(min, max))));
                localCenter = Vec3.multiply(0.5, Vec3.sum(min, max));
                orientation = rootOrientation;
                dimensions = Vec3.subtract(max, min);
            }

            return {
                center: center,
                localCenter: localCenter,
                orientation: orientation,
                dimensions: dimensions
            };
        }

        function getPositionAndOrientation() {
            // Position and orientation of root entity.
            return {
                position: rootPosition,
                orientation: rootOrientation
            };
        }

        function setPositionAndOrientation(position, orientation) {
            // Position and orientation of root entity.
            rootPosition = position;
            rootOrientation = orientation;
            Entities.editEntity(rootEntityID, {
                position: position,
                rotation: orientation
            });
        }

        function startScaling(center) {
            scaleCenter = center;
            scaleRootOffset = Vec3.subtract(selection[0].position, center);
            scaleRootOrientation = rootOrientation;
        }

        function scale(factor, rotation, center) {
            var position,
                orientation,
                i,
                length;

            // Position root.
            position = Vec3.sum(center, Vec3.multiply(factor, Vec3.multiplyQbyV(rotation, scaleRootOffset)));
            orientation = Quat.multiply(rotation, scaleRootOrientation);
            rootPosition = position;
            rootOrientation = orientation;
            Entities.editEntity(selection[0].id, {
                dimensions: Vec3.multiply(factor, selection[0].dimensions),
                position: position,
                rotation: orientation
            });

            // Scale and position children.
            for (i = 1, length = selection.length; i < length; i += 1) {
                Entities.editEntity(selection[i].id, {
                    dimensions: Vec3.multiply(factor, selection[i].dimensions),
                    localPosition: Vec3.multiply(factor, selection[i].localPosition)
                });
            }
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
            count: count,
            rootEntityID: getRootEntityID,
            boundingBox: getBoundingBox,
            getPositionAndOrientation: getPositionAndOrientation,
            setPositionAndOrientation: setPositionAndOrientation,
            startScaling: startScaling,
            scale: scale,
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
            COLORS_GRAB_SEARCHING_FULL_SQUEEZE = { red: 250, green: 10, blue: 10 },  // Per handControllgerGrab.js.
            COLORS_GRAB_SEARCHING_HALF_SQUEEZE_BRIGHT,
            COLORS_GRAB_SEARCHING_FULL_SQUEEZE_BRIGHT,
            BRIGHT_POW = 0.06;  // Per handControllgerGrab.js.

        function colorPow(color, power) {  // Per handControllerGrab.js.
            return {
                red: Math.pow(color.red / 255, power) * 255,
                green: Math.pow(color.green / 255, power) * 255,
                blue: Math.pow(color.blue / 255, power) * 255
            };
        }

        COLORS_GRAB_SEARCHING_HALF_SQUEEZE_BRIGHT = colorPow(COLORS_GRAB_SEARCHING_HALF_SQUEEZE, BRIGHT_POW);
        COLORS_GRAB_SEARCHING_FULL_SQUEEZE_BRIGHT = colorPow(COLORS_GRAB_SEARCHING_FULL_SQUEEZE, BRIGHT_POW);

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

        function updateLine(start, end, color) {
            Overlays.editOverlay(laserLine, {
                start: start,
                end: end,
                color: color,
                visible: true
            });
        }

        function updateSphere(location, size, color, brightColor) {
            var rotation;

            rotation = Quat.lookAt(location, Camera.getPosition(), Vec3.UP);

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
                color,
                brightColor;

            searchDistance = SEARCH_SPHERE_FOLLOW_RATE * searchDistance + (1.0 - SEARCH_SPHERE_FOLLOW_RATE) * distance;
            searchTarget = Vec3.sum(origin, Vec3.multiply(searchDistance, direction));
            sphereSize = SEARCH_SPHERE_SIZE * searchDistance;
            color = isClicked ? COLORS_GRAB_SEARCHING_FULL_SQUEEZE : COLORS_GRAB_SEARCHING_HALF_SQUEEZE;
            brightColor = isClicked ? COLORS_GRAB_SEARCHING_FULL_SQUEEZE_BRIGHT : COLORS_GRAB_SEARCHING_HALF_SQUEEZE_BRIGHT;

            updateLine(origin, searchTarget, color);
            updateSphere(searchTarget, sphereSize, color, brightColor);
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

    Hand = function (side, gripPressedCallback) {
        // Hand controller input.
        // Each hand has a laser, an entity selection, and entity highlighter.

        var hand,
            handController,
            controllerTrigger,
            controllerTriggerClicked,
            controllerGrip,

            isGripPressed = false,
            GRIP_ON_VALUE = 0.99,
            GRIP_OFF_VALUE = 0.95,

            TRIGGER_ON_VALUE = 0.15,  // Per handControllerGrab.js.
            TRIGGER_OFF_VALUE = 0.1,  // Per handControllerGrab.js.
            GRAB_POINT_SPHERE_OFFSET = { x: 0.04, y: 0.13, z: 0.039 },  // Per HmdDisplayPlugin.cpp and controllers.js.

            NEAR_GRAB_RADIUS = 0.1,  // Per handControllerGrab.js.
            NEAR_HOVER_RADIUS = 0.025,

            PICK_MAX_DISTANCE = 500,  // Per handControllerGrab.js.
            PRECISION_PICKING = true,
            NO_INCLUDE_IDS = [],
            NO_EXCLUDE_IDS = [],
            VISIBLE_ONLY = true,

            handPose,
            intersection = {},

            isScaleWithHandles = false,

            isLaserOn = false,
            hoveredEntityID = null,

            EDITIBLE_ENTITY_QUERY_PROPERTYES = ["parentID", "visible", "locked", "type"],
            NONEDITABLE_ENTITY_TYPES = ["Unknown", "Zone", "Light"],

            isEditing = false,
            isEditingWithHand = false,
            initialHandOrientationInverse,
            initialHandToSelectionVector,
            initialSelectionOrientation,
            laserEditingDistance,

            isScaling = false,
            initialTargetPosition,
            initialTargetsCenter,
            initialTargetsSeparation,
            initialtargetsDirection,

            doEdit,
            doHighlight,

            otherHand,

            laser,
            selection,
            highlights,
            handles;

        hand = side;
        if (hand === LEFT_HAND) {
            handController = Controller.Standard.LeftHand;
            controllerTrigger = Controller.Standard.LT;
            controllerTriggerClicked = Controller.Standard.LTClick;
            controllerGrip = Controller.Standard.LeftGrip;
            GRAB_POINT_SPHERE_OFFSET.x = -GRAB_POINT_SPHERE_OFFSET.x;
        } else {
            handController = Controller.Standard.RightHand;
            controllerTrigger = Controller.Standard.RT;
            controllerTriggerClicked = Controller.Standard.RTClick;
            controllerGrip = Controller.Standard.RightGrip;
        }

        laser = new Laser(hand);
        selection = new Selection();
        highlights = new Highlights(hand);
        handles = new Handles();

        function setOtherHand(hand) {
            otherHand = hand;
        }

        function setScaleWithHandles(value) {
            isScaleWithHandles = value;
        }

        function getIsEditing(rootEntityID) {
            return isEditing && rootEntityID === selection.rootEntityID();
        }

        function getHandPosition() {
            return Vec3.sum(Vec3.multiplyQbyV(MyAvatar.orientation, handPose.translation), MyAvatar.position);
        }

        function getTargetPosition() {
            if (isEditingWithHand) {
                return hand === LEFT_HAND ? MyAvatar.getLeftPalmPosition() : MyAvatar.getRightPalmPosition();
            }
            return Vec3.sum(getHandPosition(), Vec3.multiply(laserEditingDistance,
                Quat.getUp(Quat.multiply(MyAvatar.orientation, handPose.rotation))));
        }

        function startEditing() {
            var selectionPositionAndOrientation,
                initialOtherTargetPosition;

            initialHandOrientationInverse = Quat.inverse(Quat.multiply(MyAvatar.orientation, handPose.rotation));

            selection.select(hoveredEntityID);  // Entity may have been moved by other hand so refresh position and orientation.
            selectionPositionAndOrientation = selection.getPositionAndOrientation();
            initialHandToSelectionVector = Vec3.subtract(selectionPositionAndOrientation.position, getHandPosition());
            initialSelectionOrientation = selectionPositionAndOrientation.orientation;

            isEditingWithHand = intersection.handSelected;

            if (otherHand.isEditing(selection.rootEntityID())) {
                // Store initial values for use in scaling.
                initialTargetPosition = getTargetPosition();
                initialOtherTargetPosition = otherHand.getTargetPosition();
                initialTargetsCenter = Vec3.multiply(0.5, Vec3.sum(initialTargetPosition, initialOtherTargetPosition));
                initialTargetsSeparation = Vec3.distance(initialTargetPosition, initialOtherTargetPosition);
                initialtargetsDirection = Vec3.subtract(initialOtherTargetPosition, initialTargetPosition);
                selection.startScaling(initialTargetsCenter);
                isScaling = true;
            } else {
                isScaling = false;
            }

            isEditing = true;
        }

        function updateGrabOffset(selectionPositionAndOrientation) {
            initialHandOrientationInverse = Quat.inverse(Quat.multiply(MyAvatar.orientation, handPose.rotation));
            initialHandToSelectionVector = Vec3.subtract(selectionPositionAndOrientation.position, getHandPosition());
            initialSelectionOrientation = selectionPositionAndOrientation.orientation;
        }

        function applyGrab() {
            var handPosition,
                handOrientation,
                deltaOrientation,
                selectionPosition,
                selectionOrientation;

            handPosition = getHandPosition();
            handOrientation = Quat.multiply(MyAvatar.orientation, handPose.rotation);

            deltaOrientation = Quat.multiply(handOrientation, initialHandOrientationInverse);
            selectionPosition = Vec3.sum(handPosition, Vec3.multiplyQbyV(deltaOrientation, initialHandToSelectionVector));
            selectionOrientation = Quat.multiply(deltaOrientation, initialSelectionOrientation);

            selection.setPositionAndOrientation(selectionPosition, selectionOrientation);
        }

        function applyScale() {
            var targetPosition,
                otherTargetPosition,
                targetsSeparation,
                scale,
                rotation,
                center,
                selectionPositionAndOrientation;

            // Scale selection.
            targetPosition = getTargetPosition();
            otherTargetPosition = otherHand.getTargetPosition();
            targetsSeparation = Vec3.distance(targetPosition, otherTargetPosition);
            scale = targetsSeparation / initialTargetsSeparation;
            rotation = Quat.rotationBetween(initialtargetsDirection, Vec3.subtract(otherTargetPosition, targetPosition));
            center = Vec3.multiply(0.5, Vec3.sum(targetPosition, otherTargetPosition));
            selection.scale(scale, rotation, center);

            // Update grab offsets.
            selectionPositionAndOrientation = selection.getPositionAndOrientation();
            updateGrabOffset(selectionPositionAndOrientation);
            otherHand.updateGrabOffset(selectionPositionAndOrientation);
        }

        function stopEditing() {
            isScaling = false;
            isEditing = false;
        }

        function isEditableEntity(entityID) {
            // Entity trees are moved as a group so check the root entity.
            var properties = Entities.getEntityProperties(entityID, EDITIBLE_ENTITY_QUERY_PROPERTYES);
            while (properties.parentID && properties.parentID !== NULL_UUID) {
                properties = Entities.getEntityProperties(properties.parentID, EDITIBLE_ENTITY_QUERY_PROPERTYES);
            }
            return properties.visible && !properties.locked && NONEDITABLE_ENTITY_TYPES.indexOf(properties.type) === -1;
        }

        function hoverHandle(overlayID) {
            handles.hover(overlayID);
        }

        function update() {
            var gripValue,
                palmPosition,
                overlayID,
                overlayIDs,
                overlayDistance,
                distance,
                entityID,
                entityIDs,
                entitySize,
                size,
                isHandSelected,
                laserIntersection,
                wasLaserOn,
                handPosition,
                handOrientation,
                deltaOrigin,
                pickRay,
                laserLength,
                isTriggerPressed,
                isTriggerClicked,
                wasEditing,
                i,
                length;

            // Hand position and orientation.
            handPose = Controller.getPoseValue(handController);
            wasLaserOn = isLaserOn;
            if (!handPose.valid) {
                isLaserOn = false;
                if (wasLaserOn) {
                    laser.clear();
                    selection.clear();
                    highlights.clear();
                    handles.clear();
                    hoveredEntityID = null;
                }
                return;
            }

            // Controller trigger.
            isTriggerPressed = Controller.getValue(controllerTrigger) > (isTriggerPressed
                ? TRIGGER_OFF_VALUE : TRIGGER_ON_VALUE);
            isTriggerClicked = Controller.getValue(controllerTriggerClicked);

            // Controller grip.
            gripValue = Controller.getValue(controllerGrip);
            if (isGripPressed) {
                isGripPressed = gripValue > GRIP_OFF_VALUE;
            } else {
                isGripPressed = gripValue > GRIP_ON_VALUE;
                if (isGripPressed) {
                    gripPressedCallback();
                }
            }

            // Hand-overlay intersection, if any.
            overlayID = null;
            palmPosition = hand === LEFT_HAND ? MyAvatar.getLeftPalmPosition() : MyAvatar.getRightPalmPosition();
            overlayIDs = Overlays.findOverlays(palmPosition, NEAR_HOVER_RADIUS);
            if (overlayIDs.length > 0) {
                // Typically, there will be only one overlay; optimize for that case.
                overlayID = overlayIDs[0];
                if (overlayIDs.length > 1) {
                    // Find closest overlay.
                    overlayDistance = Vec3.length(Vec3.subtract(Overlays.getProperty(overlayID, "position"), palmPosition));
                    for (i = 1, length = overlayIDs.length; i < length; i += 1) {
                        distance =
                            Vec3.length(Vec3.subtract(Overlays.getProperty(overlayIDs[i], "position"), palmPosition));
                        if (distance > overlayDistance) {
                            overlayID = overlayIDs[i];
                            overlayDistance = distance;
                        }
                    }
                }
            }

            // Hand-entity intersection, if any.
            // TODO: Only test intersection if overlay not intersected?
            entityID = null;
            // palmPosition is set above.
            entityIDs = Entities.findEntities(palmPosition, NEAR_GRAB_RADIUS);
            if (entityIDs.length > 0) {
                // TODO: If number of entities is often 1 in practice, optimize code for this case.
                // Find smallest, editable entity.
                entitySize = HALF_TREE_SCALE;
                for (i = 0, length = entityIDs.length; i < length; i += 1) {
                    if (isEditableEntity(entityIDs[i])) {
                        size = Vec3.length(Entities.getEntityProperties(entityIDs[i], "dimensions").dimensions);
                        if (size < entitySize) {
                            entityID = entityIDs[i];
                            entitySize = size;
                        }
                    }
                }
            }

            isHandSelected = overlayID !== null || entityID !== null;

            // Laser-overlay or -entity intersection if not already intersected.
            if (!isHandSelected && isTriggerPressed) {
                handPosition = Vec3.sum(Vec3.multiplyQbyV(MyAvatar.orientation, handPose.translation), MyAvatar.position);
                handOrientation = Quat.multiply(MyAvatar.orientation, handPose.rotation);
                deltaOrigin = Vec3.multiplyQbyV(handOrientation, GRAB_POINT_SPHERE_OFFSET);
                pickRay = {
                    origin: Vec3.sum(handPosition, deltaOrigin),  // Add a bit to ...
                    direction: Quat.getUp(handOrientation),
                    length: PICK_MAX_DISTANCE
                };

                laserIntersection = Overlays.findRayIntersection(pickRay, PRECISION_PICKING, NO_INCLUDE_IDS, NO_EXCLUDE_IDS,
                    VISIBLE_ONLY);
                if (laserIntersection.intersects) {
                    overlayID = laserIntersection.overlayID;
                }
                if (!laserIntersection.intersects) {
                    laserIntersection = Entities.findRayIntersection(pickRay, PRECISION_PICKING, NO_INCLUDE_IDS, NO_EXCLUDE_IDS,
                        VISIBLE_ONLY);
                    if (laserIntersection.intersects && isEditableEntity(laserIntersection.entityID)) {
                        entityID = laserIntersection.entityID;
                    } else {
                        laserIntersection.intersects = false;
                    }
                }
                laserLength = isEditing
                    ? laserEditingDistance
                    : (laserIntersection.intersects ? laserIntersection.distance : PICK_MAX_DISTANCE);
                isLaserOn = true;
            } else {
                isLaserOn = false;
            }

            intersection = {
                intersects: overlayID !== null || entityID !== null,
                overlayID: overlayID,
                entityID: entityID,
                handSelected: isHandSelected
            };

            // Laser update.
            if (isLaserOn) {
                laser.update(pickRay.origin, pickRay.direction, laserLength, isTriggerClicked);
            } else if (wasLaserOn) {
                laser.clear();
            }

            // Highlight / edit.
            doEdit = false;
            doHighlight = false;
            if (isTriggerClicked) {
                if (isEditing) {
                    // Perform edit.
                    doEdit = true;
                } else if (intersection.intersects && intersection.entityID && (!isScaleWithHandles
                        || !otherHand.isEditing(Entities.rootOf(intersection.entityID)))) {
                    // Start editing.
                    if (intersection.entityID !== hoveredEntityID) {
                        hoveredEntityID = intersection.entityID;
                        selection.select(hoveredEntityID);
                    }
                    highlights.clear();
                    if (isLaserOn) {
                        laserEditingDistance = laserLength;
                    }
                    startEditing();
                    if (isScaleWithHandles) {
                        handles.display(selection.rootEntityID(), selection.boundingBox(), selection.count() > 1);
                    }
                }
            } else {
                wasEditing = isEditing;
                if (isEditing) {
                    // Stop editing.
                    stopEditing();
                }
                if (isScaleWithHandles) {
                    otherHand.hoverHandle(intersection.overlayID);
                }
                if (intersection.intersects && intersection.entityID) {
                    // Hover entities.
                    if (wasEditing || intersection.entityID !== hoveredEntityID) {
                        hoveredEntityID = intersection.entityID;
                        selection.select(hoveredEntityID);
                        doHighlight = true;
                    }
                } else {
                    // Unhover entities.
                    if (hoveredEntityID) {
                        selection.clear();
                        highlights.clear();
                        handles.clear();
                        hoveredEntityID = null;
                    }
                }
            }
        }

        function apply() {
            if (doEdit) {
                if (otherHand.isEditing(selection.rootEntityID())) {
                    if (isScaling && !isScaleWithHandles) {
                        applyScale();
                    }
                } else {
                    applyGrab();
                }
            } else if (doHighlight) {
                if (!isScaleWithHandles || !otherHand.isEditing(selection.rootEntityID())) {
                    highlights.display(intersection.handSelected, selection.selection(),
                        otherHand.isEditing(selection.rootEntityID()) || isScaleWithHandles);
                }
            }
        }

        function clear() {
            laser.clear();
            selection.clear();
            highlights.clear();
            handles.clear();
            isLaserOn = false;
            isEditing = false;
            isEditingWithHand = false;
            isScaling = false;
            hoveredEntityID = null;
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
            if (handles) {
                handles.destroy();
                handles = null;
            }
        }

        if (!this instanceof Hand) {
            return new Hand();
        }

        return {
            setOtherHand: setOtherHand,
            setScaleWithHandles: setScaleWithHandles,
            isEditing: getIsEditing,
            hoverHandle: hoverHandle,
            getTargetPosition: getTargetPosition,
            updateGrabOffset: updateGrabOffset,
            update: update,
            apply: apply,
            clear: clear,
            destroy: destroy
        };
    };

    function update() {
        // Main update loop.
        updateTimer = null;

        // Each hand's action depends on the state of the other hand, so update the states first then apply in actions.
        hands[LEFT_HAND].update();
        hands[RIGHT_HAND].update();
        hands[LEFT_HAND].apply();
        hands[RIGHT_HAND].apply();

        updateTimer = Script.setTimeout(update, UPDATE_LOOP_TIMEOUT);
    }

    function updateHandControllerGrab() {
        // Communicate app status to handControllerGrab.js.
        Settings.setValue(VR_EDIT_SETTING, isAppActive);
    }

    function onAppButtonClicked() {
        // Application tablet/toolbar button clicked.
        isAppActive = !isAppActive;
        updateHandControllerGrab();
        button.editProperties({ isActive: isAppActive });

        if (isAppActive) {
            update();
        } else {
            Script.clearTimeout(updateTimer);
            updateTimer = null;
            hands[LEFT_HAND].clear();
            hands[RIGHT_HAND].clear();
        }
    }

    function onGripClicked() {
        isScaleWithHandles = !isScaleWithHandles;
        hands[LEFT_HAND].setScaleWithHandles(isScaleWithHandles);
        hands[RIGHT_HAND].setScaleWithHandles(isScaleWithHandles);
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
            button.clicked.connect(onAppButtonClicked);
        }

        // Hands, each with a laser, selection, etc.
        hands[LEFT_HAND] = new Hand(LEFT_HAND, onGripClicked);
        hands[RIGHT_HAND] = new Hand(RIGHT_HAND, onGripClicked);
        hands[LEFT_HAND].setOtherHand(hands[RIGHT_HAND]);
        hands[RIGHT_HAND].setOtherHand(hands[LEFT_HAND]);

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
            button.clicked.disconnect(onAppButtonClicked);
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
