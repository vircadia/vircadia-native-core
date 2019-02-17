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
   var attackTC =  Math.exp(-1.0 / (sampleRate * 0.500)) // 500 milliseconds attack
   var releaseTC =  Math.exp(-1.0 / (sampleRate * 1.000)) // 1000 milliseconds release
   var holdReset = 2.0 * sampleRate; // 2 seconds hold
   var holdCount = 0;
   var warningOverlayID = null;

   function showWarning() {
       if (warningOverlayID) {
           return;
       }
       warningOverlayID = Overlays.addOverlay("text3d", {
           name: "Muted-Warning",
           localPosition: { x: 0.2, y: -0.35, z: -1.0 },
           localOrientation: Quat.fromVec3Degrees({ x: 0.0, y: 0.0, z: 0.0, w: 1.0 }),
           text: "Warning: you are muted",
           textAlpha: 1,
           color: { red: 226, green: 51, blue: 77 },
           backgroundAlpha: 0,
           lineHeight: 0.042,
           visible: true,
           ignoreRayIntersection: true,
           drawInFront: true,
           grabbable: false,
           parentID: MyAvatar.SELF_ID,
           parentJointIndex: MyAvatar.getJointIndex("_CAMERA_MATRIX")
       });
   };

   function hideWarning() {
       if (!warningOverlayID) {
           return;
       }
       Overlays.deleteOverlay(warningOverlayID);
       warningOverlayID = null;
   }

   function cleanup() {
       Overlays.deleteOverlay(warningOverlayID);
   }

   Script.scriptEnding.connect(cleanup);

   Script.setInterval(function() {

       var inputLoudness = Audio.inputLevel;
       var tc = (inputLoudness > lastInputLoudness) ? attackTC : releaseTC;
       inputLoudness += tc * (lastInputLoudness - inputLoudness);
       lastInputLoudness = inputLoudness;

       if (Audio.muted && inputLoudness > 0.3) {
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

}()); // END LOCAL_SCOPE
