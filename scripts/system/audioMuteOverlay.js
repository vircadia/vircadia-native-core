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
    var TWEEN_SPEED = 0.025;
    var LERP_AMOUNT = 0.25;

    var overlayPosition = Vec3.ZERO;
    var tweenPosition = 0;
    // colors are in HSV, h needs to be a value from 0-1
    var startColor = {
        h: 0,
        s: 0,
        v: 0.67
    };
    var endColor = {
        h: 0,
        s: 1,
        v: 1
    };
    var overlayID;

    AudioDevice.muteToggled.connect(onMuteToggled);
    Script.update.connect(update);
    Script.scriptEnding.connect(cleanup);

    function update(dt) {
        if (!AudioDevice.getMuted()) {
            if (overlayID) {
                deleteOverlay();
            }
            return;
        }
        updateOverlay(); 
    }

    function lerp(a, b, val) {
        return (1 - val) * a + val * b;
    }

    // hsv conversion expects 0-1 values
    function hsvToRgb(h, s, v) {
        var r, g, b;

        var i = Math.floor(h * 6);
        var f = h * 6 - i;
        var p = v * (1 - s);
        var q = v * (1 - f * s);
        var t = v * (1 - (1 - f) * s);

        switch (i % 6) {
            case 0: r = v, g = t, b = p; break;
            case 1: r = q, g = v, b = p; break;
            case 2: r = p, g = v, b = t; break;
            case 3: r = p, g = q, b = v; break;
            case 4: r = t, g = p, b = v; break;
            case 5: r = v, g = p, b = q; break;
        }

        return {
            red: r * 255,
            green: g * 255,
            blue: b * 255
        }
    }

    function getOffsetPosition() {
        return Vec3.sum(Camera.position, Quat.getFront(Camera.orientation));
    }

    function onMuteToggled() {
        if (AudioDevice.getMuted()) {
            createOverlay();
        } else {
            deleteOverlay();
        }
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

    function updateOverlay() {
        // increase by TWEEN_SPEED until completion
        if (tweenPosition < 1) {
            tweenPosition += TWEEN_SPEED;
        } else {
            // after tween completion reset to zero and flip values to ping pong 
            tweenPosition = 0;
            for (var color in startColor) {
                var storedColor = startColor[color];
                startColor[color] = endColor[color];
                endColor[color] = storedColor;
            }
        }

        // update position based on LERP_AMOUNT
        var offsetPosition = getOffsetPosition();
        overlayPosition.x = lerp(overlayPosition.x, offsetPosition.x, LERP_AMOUNT);
        overlayPosition.y = lerp(overlayPosition.y, offsetPosition.y, LERP_AMOUNT);
        overlayPosition.z = lerp(overlayPosition.z, offsetPosition.z, LERP_AMOUNT);

        var rgbColor = hsvToRgb(
        );

        Overlays.editOverlay(overlayID, {
            color: {
                red: rgbColor.red,
                green: rgbColor.green,
                blue: rgbColor.blue
            },
            position: overlayPosition,
            rotation: Camera.orientation
        });
    }

    function deleteOverlay() {
        Overlays.deleteOverlay(overlayID);
    }

    function cleanup() {
        deleteOverlay();
        AudioDevice.muteToggled.disconnect(onMuteToggled);
        Script.update.disconnect(update);
    }
}()); // END LOCAL_SCOPE
