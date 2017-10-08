//
//  highlights.js
//
//  Created by David Rowe on 21 Jul 2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* global Highlights: true */

Highlights = function (side) {
    // Draws highlights on selected entities.

    "use strict";

    var handOverlay,
        entityOverlays = [],
        boundingBoxOverlay,
        isDisplayingBoundingBox = false,
        HIGHLIGHT_COLOR = { red: 240, green: 240, blue: 0 },
        SCALE_COLOR = { red: 0, green: 240, blue: 240 },
        GROUP_COLOR = { red: 220, green: 60, blue: 220 },
        HAND_HIGHLIGHT_ALPHA = 0.35,
        ENTITY_HIGHLIGHT_ALPHA = 0.8,
        BOUNDING_BOX_ALPHA = 0.8,
        HAND_HIGHLIGHT_OFFSET = { x: 0.0, y: 0.11, z: 0.02 },
        LEFT_HAND = 0;

    if (!(this instanceof Highlights)) {
        return new Highlights();
    }

    handOverlay = Overlays.addOverlay("sphere", {
        dimension: Vec3.ZERO,
        parentID: Uuid.SELF,
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

    boundingBoxOverlay = Overlays.addOverlay("cube", {
        alpha: BOUNDING_BOX_ALPHA,
        solid: false,
        drawInFront: true,
        ignoreRayIntersection: true,
        visible: false
    });

    function setHandHighlightRadius(radius) {
        var dimension = 2 * radius;
        Overlays.editOverlay(handOverlay, {
            dimensions: { x: dimension, y: dimension, z: dimension }
        });
    }

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
        var offset = Vec3.multiplyVbyV(Vec3.subtract(Vec3.HALF, details.registrationPoint), details.dimensions);

        Overlays.editOverlay(entityOverlays[index], {
            parentID: details.id,
            localPosition: offset,
            localRotation: Quat.ZERO,
            dimensions: details.dimensions,
            color: overlayColor,
            visible: true
        });
    }

    function display(handIntersected, selection, entityIndex, boundingBox, overlayColor) {
        // Displays highlight for just entityIndex if non-null, otherwise highlights whole selection.
        var i,
            length;

        // Show/hide hand overlay.
        Overlays.editOverlay(handOverlay, {
            color: overlayColor,
            visible: handIntersected
        });

        // Display entity overlays.
        if (entityIndex !== null) {
            // Add/edit entity overlay for just entityIndex.
            maybeAddEntityOverlay(0);
            editEntityOverlay(0, selection[entityIndex], overlayColor);
        } else {
            // Add/edit entity overlays for all entities in selection.
            for (i = 0, length = selection.length; i < length; i++) {
                maybeAddEntityOverlay(i);
                editEntityOverlay(i, selection[i], overlayColor);
            }
        }

        // Delete extra entity overlays.
        for (i = entityOverlays.length - 1, length = selection.length; i >= length; i--) {
            Overlays.deleteOverlay(entityOverlays[i]);
            entityOverlays.splice(i, 1);
        }

        // Update bounding box overlay.
        if (boundingBox !== null) {
            Overlays.editOverlay(boundingBoxOverlay, {
                position: boundingBox.center,
                rotation: boundingBox.orientation,
                dimensions: boundingBox.dimensions,
                color: overlayColor,
                visible: true
            });
            isDisplayingBoundingBox = true;
        } else if (isDisplayingBoundingBox) {
            Overlays.editOverlay(boundingBoxOverlay, { visible: false });
            isDisplayingBoundingBox = false;
        }
    }

    function clear() {
        var i,
            length;

        // Hide hand and bounding box overlays.
        Overlays.editOverlay(handOverlay, { visible: false });
        Overlays.editOverlay(boundingBoxOverlay, { visible: false });

        // Delete entity overlays.
        for (i = 0, length = entityOverlays.length; i < length; i++) {
            Overlays.deleteOverlay(entityOverlays[i]);
        }
        entityOverlays = [];
    }

    function destroy() {
        clear();
        Overlays.deleteOverlay(handOverlay);
        Overlays.deleteOverlay(boundingBoxOverlay);
    }

    return {
        HIGHLIGHT_COLOR: HIGHLIGHT_COLOR,
        SCALE_COLOR: SCALE_COLOR,
        GROUP_COLOR: GROUP_COLOR,
        setHandHighlightRadius: setHandHighlightRadius,
        display: display,
        clear: clear,
        destroy: destroy
    };
};

Highlights.prototype = {};
