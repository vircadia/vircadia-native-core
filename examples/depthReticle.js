//  depthReticle.js
//  examples
//
//  Created by Brad Hefta-Gaub on 2/23/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  When used in HMD, this script will make the reticle depth track to any clickable item in view.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var APPARENT_2D_OVERLAY_DEPTH = 1.0;
var APPARENT_MAXIMUM_DEPTH = 100.0; // this is a depth at which things all seem sufficiently distant
var lastDepthCheckTime = Date.now();
var desiredDepth = APPARENT_2D_OVERLAY_DEPTH;
var wasDepth = Reticle.depth; // depth at the time we changed our desired depth
var desiredDepthLastSet = lastDepthCheckTime; // time we changed our desired depth
var TIME_BETWEEN_DEPTH_CHECKS = 100;
var TIME_TO_FADE_DEPTH = 50;

Script.update.connect(function(deltaTime) {
    var now = Date.now();
    var timeSinceLastDepthCheck = now - lastDepthCheckTime;
    if (timeSinceLastDepthCheck > TIME_BETWEEN_DEPTH_CHECKS) {
        var newDesiredDepth = desiredDepth;
        lastDepthCheckTime = now;
        var reticlePosition = Reticle.position;

        // first check the 2D Overlays
        if (Reticle.pointingAtSystemOverlay || Overlays.getOverlayAtPoint(reticlePosition)) {
            newDesiredDepth = APPARENT_2D_OVERLAY_DEPTH;
        } else {
            var pickRay = Camera.computePickRay(reticlePosition.x, reticlePosition.y);

            // Then check the 3D overlays
            var result = Overlays.findRayIntersection(pickRay);

            if (!result.intersects) {
                // finally check the entities
                result = Entities.findRayIntersection(pickRay, true);
            }

            // If either the overlays or entities intersect, then set the reticle depth to 
            // the distance of intersection
            if (result.intersects) {
                newDesiredDepth = result.distance;
            } else {
                // if nothing intersects... set the depth to some sufficiently large depth
                newDesiredDepth = APPARENT_MAXIMUM_DEPTH;
            }
        }

        // If the desired depth has changed, reset our fade start time
        if (desiredDepth != newDesiredDepth) {
            desiredDepthLastSet = now;
            desiredDepth = newDesiredDepth;
            wasDepth = Reticle.depth;
        }
    }

    // move the reticle toward the desired depth
    if (desiredDepth != Reticle.depth) {

        // determine the time between now, and when we set our determined our desiredDepth
        var elapsed = now - desiredDepthLastSet;
        var distanceToFade = desiredDepth - wasDepth;
        var percentElapsed = Math.min(1, elapsed / TIME_TO_FADE_DEPTH);

        // special case to handle no fade settings
        if (TIME_TO_FADE_DEPTH == 0) {
            percentElapsed = 1;
        }
        var depthDelta = distanceToFade * percentElapsed;

        var newDepth = wasDepth + depthDelta;
        if (percentElapsed == 1) {
            newDepth = desiredDepth;
        }
        Reticle.setDepth(newDepth);
    }
});
