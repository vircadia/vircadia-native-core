//
//  handles.js
//
//  Created by David Rowe on 21 Jul 2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* global Handles:true */

Handles = function (side) {
    // Draws scaling handles.

    "use strict";

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
        HANDLE_NORMAL_ALPHA = 0.7,
        HANDLE_HOVER_ALPHA = 0.9,
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
        DISTANCE_MULTIPLIER_MULTIPLIER = 0.25,
        hoveredOverlayID = null,
        isVisible = false,

        // Scaling.
        scalingBoundingBoxDimensions,
        scalingBoundingBoxLocalCenter,

        i;

    if (!(this instanceof Handles)) {
        return new Handles(side);
    }

    CORNER_HANDLE_OVERLAY_AXES = [
        // Ordered such that items 4 apart are opposite corners - used in display().
        { x: -0.5, y: -0.5, z: -0.5 },
        { x: -0.5, y: -0.5, z: 0.5 },
        { x: -0.5, y: 0.5, z: -0.5 },
        { x: -0.5, y: 0.5, z: 0.5 },
        { x: 0.5, y: 0.5, z: 0.5 },
        { x: 0.5, y: 0.5, z: -0.5 },
        { x: 0.5, y: -0.5, z: 0.5 },
        { x: 0.5, y: -0.5, z: -0.5 }
    ];

    FACE_HANDLE_OVERLAY_AXES = [
        { x: -0.5, y: 0, z: 0 },
        { x: 0.5, y: 0, z: 0 },
        { x: 0, y: -0.5, z: 0 },
        { x: 0, y: 0.5, z: 0 },
        { x: 0, y: 0, z: -0.5 },
        { x: 0, y: 0, z: 0.5 }
    ];

    FACE_HANDLE_OVERLAY_OFFSETS = {
        x: FACE_HANDLE_OVERLAY_DIMENSIONS.y,
        y: FACE_HANDLE_OVERLAY_DIMENSIONS.y,
        z: FACE_HANDLE_OVERLAY_DIMENSIONS.y
    };

    FACE_HANDLE_OVERLAY_ROTATIONS = [
        Quat.fromVec3Degrees({ x: 0, y: 0, z: 90 }),
        Quat.fromVec3Degrees({ x: 0, y: 0, z: -90 }),
        Quat.fromVec3Degrees({ x: 180, y: 0, z: 0 }),
        Quat.fromVec3Degrees({ x: 0, y: 0, z: 0 }),
        Quat.fromVec3Degrees({ x: -90, y: 0, z: 0 }),
        Quat.fromVec3Degrees({ x: 90, y: 0, z: 0 })
    ];

    FACE_HANDLE_OVERLAY_SCALE_AXES = [
        Vec3.UNIT_NEG_X,
        Vec3.UNIT_X,
        Vec3.UNIT_NEG_Y,
        Vec3.UNIT_Y,
        Vec3.UNIT_NEG_Z,
        Vec3.UNIT_Z
    ];

    function isAxisHandle(overlayID) {
        return faceHandleOverlays.indexOf(overlayID) !== -1;
    }

    function isCornerHandle(overlayID) {
        return cornerHandleOverlays.indexOf(overlayID) !== -1;
    }

    function isHandle(overlayID) {
        return isAxisHandle(overlayID) || isCornerHandle(overlayID);
    }

    function handleOffset(overlayID) {
        // Distance from overlay position to entity surface.
        if (isCornerHandle(overlayID)) {
            return 0; // Corner overlays are centered on the corner.
        }
        return faceHandleOffsets.y / 2;
    }

    function getOverlays() {
        return [].concat(cornerHandleOverlays, faceHandleOverlays);
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
        return Vec3.abs(FACE_HANDLE_OVERLAY_SCALE_AXES[faceHandleOverlays.indexOf(overlayID)]);
    }

    function display(rootEntityID, boundingBox, isMultipleEntities, isSuppressZAxis) {
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
        boundingBoxOverlay = Overlays.addOverlay("cube", {
            parentID: rootEntityID,
            localPosition: boundingBoxLocalCenter,
            localRotation: Quat.ZERO,
            dimensions: boundingBoxDimensions,
            color: BOUNDING_BOX_COLOR,
            alpha: BOUNDING_BOX_ALPHA,
            solid: false,
            drawInFront: true,
            ignoreRayIntersection: true,
            visible: true
        });

        // Somewhat maintain general angular size of scale handles per bounding box center but make more distance ones 
        // display smaller in order to give comfortable depth cue.
        cameraPosition = Camera.position;
        boundingBoxVector = Vec3.subtract(boundingBox.center, Camera.position);
        distanceMultiplier = Vec3.length(boundingBoxVector);
        distanceMultiplier = DISTANCE_MULTIPLIER_MULTIPLIER
            * (distanceMultiplier + (1 - Math.LOG10E * Math.log(distanceMultiplier + 1)));

        // Corner scale handles.
        // At right-most and opposite corners of bounding box.
        cameraUp = Quat.getUp(Camera.orientation);
        maxCrossProductScale = 0;
        for (i = 0; i < NUM_CORNERS; i++) {
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
        for (i = 0; i < NUM_CORNER_HANDLES; i++) {
            cornerHandleOverlays[i] = Overlays.addOverlay("sphere", {
                parentID: rootEntityID,
                localPosition: Vec3.sum(boundingBoxLocalCenter,
                    Vec3.multiplyVbyV(CORNER_HANDLE_OVERLAY_AXES[cornerIndexes[i]], boundingBoxDimensions)),
                localRotation: Quat.ZERO,
                dimensions: cornerHandleDimensions,
                color: HANDLE_NORMAL_COLOR,
                alpha: HANDLE_NORMAL_ALPHA,
                solid: true,
                drawInFront: true,
                ignoreRayIntersection: false,
                visible: true
            });
        }

        // Face scale handles.
        // Only valid for a single entity because for multiple entities, some may be at an angle relative to the root entity
        // which would necessitate a (non-existent) shear transform be applied to them when scaling a face of the set.
        if (!isMultipleEntities) {
            faceHandleDimensions = Vec3.multiply(distanceMultiplier, FACE_HANDLE_OVERLAY_DIMENSIONS);
            faceHandleOffsets = Vec3.multiply(distanceMultiplier, FACE_HANDLE_OVERLAY_OFFSETS);
            for (i = 0; i < NUM_FACE_HANDLES; i++) {
                if (!isSuppressZAxis || FACE_HANDLE_OVERLAY_AXES[i].z === 0) {
                    faceHandleOverlays[i] = Overlays.addOverlay("shape", {
                        parentID: rootEntityID,
                        localPosition: Vec3.sum(boundingBoxLocalCenter,
                            Vec3.multiplyVbyV(FACE_HANDLE_OVERLAY_AXES[i], Vec3.sum(boundingBoxDimensions, faceHandleOffsets))),
                        localRotation: FACE_HANDLE_OVERLAY_ROTATIONS[i],
                        dimensions: faceHandleDimensions,
                        shape: "Cone",
                        color: HANDLE_NORMAL_COLOR,
                        alpha: HANDLE_NORMAL_ALPHA,
                        solid: true,
                        drawInFront: true,
                        ignoreRayIntersection: false,
                        visible: true
                    });
                }
            }
        } else {
            faceHandleOverlays = [];
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
        for (i = 0; i < NUM_CORNER_HANDLES; i++) {
            Overlays.editOverlay(cornerHandleOverlays[i], {
                localPosition: Vec3.sum(scalingBoundingBoxDimensions,
                    Vec3.multiplyVbyV(CORNER_HANDLE_OVERLAY_AXES[cornerIndexes[i]], scalingBoundingBoxLocalCenter))
            });
        }

        // Face scale handles.
        if (faceHandleOverlays.length > 0) {
            for (i = 0; i < NUM_FACE_HANDLES; i++) {
                Overlays.editOverlay(faceHandleOverlays[i], {
                    localPosition: Vec3.sum(scalingBoundingBoxDimensions,
                        Vec3.multiplyVbyV(FACE_HANDLE_OVERLAY_AXES[i],
                            Vec3.sum(scalingBoundingBoxLocalCenter, faceHandleOffsets)))
                });
            }
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
                Overlays.editOverlay(overlayID, {
                    color: HANDLE_HOVER_COLOR,
                    alpha: HANDLE_HOVER_ALPHA
                });
                hoveredOverlayID = overlayID;
            }
        }
    }

    function grab(overlayID) {
        var overlay,
            isShowAll = overlayID === null,
            color = isShowAll ? HANDLE_NORMAL_COLOR : HANDLE_HOVER_COLOR,
            alpha = isShowAll ? HANDLE_NORMAL_ALPHA : HANDLE_HOVER_ALPHA,
            i,
            length;

        for (i = 0, length = cornerHandleOverlays.length; i < length; i++) {
            overlay = cornerHandleOverlays[i];
            Overlays.editOverlay(overlay, {
                visible: isVisible && (isShowAll || overlay === overlayID),
                color: color,
                alpha: alpha
            });
        }

        for (i = 0, length = faceHandleOverlays.length; i < length; i++) {
            overlay = faceHandleOverlays[i];
            Overlays.editOverlay(overlay, {
                visible: isVisible && (isShowAll || overlay === overlayID),
                color: color,
                alpha: alpha
            });
        }
    }

    function clear() {
        var i,
            length;

        Overlays.deleteOverlay(boundingBoxOverlay);
        for (i = 0; i < NUM_CORNER_HANDLES; i++) {
            Overlays.deleteOverlay(cornerHandleOverlays[i]);
        }
        for (i = 0, length = faceHandleOverlays.length; i < length; i++) {
            Overlays.deleteOverlay(faceHandleOverlays[i]);
        }

        isVisible = false;
    }

    function destroy() {
        clear();
    }

    return {
        display: display,
        overlays: getOverlays,
        isHandle: isHandle,
        handleOffset: handleOffset,
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

Handles.prototype = {};
