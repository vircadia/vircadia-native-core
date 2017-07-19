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
        isAppScaleWithHandles = false,

        VR_EDIT_SETTING = "io.highfidelity.isVREditing",  // Note: This constant is duplicated in utils.js.

        editors = [],
        LEFT_HAND = 0,
        RIGHT_HAND = 1,

        UPDATE_LOOP_TIMEOUT = 16,
        updateTimer = null,

        Highlights,
        Handles,
        Selection,
        Laser,
        Hand,
        Editor,

        AVATAR_SELF_ID = "{00000000-0000-0000-0000-000000000001}",
        NULL_UUID = "{00000000-0000-0000-0000-000000000000}",
        HALF_TREE_SCALE = 16384,

        DEBUG = true;  // TODO: Set false.


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
            if (entityID === undefined || entityID === null) {
                return null;
            }
            rootEntityID = entityID;
            entityProperties = Entities.getEntityProperties(rootEntityID, PARENT_PROPERTIES);
            while (entityProperties.parentID !== NULL_UUID) {
                rootEntityID = entityProperties.parentID;
                entityProperties = Entities.getEntityProperties(rootEntityID, PARENT_PROPERTIES);
            }
            return rootEntityID;
        };
    }


    function log(message) {
        print(APP_NAME + ": " + message);
    }

    function debug(side, message) {
        // Optional parameter: side.
        var hand = "",
            HAND_LETTERS = ["L", "R"];
        if (DEBUG) {
            if (side === 0 || side === 1) {
                hand = HAND_LETTERS[side] + " ";
            } else {
                message = side;
            }
            log(hand + message);
        }
    }

    function isEditableRoot(entityID) {
        var EDITIBLE_ENTITY_QUERY_PROPERTYES = ["parentID", "visible", "locked", "type"],
            NONEDITABLE_ENTITY_TYPES = ["Unknown", "Zone", "Light"],
            properties;
        properties = Entities.getEntityProperties(entityID, EDITIBLE_ENTITY_QUERY_PROPERTYES);
        while (properties.parentID && properties.parentID !== NULL_UUID) {
            properties = Entities.getEntityProperties(properties.parentID, EDITIBLE_ENTITY_QUERY_PROPERTYES);
        }
        return properties.visible && !properties.locked && NONEDITABLE_ENTITY_TYPES.indexOf(properties.type) === -1;
    }


    Highlights = function (side) {
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
            parentJointIndex: MyAvatar.getJointIndex(side === LEFT_HAND
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

        function display(handIntersected, selection, isScale) {
            var overlayColor = isScale ? SCALE_HIGHLIGHT_COLOR : GRAB_HIGHLIGHT_COLOR,
                i,
                length;

            // Show/hide hand overlay.
            Overlays.editOverlay(handOverlay, {
                color: overlayColor,
                visible: handIntersected
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


    Handles = function (side) {
        var boundingBoxOverlay,
            boundingBoxDimensions,
            boundingBoxLocalCenter,
            cornerIndexes = [],
            cornerHandleOverlays = [],
            faceHandleOverlays = [],
            faceHandleOffsets,
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
            FACE_HANDLE_OVERLAY_SCALE_AXES,
            ZERO_ROTATION = Quat.fromVec3Radians(Vec3.ZERO),
            DISTANCE_MULTIPLIER_MULTIPLIER = 0.5,
            hoveredOverlayID = null,
            isVisible = false,

            // Scaling.
            scalingBoundingBoxDimensions,
            scalingBoundingBoxLocalCenter,

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

        FACE_HANDLE_OVERLAY_SCALE_AXES = [
            Vec3.UNIT_X,
            Vec3.UNIT_X,
            Vec3.UNIT_Y,
            Vec3.UNIT_Y,
            Vec3.UNIT_Z,
            Vec3.UNIT_Z
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

        function isAxisHandle(overlayID) {
            return faceHandleOverlays.indexOf(overlayID) !== -1;
        }

        function isCornerHandle(overlayID) {
            return cornerHandleOverlays.indexOf(overlayID) !== -1;
        }

        function isHandle(overlayID) {
            return isAxisHandle(overlayID) || isCornerHandle(overlayID);
        }

        function scalingAxis(overlayID) {
            var axesIndex;
            if (isCornerHandle(overlayID)) {
                axesIndex = CORNER_HANDLE_OVERLAY_AXES[cornerIndexes[cornerHandleOverlays.indexOf(overlayID)]];
                return Vec3.normalize(Vec3.multiplyVbyV(axesIndex, boundingBoxDimensions));
            }
            return FACE_HANDLE_OVERLAY_SCALE_AXES[faceHandleOverlays.indexOf(overlayID)];
        }

        function scalingDirections(overlayID) {
            if (isCornerHandle(overlayID)) {
                return Vec3.ONE;
            }
            return FACE_HANDLE_OVERLAY_SCALE_AXES[faceHandleOverlays.indexOf(overlayID)];
        }

        function display(rootEntityID, boundingBox, isMultiple) {
            var boundingBoxCenter,
                boundingBoxOrientation,
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
                i;

            isVisible = true;

            boundingBoxDimensions = boundingBox.dimensions;
            boundingBoxCenter = boundingBox.center;
            boundingBoxLocalCenter = boundingBox.localCenter;
            boundingBoxOrientation = boundingBox.orientation;

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

        function startScaling() {
            // Nothing to do.
        }

        function scale(scale3D) {
            // Scale relative to dimensions and positions at start of scaling.

            // Selection bounding box.
            scalingBoundingBoxDimensions = Vec3.multiplyVbyV(scale3D, boundingBoxLocalCenter);
            scalingBoundingBoxLocalCenter = Vec3.multiplyVbyV(scale3D, boundingBoxDimensions);
            Overlays.editOverlay(boundingBoxOverlay, {
                localPosition: scalingBoundingBoxDimensions,
                dimensions: scalingBoundingBoxLocalCenter
            });

            // Corner scale handles.
            for (i = 0; i < NUM_CORNER_HANDLES; i += 1) {
                Overlays.editOverlay(cornerHandleOverlays[i], {
                    localPosition: Vec3.sum(scalingBoundingBoxDimensions,
                        Vec3.multiplyVbyV(CORNER_HANDLE_OVERLAY_AXES[cornerIndexes[i]], scalingBoundingBoxLocalCenter))
                });
            }

            // Face scale handles.
            for (i = 0; i < NUM_FACE_HANDLES; i += 1) {
                Overlays.editOverlay(faceHandleOverlays[i], {
                    localPosition: Vec3.sum(scalingBoundingBoxDimensions,
                        Vec3.multiplyVbyV(FACE_HANDLE_OVERLAY_AXES[i],
                            Vec3.sum(scalingBoundingBoxLocalCenter, faceHandleOffsets)))
                });
            }
        }

        function finishScaling() {
            // Adopt final scale.
            boundingBoxLocalCenter = scalingBoundingBoxDimensions;
            boundingBoxDimensions = scalingBoundingBoxLocalCenter;
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

        function grab(overlayID) {
            var overlay,
                isShowAll = overlayID === null,
                color = isShowAll ? HANDLE_NORMAL_COLOR : HANDLE_HOVER_COLOR,
                i,
                length;

            for (i = 0, length = cornerHandleOverlays.length; i < length; i += 1) {
                overlay = cornerHandleOverlays[i];
                Overlays.editOverlay(overlay, {
                    visible: isVisible && (isShowAll || overlay === overlayID),
                    color: color
                });
            }

            for (i = 0, length = faceHandleOverlays.length; i < length; i += 1) {
                overlay = faceHandleOverlays[i];
                Overlays.editOverlay(overlay, {
                    visible: isVisible && (isShowAll || overlay === overlayID),
                    color: color
                });
            }
        }

        function clear() {
            var i;

            isVisible = false;

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
            return new Handles(side);
        }

        return {
            display: display,
            isHandle: isHandle,
            scalingAxis: scalingAxis,
            scalingDirections: scalingDirections,
            startScaling: startScaling,
            scale: scale,
            finishScaling: finishScaling,
            hover: hover,
            grab: grab,
            clear: clear,
            destroy: destroy
        };
    };


    Selection = function (side) {
        // Manages set of selected entities. Currently supports just one set of linked entities.
        var selection = [],
            selectedEntityID = null,
            rootEntityID = null,
            rootPosition,
            rootOrientation,
            scaleRootPosition,
            scaleRootRegistrationPoint,
            scaleRootRegistrationOffset,
            scaleCenter,
            scaleRootOffset,
            scaleRootOrientation,
            isRootCenterRegistration,
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

        function startDirectScaling(center) {
            // Save initial position and orientation so that can scale relative to these without accumulating float errors.
            scaleCenter = center;
            scaleRootOffset = Vec3.subtract(rootPosition, center);
            scaleRootOrientation = rootOrientation;
        }

        function directScale(factor, rotation, center) {
            // Scale, position, and rotate selection.
            var i,
                length;

            // Scale, position, and orient root.
            rootPosition = Vec3.sum(center, Vec3.multiply(factor, Vec3.multiplyQbyV(rotation, scaleRootOffset)));
            rootOrientation = Quat.multiply(rotation, scaleRootOrientation);
            Entities.editEntity(selection[0].id, {
                dimensions: Vec3.multiply(factor, selection[0].dimensions),
                position: rootPosition,
                rotation: rootOrientation
            });

            // Scale and position children.
            for (i = 1, length = selection.length; i < length; i += 1) {
                Entities.editEntity(selection[i].id, {
                    dimensions: Vec3.multiply(factor, selection[i].dimensions),
                    localPosition: Vec3.multiply(factor, selection[i].localPosition)
                });
            }
        }

        function finishDirectScaling() {
            select(selectedEntityID);  // Refresh.
        }

        function startHandleScaling() {
            // Save initial position data so that can scale relative to these without accumulating float errors.
            scaleRootPosition = rootPosition;
            scaleRootRegistrationPoint = selection[0].registrationPoint;
            isRootCenterRegistration = Vec3.equal(scaleRootRegistrationPoint, Vec3.HALF);
            scaleRootRegistrationOffset = Vec3.subtract(scaleRootRegistrationPoint, Vec3.HALF);
        }

        function handleScale(factor) {
            // Scale selection about bounding box center.

            // Scale and position root.
            if (isRootCenterRegistration) {
                Entities.editEntity(selection[0].id, {
                    dimensions: Vec3.multiplyVbyV(factor, selection[0].dimensions)
                });
            } else {
                rootPosition = Vec3.sum(scaleRootPosition, Vec3.multiplyVbyV(factor, scaleRootRegistrationOffset));
                Entities.editEntity(selection[0].id, {
                    dimensions: Vec3.multiplyVbyV(factor, selection[0].dimensions),
                    position: rootPosition
                });
            }

            // Scale and position children.
            // TODO
        }

        function finishHandleScaling() {
            select(selectedEntityID);  // Refresh.
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
            return new Selection(side);
        }

        return {
            select: select,
            selection: getSelection,
            count: count,
            rootEntityID: getRootEntityID,
            boundingBox: getBoundingBox,
            getPositionAndOrientation: getPositionAndOrientation,
            setPositionAndOrientation: setPositionAndOrientation,
            startDirectScaling: startDirectScaling,
            directScale: directScale,
            finishDirectScaling: finishDirectScaling,
            startHandleScaling: startHandleScaling,
            handleScale: handleScale,
            finishHandleScaling: finishHandleScaling,
            clear: clear,
            destroy: destroy
        };
    };


    Laser = function (side) {
        // Draws hand lasers.
        // May intersect with entities or bounding box of other hand's selection.

        var isLaserOn = false,

            laserLine = null,
            laserSphere = null,

            searchDistance = 0.0,

            SEARCH_SPHERE_SIZE = 0.013,  // Per handControllerGrab.js multiplied by 1.2 per handControllerGrab.js.
            SEARCH_SPHERE_FOLLOW_RATE = 0.5,  // Per handControllerGrab.js.
            COLORS_GRAB_SEARCHING_HALF_SQUEEZE = { red: 10, green: 10, blue: 255 },  // Per handControllgerGrab.js.
            COLORS_GRAB_SEARCHING_FULL_SQUEEZE = { red: 250, green: 10, blue: 10 },  // Per handControllgerGrab.js.
            COLORS_GRAB_SEARCHING_HALF_SQUEEZE_BRIGHT,
            COLORS_GRAB_SEARCHING_FULL_SQUEEZE_BRIGHT,
            BRIGHT_POW = 0.06,  // Per handControllgerGrab.js.

            GRAB_POINT_SPHERE_OFFSET = { x: 0.04, y: 0.13, z: 0.039 },  // Per HmdDisplayPlugin.cpp and controllers.js.

            PICK_MAX_DISTANCE = 500,  // Per handControllerGrab.js.
            PRECISION_PICKING = true,
            NO_INCLUDE_IDS = [],
            NO_EXCLUDE_IDS = [],
            VISIBLE_ONLY = true,

            laserLength,
            specifiedLaserLength = null,

            intersection;

        function colorPow(color, power) {  // Per handControllerGrab.js.
            return {
                red: Math.pow(color.red / 255, power) * 255,
                green: Math.pow(color.green / 255, power) * 255,
                blue: Math.pow(color.blue / 255, power) * 255
            };
        }

        COLORS_GRAB_SEARCHING_HALF_SQUEEZE_BRIGHT = colorPow(COLORS_GRAB_SEARCHING_HALF_SQUEEZE, BRIGHT_POW);
        COLORS_GRAB_SEARCHING_FULL_SQUEEZE_BRIGHT = colorPow(COLORS_GRAB_SEARCHING_FULL_SQUEEZE, BRIGHT_POW);

        if (side === LEFT_HAND) {
            GRAB_POINT_SPHERE_OFFSET.x = -GRAB_POINT_SPHERE_OFFSET.x;
        }

        laserLine = Overlays.addOverlay("line3d", {
            lineWidth: 5,
            alpha: 1.0,
            glow: 1.0,
            ignoreRayIntersection: true,
            drawInFront: true,
            parentID: AVATAR_SELF_ID,
            parentJointIndex: MyAvatar.getJointIndex(side === LEFT_HAND
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

        function display(origin, direction, distance, isClicked) {
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

        function hide() {
            Overlays.editOverlay(laserLine, { visible: false });
            Overlays.editOverlay(laserSphere, { visible: false });
        }

        function update(hand) {
            var handPosition,
                handOrientation,
                deltaOrigin,
                pickRay;

            if (!hand.intersection().intersects && hand.triggerPressed()) {
                handPosition = hand.position();
                handOrientation = hand.orientation();
                deltaOrigin = Vec3.multiplyQbyV(handOrientation, GRAB_POINT_SPHERE_OFFSET);
                pickRay = {
                    origin: Vec3.sum(handPosition, deltaOrigin),
                    direction: Quat.getUp(handOrientation),
                    length: PICK_MAX_DISTANCE
                };

                intersection = Overlays.findRayIntersection(pickRay, PRECISION_PICKING, NO_INCLUDE_IDS, NO_EXCLUDE_IDS,
                    VISIBLE_ONLY);
                if (!intersection.intersects) {
                    intersection = Entities.findRayIntersection(pickRay, PRECISION_PICKING, NO_INCLUDE_IDS, NO_EXCLUDE_IDS,
                        VISIBLE_ONLY);
                    if (intersection.intersects && !isEditableRoot(intersection.entityID)) {
                        intersection.intersects = false;
                        intersection.entityID = null;
                    }
                }
                intersection.laserIntersected = true;
                laserLength = (specifiedLaserLength !== null)
                    ? specifiedLaserLength
                    : (intersection.intersects ? intersection.distance : PICK_MAX_DISTANCE);

                isLaserOn = true;
                display(pickRay.origin, pickRay.direction, laserLength, hand.triggerClicked());
            } else {
                intersection = {
                    intersects: false
                };
                if (isLaserOn) {
                    isLaserOn = false;
                    hide();
                }
            }
        }

        function getIntersection() {
            return intersection;
        }

        function setLength(length) {
            specifiedLaserLength = length;
            laserLength = length;
        }

        function clearLength() {
            specifiedLaserLength = null;
        }

        function getLength() {
            return laserLength;
        }

        function clear() {
            isLaserOn = false;
            hide();
        }

        function destroy() {
            Overlays.deleteOverlay(laserLine);
            Overlays.deleteOverlay(laserSphere);
        }

        if (!this instanceof Laser) {
            return new Laser(side);
        }

        return {
            update: update,
            intersection: getIntersection,
            setLength: setLength,
            clearLength: clearLength,
            length: getLength,
            clear: clear,
            destroy: destroy
        };
    };


    Hand = function (side, gripPressedCallback) {
        // Hand controller input.
        var handController,  // ####### Rename to "controller".
            controllerTrigger,
            controllerTriggerClicked,
            controllerGrip,

            isGripPressed = false,
            GRIP_ON_VALUE = 0.99,
            GRIP_OFF_VALUE = 0.95,

            isTriggerPressed,
            isTriggerClicked,
            TRIGGER_ON_VALUE = 0.15,  // Per handControllerGrab.js.
            TRIGGER_OFF_VALUE = 0.1,  // Per handControllerGrab.js.

            NEAR_GRAB_RADIUS = 0.1,  // Per handControllerGrab.js.
            NEAR_HOVER_RADIUS = 0.025,

            handPose,
            handPosition,
            handOrientation,

            intersection = {};

        if (side === LEFT_HAND) {
            handController = Controller.Standard.LeftHand;
            controllerTrigger = Controller.Standard.LT;
            controllerTriggerClicked = Controller.Standard.LTClick;
            controllerGrip = Controller.Standard.LeftGrip;
        } else {
            handController = Controller.Standard.RightHand;
            controllerTrigger = Controller.Standard.RT;
            controllerTriggerClicked = Controller.Standard.RTClick;
            controllerGrip = Controller.Standard.RightGrip;
        }

        function valid() {
            return handPose.valid;
        }

        function position() {
            return handPosition;
        }

        function orientation() {
            return handOrientation;
        }

        function triggerPressed() {
            return isTriggerPressed;
        }

        function triggerClicked() {
            return isTriggerClicked;
        }

        function getIntersection() {
            return intersection;
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
                i,
                length;


            // Hand pose.
            handPose = Controller.getPoseValue(handController);
            if (!handPose.valid) {
                intersection = {};
                return;
            }
            handPosition = Vec3.sum(Vec3.multiplyQbyV(MyAvatar.orientation, handPose.translation), MyAvatar.position);
            handOrientation = Quat.multiply(MyAvatar.orientation, handPose.rotation);

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
            palmPosition = side === LEFT_HAND ? MyAvatar.getLeftPalmPosition() : MyAvatar.getRightPalmPosition();
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

            // Hand-entity intersection, if any, if overlay not intersected.
            entityID = null;
            if (overlayID === null) {
                // palmPosition is set above.
                entityIDs = Entities.findEntities(palmPosition, NEAR_GRAB_RADIUS);
                if (entityIDs.length > 0) {
                    // Typically, there will be only one entity; optimize for that case.
                    if (isEditableRoot(entityIDs[0])) {
                        entityID = entityIDs[0];
                    }
                    if (entityIDs.length > 1) {
                        // Find smallest, editable entity.
                        entitySize = HALF_TREE_SCALE;
                        for (i = 0, length = entityIDs.length; i < length; i += 1) {
                            if (isEditableRoot(entityIDs[i])) {
                                size = Vec3.length(Entities.getEntityProperties(entityIDs[i], "dimensions").dimensions);
                                if (size < entitySize) {
                                    entityID = entityIDs[i];
                                    entitySize = size;
                                }
                            }
                        }
                    }
                }
            }

            intersection = {
                intersects: overlayID !== null || entityID !== null,
                overlayID: overlayID,
                entityID: entityID,
                handIntersected: true
            };
        }

        function clear() {
            // Nothing to do.
        }

        function destroy() {
            // Nothing to do.
        }

        if (!this instanceof Hand) {
            return new Hand(side);
        }

        return {
            valid: valid,
            position: position,
            orientation: orientation,
            triggerPressed: triggerPressed,
            triggerClicked: triggerClicked,
            intersection: getIntersection,
            update: update,
            clear: clear,
            destroy: destroy
        };
    };


    Editor = function (side, gripPressedCallback) {
        // Each controller has a hand, laser, an entity selection, entity highlighter, and entity handles.

        var otherEditor,  // Other hand's Editor object.

            // Editor states.
            EDITOR_IDLE = 0,
            EDITOR_SEARCHING = 1,
            EDITOR_HIGHLIGHTING = 2,  // Highlighting an entity (not hovering a handle).
            EDITOR_GRABBING = 3,
            EDITOR_DIRECT_SCALING = 4,  // Scaling data are sent to other editor's EDITOR_GRABBING state.
            EDITOR_HANDLE_SCALING = 5,  // ""
            editorState = EDITOR_IDLE,
            EDITOR_STATE_STRINGS = ["EDITOR_IDLE", "EDITOR_SEARCHING", "EDITOR_HIGHLIGHTING", "EDITOR_GRABBING",
                "EDITOR_DIRECT_SCALING", "EDITOR_HANDLE_SCALING"],

            // State machine.
            STATE_MACHINE,
            highlightedEntityID = null,
            wasAppScaleWithHandles = false,
            isOtherEditorEditingEntityID = false,
            hoveredOverlayID = null,

            // Primary objects.
            hand,
            laser,
            selection,
            highlights,
            handles,

            // Position values.
            initialHandOrientationInverse,
            initialHandToSelectionVector,
            initialSelectionOrientation,

            // Scaling values.
            isScalingWithHand = false,
            isDirectScaling = false,  // Modifies EDITOR_GRABBING state.
            isHandleScaling = false,  // ""
            initialTargetsSeparation,
            initialtargetsDirection,
            otherTargetPosition,
            handleUnitScaleAxis,
            handleScaleDirections,
            handleHandOffset,
            initialHandleDistance,

            intersection;

        hand = new Hand(side, gripPressedCallback);
        laser = new Laser(side);
        selection = new Selection(side);
        highlights = new Highlights(side);
        handles = new Handles(side);

        function setOtherEditor(editor) {
            otherEditor = editor;
        }

        function hoverHandle(overlayID) {
            // Highlights handle if overlayID is a handle, otherwise unhighlights currently highlighted handle if any.
            handles.hover(overlayID);
        }

        function isHandle(overlayID) {
            return handles.isHandle(overlayID);
        }

        function isEditing(rootEntityID) {
            // rootEntityID is an optional parameter.
            return editorState > EDITOR_HIGHLIGHTING
                && (rootEntityID === undefined || rootEntityID === selection.rootEntityID());
        }

        function startEditing() {
            var selectionPositionAndOrientation;

            initialHandOrientationInverse = Quat.inverse(hand.orientation());
            selectionPositionAndOrientation = selection.getPositionAndOrientation();
            initialHandToSelectionVector = Vec3.subtract(selectionPositionAndOrientation.position, hand.position());
            initialSelectionOrientation = selectionPositionAndOrientation.orientation;
        }

        function stopEditing() {
            // Nothing to do.
        }


        function getScaleTargetPosition() {
            if (isScalingWithHand) {
                return side === LEFT_HAND ? MyAvatar.getLeftPalmPosition() : MyAvatar.getRightPalmPosition();
            }
            return Vec3.sum(hand.position(), Vec3.multiply(laser.length(), Quat.getUp(hand.orientation())));
        }

        function startDirectScaling(targetPosition) {
            var initialTargetPosition,
                initialTargetsCenter;

            isScalingWithHand = intersection.handIntersected;

            otherTargetPosition = targetPosition;
            initialTargetPosition = getScaleTargetPosition();
            initialTargetsCenter = Vec3.multiply(0.5, Vec3.sum(initialTargetPosition, otherTargetPosition));
            initialTargetsSeparation = Vec3.distance(initialTargetPosition, otherTargetPosition);
            initialtargetsDirection = Vec3.subtract(otherTargetPosition, initialTargetPosition);

            selection.startDirectScaling(initialTargetsCenter);
            isDirectScaling = true;
        }

        function updateDirectScaling(targetPosition) {
            otherTargetPosition = targetPosition;
        }

        function stopDirectScaling() {
            selection.finishDirectScaling();
            isDirectScaling = false;
        }

        function startHandleScaling(targetPosition, overlayID) {
            var boundingBox,
                scaleAxis,
                handDistance;

            isScalingWithHand = intersection.handIntersected;

            // Keep grabbed handle highlighted and hide other handles.
            handles.grab(overlayID);

            handleUnitScaleAxis = handles.scalingAxis(overlayID);  // Unit vector in direction of scaling.
            handleScaleDirections = handles.scalingDirections(overlayID);  // Which axes to scale the selection on.

            // Distance from handle to bounding box center.
            boundingBox = selection.boundingBox();
            initialHandleDistance = Vec3.length(Vec3.multiplyVbyV(boundingBox.dimensions, handleScaleDirections)) / 2;

            // Distance from hand to handle in direction of handle.
            otherTargetPosition = targetPosition;
            scaleAxis = Vec3.multiplyQbyV(selection.getPositionAndOrientation().orientation, handleUnitScaleAxis);
            handDistance = Math.abs(Vec3.dot(Vec3.subtract(otherTargetPosition, boundingBox.center), scaleAxis));
            handleHandOffset = handDistance - initialHandleDistance;

            selection.startHandleScaling();
            handles.startScaling();
            isHandleScaling = true;
        }

        function updateHandleScaling(targetPosition) {
            otherTargetPosition = targetPosition;
        }

        function stopHandleScaling() {
            handles.finishScaling();
            selection.finishHandleScaling();
            handles.grab(null);  // Stop highlighting grabbed handle and resume displaying all handles.
            isHandleScaling = false;
        }


        function applyGrab() {
            // Sets position and orientation of selection per grabbing hand.
            var handPosition,
                handOrientation,
                deltaOrientation,
                selectionPosition,
                selectionOrientation;

            handPosition = hand.position();
            handOrientation = hand.orientation();

            deltaOrientation = Quat.multiply(handOrientation, initialHandOrientationInverse);
            selectionPosition = Vec3.sum(handPosition, Vec3.multiplyQbyV(deltaOrientation, initialHandToSelectionVector));
            selectionOrientation = Quat.multiply(deltaOrientation, initialSelectionOrientation);

            selection.setPositionAndOrientation(selectionPosition, selectionOrientation);
        }

        function applyDirectScale() {
            // Scales, rotates, and positions selection per changing length, orientation, and position of vector between hands.
            var targetPosition,
                targetsSeparation,
                scale,
                rotation,
                center,
                selectionPositionAndOrientation;

            // Scale selection.
            targetPosition = getScaleTargetPosition();
            targetsSeparation = Vec3.distance(targetPosition, otherTargetPosition);
            scale = targetsSeparation / initialTargetsSeparation;
            rotation = Quat.rotationBetween(initialtargetsDirection, Vec3.subtract(otherTargetPosition, targetPosition));
            center = Vec3.multiply(0.5, Vec3.sum(targetPosition, otherTargetPosition));
            selection.directScale(scale, rotation, center);

            // Update grab offset.
            selectionPositionAndOrientation = selection.getPositionAndOrientation();
            initialHandOrientationInverse = Quat.inverse(hand.orientation());
            initialHandToSelectionVector = Vec3.subtract(selectionPositionAndOrientation.position, hand.position());
            initialSelectionOrientation = selectionPositionAndOrientation.orientation;
        }

        function applyHandleScale() {
            // Scales selection per changing position of scaling hand; positions and orients per grabbing hand.
            var boundingBoxCenter,
                scaleAxis,
                handleDistance,
                scale,
                scale3D;

            // Position and orient selection per grabbing hand before scaling it per scaling hand.
            applyGrab();

            // Desired distance of handle from center of bounding box.
            boundingBoxCenter = selection.boundingBox().center;  // TODO: Too expensive for update loop?
            scaleAxis = Vec3.multiplyQbyV(selection.getPositionAndOrientation().orientation, handleUnitScaleAxis);
            handleDistance = Math.abs(Vec3.dot(Vec3.subtract(otherTargetPosition, boundingBoxCenter), scaleAxis));
            handleDistance -= handleHandOffset;

            // Scale selection relative to initial dimensions.
            scale = handleDistance / initialHandleDistance;
            scale3D = Vec3.multiply(scale, handleScaleDirections);
            scale3D = {
                x: scale3D.x !== 0 ? scale3D.x : 1,
                y: scale3D.y !== 0 ? scale3D.y : 1,
                z: scale3D.z !== 0 ? scale3D.z : 1
            };
            handles.scale(scale3D);
            selection.handleScale(scale3D);
        }


        function enterEditorIdle() {
            selection.clear();
        }

        function exitEditorIdle() {
            // Nothing to do.
        }

        function enterEditorSearching() {
            selection.clear();
            hoveredOverlayID = intersection.overlayID;
            otherEditor.hoverHandle(hoveredOverlayID);
        }

        function updateEditorSearching() {
            if (isAppScaleWithHandles && intersection.overlayID !== hoveredOverlayID && otherEditor.isEditing()) {
                hoveredOverlayID = intersection.overlayID;
                otherEditor.hoverHandle(hoveredOverlayID);
            }
        }

        function exitEditorSearching() {
            otherEditor.hoverHandle(null);
        }

        function enterEditorHighlighting() {
            selection.select(highlightedEntityID);
            if (!isAppScaleWithHandles || !otherEditor.isEditing(highlightedEntityID)) {
                highlights.display(intersection.handIntersected, selection.selection(),
                    isAppScaleWithHandles || otherEditor.isEditing(highlightedEntityID));
            }
            isOtherEditorEditingEntityID = otherEditor.isEditing(highlightedEntityID);
        }

        function updateEditorHighlighting() {
            selection.select(highlightedEntityID);
            if (!isAppScaleWithHandles || !otherEditor.isEditing(highlightedEntityID)) {
                highlights.display(intersection.handIntersected, selection.selection(),
                    isAppScaleWithHandles || otherEditor.isEditing(highlightedEntityID));
            } else {
                highlights.clear();
            }
            isOtherEditorEditingEntityID = !isOtherEditorEditingEntityID;
        }

        function exitEditorHighlighting() {
            highlights.clear();
            isOtherEditorEditingEntityID = false;
        }

        function enterEditorGrabbing() {
            selection.select(highlightedEntityID);  // For when transitioning from EDITOR_SEARCHING.
            if (intersection.laserIntersected) {
                laser.setLength(laser.length());
            }
            if (isAppScaleWithHandles) {
                handles.display(highlightedEntityID, selection.boundingBox(), selection.count() > 1);
            }
            startEditing();
        }

        function updateEditorGrabbing() {
            if (isAppScaleWithHandles) {
                handles.display(highlightedEntityID, selection.boundingBox(), selection.count() > 1);
            } else {
                handles.clear();
            }
        }

        function exitEditorGrabbing() {
            stopEditing();
            handles.clear();
            laser.clearLength();
        }

        function enterEditorDirectScaling() {
            selection.select(highlightedEntityID);  // For when transitioning from EDITOR_SEARCHING.
            isScalingWithHand = intersection.handIntersected;
            if (intersection.laserIntersected) {
                laser.setLength(laser.length());
            }
            otherEditor.startDirectScaling(getScaleTargetPosition());
        }

        function updateEditorDirectScaling() {
            otherEditor.updateDirectScaling(getScaleTargetPosition());
        }

        function exitEditorDirectScaling() {
            otherEditor.stopDirectScaling();
            laser.clearLength();
        }

        function enterEditorHandleScaling() {
            selection.select(highlightedEntityID);  // For when transitioning from EDITOR_SEARCHING.
            isScalingWithHand = intersection.handIntersected;
            if (intersection.laserIntersected) {
                laser.setLength(laser.length());
            }
            otherEditor.startHandleScaling(getScaleTargetPosition(), intersection.overlayID);
        }

        function updateEditorHandleScaling() {
            otherEditor.updateHandleScaling(getScaleTargetPosition());
        }

        function exitEditorHandleScaling() {
            otherEditor.stopHandleScaling();
            laser.clearLength();
        }

        STATE_MACHINE = {
            EDITOR_IDLE: {
                enter: enterEditorIdle,
                update: null,
                exit: exitEditorIdle
            },
            EDITOR_SEARCHING: {
                enter: enterEditorSearching,
                update: updateEditorSearching,
                exit: exitEditorSearching
            },
            EDITOR_HIGHLIGHTING: {
                enter: enterEditorHighlighting,
                update: updateEditorHighlighting,
                exit: exitEditorHighlighting
            },
            EDITOR_GRABBING: {
                enter: enterEditorGrabbing,
                update: updateEditorGrabbing,
                exit: exitEditorGrabbing
            },
            EDITOR_DIRECT_SCALING: {
                enter: enterEditorDirectScaling,
                update: updateEditorDirectScaling,
                exit: exitEditorDirectScaling
            },
            EDITOR_HANDLE_SCALING: {
                enter: enterEditorHandleScaling,
                update: updateEditorHandleScaling,
                exit: exitEditorHandleScaling
            }
        };

        function setState(state) {
            if (state !== editorState) {
                STATE_MACHINE[EDITOR_STATE_STRINGS[editorState]].exit();
                STATE_MACHINE[EDITOR_STATE_STRINGS[state]].enter();
                editorState = state;
            } else if (DEBUG) {
                log("ERROR: Null state transition: " + state + "!");
            }
        }

        function updateState() {
            STATE_MACHINE[EDITOR_STATE_STRINGS[editorState]].update();
        }


        function update() {
            var previousState = editorState;

            // Hand update.
            hand.update();
            intersection = hand.intersection();

            // Laser update.
            // Displays laser if hand has no intersection and trigger is pressed.
            if (hand.valid()) {
                laser.update(hand);
                if (!intersection.intersects) {
                    intersection = laser.intersection();
                }
            }

            // State update.
            switch (editorState) {
            case EDITOR_IDLE:
                if (!hand.valid()) {
                    // No transition.
                    break;
                }
                setState(EDITOR_SEARCHING);
                break;
            case EDITOR_SEARCHING:
                if (hand.valid() && !intersection.entityID
                        && !(intersection.overlayID && hand.triggerClicked())) {
                    // No transition.
                    updateState();
                    break;
                }
                if (!hand.valid()) {
                    setState(EDITOR_IDLE);
                } else if (intersection.overlayID && hand.triggerClicked()
                        && otherEditor.isHandle(intersection.overlayID)) {
                    setState(EDITOR_HANDLE_SCALING);
                } else if (intersection.entityID && !hand.triggerClicked()) {
                    highlightedEntityID = Entities.rootOf(intersection.entityID);
                    wasAppScaleWithHandles = isAppScaleWithHandles;
                    setState(EDITOR_HIGHLIGHTING);
                } else if (intersection.entityID && hand.triggerClicked()) {
                    highlightedEntityID = Entities.rootOf(intersection.entityID);
                    wasAppScaleWithHandles = isAppScaleWithHandles;
                    if (otherEditor.isEditing(highlightedEntityID)) {
                        if (!isAppScaleWithHandles) {
                            setState(EDITOR_DIRECT_SCALING);
                        }
                    } else {
                        setState(EDITOR_GRABBING);
                    }
                } else {
                    debug(side, "ERROR: Unexpected condition in EDITOR_SEARCHING!");
                }
                break;
            case EDITOR_HIGHLIGHTING:
                if (hand.valid() && Entities.rootOf(intersection.entityID) === highlightedEntityID
                        && !hand.triggerClicked() && isAppScaleWithHandles === wasAppScaleWithHandles) {
                    // No transition.
                    if (otherEditor.isEditing(highlightedEntityID) !== isOtherEditorEditingEntityID) {
                        updateState();
                    }
                    break;
                }
                if (!hand.valid()) {
                    setState(EDITOR_IDLE);
                } else if (intersection.overlayID && hand.triggerClicked()
                        && otherEditor.isHandle(intersection.overlayID)) {
                    setState(EDITOR_HANDLE_SCALING);
                } else if (intersection.entityID && hand.triggerClicked()) {
                    highlightedEntityID = Entities.rootOf(intersection.entityID);  // May be a different entityID.
                    wasAppScaleWithHandles = isAppScaleWithHandles;
                    if (otherEditor.isEditing(highlightedEntityID)) {
                        if (!isAppScaleWithHandles) {
                            setState(EDITOR_DIRECT_SCALING);
                        }
                    } else {
                        setState(EDITOR_GRABBING);
                    }
                } else if (intersection.entityID && Entities.rootOf(intersection.entityID) !== highlightedEntityID) {
                    highlightedEntityID = Entities.rootOf(intersection.entityID);
                    wasAppScaleWithHandles = isAppScaleWithHandles;
                    updateState();
                } else if (intersection.entityID && isAppScaleWithHandles !== wasAppScaleWithHandles) {
                    wasAppScaleWithHandles = isAppScaleWithHandles;
                    updateState();
                } else if (!intersection.entityID) {
                    // Note that this transition includes the case of highlighting a scaling handle.
                    setState(EDITOR_SEARCHING);
                } else {
                    debug(side, "ERROR: Unexpected condition in EDITOR_HIGHLIGHTING!");
                }
                break;
            case EDITOR_GRABBING:
                if (hand.valid() && hand.triggerClicked()) {
                    // Don't test for intersection.intersected because when scaling with handles intersection may lag behind.
                    // No transition.
                    if (isAppScaleWithHandles !== wasAppScaleWithHandles) {
                        wasAppScaleWithHandles = isAppScaleWithHandles;
                        updateState();
                    }
                    break;
                }
                if (!hand.valid()) {
                    setState(EDITOR_IDLE);
                } else if (!hand.triggerClicked()) {
                    if (intersection.entityID) {
                        highlightedEntityID = Entities.rootOf(intersection.entityID);
                        wasAppScaleWithHandles = isAppScaleWithHandles;
                        setState(EDITOR_HIGHLIGHTING);
                    } else {
                        setState(EDITOR_SEARCHING);
                    }
                } else {
                    debug(side, "ERROR: Unexpected condition in EDITOR_GRABBING!");
                }
                break;
            case EDITOR_DIRECT_SCALING:
                if (hand.valid() && hand.triggerClicked() && otherEditor.isEditing(highlightedEntityID)) {
                    // Don't test for intersection.intersected because when scaling with handles intersection may lag behind.
                    // Don't test isAppScaleWithHandles because this will eventually be a UI element and so not able to be 
                    // changed while scaling with two hands.
                    // No transition.
                    updateState();
                    break;
                }
                if (!hand.valid()) {
                    setState(EDITOR_IDLE);
                } else if (!hand.triggerClicked()) {
                    if (!intersection.entityID) {
                        setState(EDITOR_SEARCHING);
                    } else {
                        highlightedEntityID = Entities.rootOf(intersection.entityID);
                        wasAppScaleWithHandles = isAppScaleWithHandles;
                        setState(EDITOR_HIGHLIGHTING);
                    }
                } else if (!otherEditor.isEditing(highlightedEntityID)) {
                    // Grab highlightEntityID that was scaling and has already been set.
                    wasAppScaleWithHandles = isAppScaleWithHandles;
                    setState(EDITOR_GRABBING);
                }
                break;
            case EDITOR_HANDLE_SCALING:
                if (hand.valid() && hand.triggerClicked() && otherEditor.isEditing(highlightedEntityID)) {
                    // Don't test for intersection.intersected because when scaling with handles intersection may lag behind.
                    // Don't test isAppScaleWithHandles because this will eventually be a UI element and so not able to be 
                    // changed while scaling with two hands.
                    // No transition.
                    updateState();
                    break;
                }
                if (!hand.valid()) {
                    setState(EDITOR_IDLE);
                } else if (!hand.triggerClicked()) {
                    if (!intersection.entityID) {
                        setState(EDITOR_SEARCHING);
                    } else {
                        highlightedEntityID = Entities.rootOf(intersection.entityID);
                        wasAppScaleWithHandles = isAppScaleWithHandles;
                        setState(EDITOR_HIGHLIGHTING);
                    }
                } else if (!otherEditor.isEditing(highlightedEntityID)) {
                    // Grab highlightEntityID that was scaling and has already been set.
                    wasAppScaleWithHandles = isAppScaleWithHandles;
                    setState(EDITOR_GRABBING);
                }
                break;
            }

            if (DEBUG && editorState !== previousState) {
                debug(side, EDITOR_STATE_STRINGS[editorState]);
            }
        }

        function apply() {
            switch (editorState) {
            case EDITOR_GRABBING:
                if (isDirectScaling) {
                    applyDirectScale();
                } else if (isHandleScaling) {
                    applyHandleScale();
                } else {
                    applyGrab();
                }
                break;
            }
        }

        function clear() {
            hand.clear();
            laser.clear();
            selection.clear();
            highlights.clear();
            handles.clear();
        }

        function destroy() {
            if (hand) {
                hand.destroy();
                hand = null;
            }
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

        if (!this instanceof Editor) {
            return new Editor();
        }

        return {
            setOtherEditor: setOtherEditor,
            hoverHandle: hoverHandle,
            isHandle: isHandle,
            isEditing: isEditing,
            startDirectScaling: startDirectScaling,
            updateDirectScaling: updateDirectScaling,
            stopDirectScaling: stopDirectScaling,
            startHandleScaling: startHandleScaling,
            updateHandleScaling: updateHandleScaling,
            stopHandleScaling: stopHandleScaling,
            update: update,
            apply: apply,
            clear: clear,
            destroy: destroy
        };
    };


    function update() {
        // Main update loop.
        updateTimer = null;

        // Each hand's action depends on the state of the other hand, so update the states first then apply actions.
        editors[LEFT_HAND].update();
        editors[RIGHT_HAND].update();
        editors[LEFT_HAND].apply();
        editors[RIGHT_HAND].apply();

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
            editors[LEFT_HAND].clear();
            editors[RIGHT_HAND].clear();
        }
    }

    function onGripClicked() {
        isAppScaleWithHandles = !isAppScaleWithHandles;
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
        editors[LEFT_HAND] = new Editor(LEFT_HAND, onGripClicked);
        editors[RIGHT_HAND] = new Editor(RIGHT_HAND, onGripClicked);
        editors[LEFT_HAND].setOtherEditor(editors[RIGHT_HAND]);
        editors[RIGHT_HAND].setOtherEditor(editors[LEFT_HAND]);

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

        if (editors[LEFT_HAND]) {
            editors[LEFT_HAND].destroy();
            editors[LEFT_HAND] = null;
        }
        if (editors[RIGHT_HAND]) {
            editors[RIGHT_HAND].destroy();
            editors[RIGHT_HAND] = null;
        }

        tablet = null;
    }

    setUp();
    Script.scriptEnding.connect(tearDown);
}());
