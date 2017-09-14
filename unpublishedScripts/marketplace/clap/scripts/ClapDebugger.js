"use strict";

/*
	clapApp.js
	unpublishedScripts/marketplace/clap/scripts/clapDebugger.js

	Created by Matti 'Menithal' Lahtinen on 9/11/2017
	Copyright 2017 High Fidelity, Inc.

	Distributed under the Apache License, Version 2.0.
	See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
*/

var DEBUG_RIGHT_HAND;
var DEBUG_LEFT_HAND;
var DEBUG_CLAP_LEFT;
var DEBUG_CLAP_RIGHT;
var DEBUG_CLAP;
var DEBUG_CLAP_DIRECTION;

// Debug Values:
var DEBUG_CORRECT = {
    red: 0,
    green: 255,
    blue: 0
};
var DEBUG_WRONG = {
    red: 255,
    green: 0,
    blue: 0
};

var DEBUG_VOLUME = {
    red: 255,
    green: 255,
    blue: 128
};

module.exports = {
    disableDebug: function () {
        Overlays.deleteOverlay(DEBUG_RIGHT_HAND);
        Overlays.deleteOverlay(DEBUG_LEFT_HAND);
        Overlays.deleteOverlay(DEBUG_CLAP_LEFT);
        Overlays.deleteOverlay(DEBUG_CLAP_RIGHT);
        Overlays.deleteOverlay(DEBUG_CLAP_DIRECTION);
    },

    debugPositions: function (leftAlignmentWorld, leftHandPositionOffset, leftHandDownWorld, rightAlignmentWorld, rightHandPositionOffset, rightHandDownWorld, tolerance) {

        Overlays.editOverlay(DEBUG_CLAP_LEFT, {
            color: leftAlignmentWorld > tolerance ? DEBUG_CORRECT : DEBUG_WRONG,
            position: leftHandPositionOffset
        });

        Overlays.editOverlay(DEBUG_CLAP_RIGHT, {
            color: rightAlignmentWorld > tolerance ? DEBUG_CORRECT : DEBUG_WRONG,
            position: rightHandPositionOffset
        });

        Overlays.editOverlay(DEBUG_LEFT_HAND, {
            color: leftAlignmentWorld > tolerance ? DEBUG_CORRECT : DEBUG_WRONG,
            start: leftHandPositionOffset,
            end: Vec3.sum(leftHandPositionOffset, Vec3.multiply(leftHandDownWorld, 0.2))
        });

        Overlays.editOverlay(DEBUG_RIGHT_HAND, {
            color: rightAlignmentWorld > tolerance ? DEBUG_CORRECT : DEBUG_WRONG,
            start: rightHandPositionOffset,
            end: Vec3.sum(rightHandPositionOffset, Vec3.multiply(rightHandDownWorld, 0.2))
        });
    },

    debugClapLine: function (start, end, visible) {
        Overlays.editOverlay(DEBUG_CLAP_DIRECTION, {
            start: start,
            end: end,
            visible: visible
        });
    },

    enableDebug: function () {
        DEBUG_RIGHT_HAND = Overlays.addOverlay("line3d", {
            color: DEBUG_WRONG,
            start: MyAvatar.position,
            end: Vec3.sum(MyAvatar.position, {
                x: 0,
                y: 1,
                z: 0
            }),
            dimensions: {
                x: 2,
                y: 2,
                z: 2
            }
        });

        DEBUG_LEFT_HAND = Overlays.addOverlay("line3d", {
            color: DEBUG_WRONG,
            start: MyAvatar.position,
            end: Vec3.sum(MyAvatar.position, {
                x: 0,
                y: 1,
                z: 0
            }),
            dimensions: {
                x: 2,
                y: 2,
                z: 2
            }
        });

        DEBUG_CLAP_LEFT = Overlays.addOverlay("sphere", {
            position: MyAvatar.position,
            color: DEBUG_WRONG,
            scale: {
                x: 0.05,
                y: 0.05,
                z: 0.05
            }
        });

        DEBUG_CLAP_RIGHT = Overlays.addOverlay("sphere", {
            position: MyAvatar.position,
            color: DEBUG_WRONG,
            scale: {
                x: 0.05,
                y: 0.05,
                z: 0.05
            }
        });


        DEBUG_CLAP_DIRECTION = Overlays.addOverlay("line3d", {
            color: DEBUG_VOLUME,
            start: MyAvatar.position,
            end: MyAvatar.position,
            dimensions: {
                x: 2,
                y: 2,
                z: 2
            }
        });
    }
};
