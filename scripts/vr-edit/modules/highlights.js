//
//  highlights.js
//
//  Created by David Rowe on 21 Jul 2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* global Highlights */

Highlights = function (side) {
    // Draws highlights on selected entities.

    "use strict";

    var handOverlay,
        entityOverlays = [],
        HIGHLIGHT_COLOR = { red: 240, green: 240, blue: 0 },
        SCALE_COLOR = { red: 0, green: 240, blue: 240 },
        GROUP_COLOR = { red: 220, green: 60, blue: 220 },
        HAND_HIGHLIGHT_ALPHA = 0.35,
        ENTITY_HIGHLIGHT_ALPHA = 0.8,
        HAND_HIGHLIGHT_DIMENSIONS = { x: 0.1, y: 0.1, z: 0.1 },
        HAND_HIGHLIGHT_OFFSET = { x: 0.0, y: 0.11, z: 0.02 },
        LEFT_HAND = 0;

    if (!this instanceof Highlights) {
        return new Highlights();
    }

    handOverlay = Overlays.addOverlay("sphere", {
        dimensions: HAND_HIGHLIGHT_DIMENSIONS,
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

    function display(handIntersected, selection, overlayColor) {
        var i,
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

    return {
        HIGHLIGHT_COLOR: HIGHLIGHT_COLOR,
        SCALE_COLOR: SCALE_COLOR,
        GROUP_COLOR: GROUP_COLOR,
        display: display,
        clear: clear,
        destroy: destroy
    };
};

Highlights.prototype = {};
