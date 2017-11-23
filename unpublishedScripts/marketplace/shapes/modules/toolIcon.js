//
//  toolIcon.js
//
//  Created by David Rowe on 28 Jul 2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* global ToolIcon:true, App, UIT */

ToolIcon = function (side) {
    // Tool icon displayed on non-dominant hand.

    "use strict";

    var LEFT_HAND = 0,

        MODEL_DIMENSIONS = { x: 0.1944, y: 0.1928, z: 0.1928 }, // Raw FBX dimensions.
        MODEL_SCALE = 0.7, // Adjust icon dimensions so that the green bar matches that of the Tools header.
        MODEL_POSITION_LEFT_HAND = { x: -0.025, y: 0.03, z: 0 }, // x raises in thumb direction; y moves in fingers direction.
        MODEL_POSITION_RIGHT_HAND = { x: 0.025, y: 0.03, z: 0 }, // ""
        MODEL_ROTATION_LEFT_HAND = Quat.fromVec3Degrees({ x: 0, y: 0, z: 100 }),
        MODEL_ROTATION_RIGHT_HAND = Quat.fromVec3Degrees({ x: 0, y: 180, z: -100 }),

        MODEL_TYPE = "model",
        MODEL_PROPERTIES = {
            url: Script.resolvePath("../assets/tools/tool-icon.fbx"),
            dimensions: Vec3.multiply(MODEL_SCALE, MODEL_DIMENSIONS),
            solid: true,
            alpha: 1.0,
            parentID: Uuid.SELF,
            ignoreRayIntersection: true,
            visible: true
        },

        IMAGE_TYPE = "image3d",
        IMAGE_PROPERTIES = {
            localRotation: Quat.fromVec3Degrees({ x: -90, y: -90, z: 0 }),
            alpha: 1.0,
            emissive: true,
            ignoreRayIntersection: true,
            isFacingAvatar: false,
            visible: true
        },

        ICON_PROPERTIES = {
            // Relative to model overlay. x is in fingers direction; y is in thumb direction.
            localPosition: { x: 0.020, y: 0.069, z: 0 },
            color: UIT.colors.lightGrayText
        },
        LABEL_PROPERTIES = {
            localPosition: { x: -0.040, y: 0.067, z: 0 },
            color: UIT.colors.white
        },
        SUBLABEL_PROPERTIES = {
            localPosition: { x: -0.055, y: 0.067, z: 0 },
            color: UIT.colors.lightGrayText
        },

        ICON_SCALE_FACTOR = 3.0,
        LABEL_SCALE_FACTOR = 1.8,

        handJointName,
        localPosition,
        localRotation,

        modelOverlay = null;

    if (!(this instanceof ToolIcon)) {
        return new ToolIcon();
    }

    function getAssetURLs() {
        return [MODEL_PROPERTIES.url];
    }

    function setHand(side) {
        // Assumes UI is not displaying.
        if (side === LEFT_HAND) {
            handJointName = "LeftHand";
            localPosition = MODEL_POSITION_LEFT_HAND;
            localRotation = MODEL_ROTATION_LEFT_HAND;
        } else {
            handJointName = "RightHand";
            localPosition = MODEL_POSITION_RIGHT_HAND;
            localRotation = MODEL_ROTATION_RIGHT_HAND;
        }
    }

    setHand(side);

    function clear() {
        // Deletes current tool model.
        if (modelOverlay) {
            Overlays.deleteOverlay(modelOverlay); // Child overlays are automatically deleted.
            modelOverlay = null;
        }
    }

    function display(iconInfo) {
        // Displays icon on hand.
        var handJointIndex,
            properties;

        handJointIndex = MyAvatar.getJointIndex(handJointName);
        if (handJointIndex === -1) {
            // Don't display if joint isn't available (yet) to attach to.
            // User can clear this condition by toggling the app off and back on once avatar finishes loading.
            App.log(side, "ERROR: ToolIcon: Hand joint index isn't available!");
            return;
        }

        if (modelOverlay !== null) {
            // Should never happen because tool needs to be cleared in order for user to return to Tools menu.
            clear();
        }

        // Model.
        properties = Object.clone(MODEL_PROPERTIES);
        properties.parentJointIndex = handJointIndex;
        properties.localPosition = localPosition;
        properties.localRotation = localRotation;
        modelOverlay = Overlays.addOverlay(MODEL_TYPE, properties);

        // Icon.
        properties = Object.clone(IMAGE_PROPERTIES);
        properties = Object.merge(properties, ICON_PROPERTIES);
        properties.parentID = modelOverlay;
        properties.url = iconInfo.icon.properties.url;
        properties.dimensions = {
            x: ICON_SCALE_FACTOR * iconInfo.icon.properties.dimensions.x,
            y: ICON_SCALE_FACTOR * iconInfo.icon.properties.dimensions.y
        };
        properties.localPosition.y += ICON_SCALE_FACTOR * iconInfo.icon.headerOffset.y;
        Overlays.addOverlay(IMAGE_TYPE, properties);

        // Label.
        properties = Object.clone(IMAGE_PROPERTIES);
        properties = Object.merge(properties, LABEL_PROPERTIES);
        properties.parentID = modelOverlay;
        properties.url = iconInfo.label.properties.url;
        properties.scale = LABEL_SCALE_FACTOR * iconInfo.label.properties.scale;
        Overlays.addOverlay(IMAGE_TYPE, properties);

        // Sublabel.
        properties = Object.clone(IMAGE_PROPERTIES);
        properties = Object.merge(properties, SUBLABEL_PROPERTIES);
        properties.parentID = modelOverlay;
        properties.url = iconInfo.sublabel.properties.url;
        properties.scale = LABEL_SCALE_FACTOR * iconInfo.sublabel.properties.scale;
        Overlays.addOverlay(IMAGE_TYPE, properties);
    }

    function destroy() {
        clear();
    }

    return {
        assetURLs: getAssetURLs,
        setHand: setHand,
        display: display,
        clear: clear,
        destroy: destroy
    };
};

ToolIcon.prototype = {};
