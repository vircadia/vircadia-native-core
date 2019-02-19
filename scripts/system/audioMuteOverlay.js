//
// audioMuteOverlay.js
//
// client script that creates an overlay to provide mute feedback
//
// Created by Triplelexx on 17/03/09
// Reworked by Seth Alves on 2019-2-17
// Copyright 2017 High Fidelity, Inc.
//
// Distributed under the Apache License, Version 2.0.
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

"use strict";

/* global Audio, Script, Overlays, Quat, MyAvatar */

(function() { // BEGIN LOCAL_SCOPE

    var lastInputLoudness = 0.0;
    var sampleRate = 8.0; // Hz
    var attackTC =  Math.exp(-1.0 / (sampleRate * 0.500)); // 500 milliseconds attack
    var releaseTC =  Math.exp(-1.0 / (sampleRate * 1.000)); // 1000 milliseconds release
    var holdReset = 2.0 * sampleRate; // 2 seconds hold
    var holdCount = 0;
    var warningOverlayID = null;
    var pollInterval = null;
    var warningText = "Muted";
    var textDimensions = { x: 100, y: 50 };

    function showWarning() {
        if (warningOverlayID) {
            return;
        }

        var windowWidth;
        var windowHeight;
        if (HMD.active) {
            var viewportDimension = Controller.getViewportDimensions();
            windowWidth = viewportDimension.x;
            windowHeight = viewportDimension.y;
        } else {
            windowWidth = Window.innerWidth;
            windowHeight = Window.innerHeight;
        }

        warningOverlayID = Overlays.addOverlay("text", {
            name: "Muted-Warning",
            font: { size: 36 },
            text: warningText,
            x: windowWidth / 2 - textDimensions.x / 2,
            y: windowHeight / 2 - textDimensions.y / 2,
            width: textDimensions.x,
            height: textDimensions.y,
            textColor: { red: 226, green: 51, blue: 77 },
            backgroundAlpha: 0,
            visible: true
        });
    }

    function hideWarning() {
        if (!warningOverlayID) {
            return;
        }
        Overlays.deleteOverlay(warningOverlayID);
        warningOverlayID = null;
    }

    function startPoll() {
        if (pollInterval) {
            return;
        }
        pollInterval = Script.setInterval(function() {
            var inputLoudness = Audio.inputLevel;
            var tc = (inputLoudness > lastInputLoudness) ? attackTC : releaseTC;
            inputLoudness += tc * (lastInputLoudness - inputLoudness);
            lastInputLoudness = inputLoudness;

            if (inputLoudness > 0.1) {
                holdCount = holdReset;
            } else {
                holdCount = Math.max(holdCount - 1, 0);
            }

            if (holdCount > 0) {
                showWarning();
            } else {
                hideWarning();
            }
        }, 1000.0 / sampleRate);
    }

    function stopPoll() {
        if (!pollInterval) {
            return;
        }
        Script.clearInterval(pollInterval);
        pollInterval = null;
        hideWarning();
    }

    function startOrStopPoll() {
        if (Audio.warnWhenMuted && Audio.muted) {
            startPoll();
        } else {
            stopPoll();
        }
    }

    function cleanup() {
        stopPoll();
    }

    Script.scriptEnding.connect(cleanup);

    startOrStopPoll();
    Audio.mutedChanged.connect(startOrStopPoll);
    Audio.warnWhenMutedChanged.connect(startOrStopPoll);

}()); // END LOCAL_SCOPE
