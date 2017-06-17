"use strict";
/* jslint vars: true, plusplus: true, forin: true*/
/* globals Tablet, Script, AvatarList, Users, Entities, MyAvatar, Camera, Overlays, Vec3, Quat, Controller, print, getControllerWorldLocation */
/* eslint indent: ["error", 4, { "outerIIFEBody": 0 }] */
//
// audioMuteOverlay.js
//
// client script that creates an overlay to provide mute feedback
//
// Created by Triplelexx on 17/03/09
// Copyright 2017 High Fidelity, Inc.
//
// Distributed under the Apache License, Version 2.0.
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() { // BEGIN LOCAL_SCOPE
    var utilsPath = Script.resolvePath('../developer/libraries/utils.js');
    Script.include(utilsPath);

    var TWEEN_SPEED = 0.025;
    var MIX_AMOUNT = 0.25;

    var overlayPosition = Vec3.ZERO;
    var tweenPosition = 0;
    var startColor = {
        red: 170,
        green: 170,
        blue: 170
    };
    var endColor = {
        red: 255,
        green: 0,
        blue: 0
    };
    var overlayID;

    Script.update.connect(update);
    Script.scriptEnding.connect(cleanup);

    function update(dt) {
        if (!Audio.muted) {
            if (hasOverlay()) {
                deleteOverlay();
            }
        } else if (!hasOverlay()) {
            createOverlay();
        } else {
            updateOverlay();
        }
    }

    function getOffsetPosition() {
        return Vec3.sum(Camera.position, Quat.getFront(Camera.orientation));
    }

    function createOverlay() {
        overlayPosition = getOffsetPosition();
        overlayID = Overlays.addOverlay("sphere", {
            position: overlayPosition,
            rotation: Camera.orientation,
            alpha: 0.9,
            dimensions: 0.1,
            solid: true,
            ignoreRayIntersection: true
        });
    }

    function hasOverlay() {
       return Overlays.getProperty(overlayID, "position") !== undefined;
    }

    function updateOverlay() {
        // increase by TWEEN_SPEED until completion
        if (tweenPosition < 1) {
            tweenPosition += TWEEN_SPEED;
        } else {
            // after tween completion reset to zero and flip values to ping pong 
            tweenPosition = 0;
            for (var component in startColor) {
                var storedColor = startColor[component];
                startColor[component] = endColor[component];
                endColor[component] = storedColor;
            }
        }
        // mix previous position with new and mix colors
        overlayPosition = Vec3.mix(overlayPosition, getOffsetPosition(), MIX_AMOUNT);
        Overlays.editOverlay(overlayID, {
            color: colorMix(startColor, endColor, easeIn(tweenPosition)),
            position: overlayPosition,
            rotation: Camera.orientation
        });
    }

    function deleteOverlay() {
        Overlays.deleteOverlay(overlayID);
    }

    function cleanup() {
        deleteOverlay();
        Audio.muted.disconnect(onMuteToggled);
        Script.update.disconnect(update);
    }
}()); // END LOCAL_SCOPE
