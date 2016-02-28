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
var TIME_BETWEEN_DEPTH_CHECKS = 100;
var MINIMUM_DEPTH_ADJUST = 0.01;
var NON_LINEAR_DIVISOR = 2;

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
            desiredDepth = newDesiredDepth;
        }
    }

    // move the reticle toward the desired depth
    if (desiredDepth != Reticle.depth) {

        // cut distance between desiredDepth and current depth in half until we're close enough
        var distanceToAdjustThisCycle = (desiredDepth - Reticle.depth) / NON_LINEAR_DIVISOR;
        if (Math.abs(distanceToAdjustThisCycle) < MINIMUM_DEPTH_ADJUST) {
            newDepth = desiredDepth;
        } else {
            newDepth = Reticle.depth + distanceToAdjustThisCycle;
        }

        Reticle.setDepth(newDepth);
    }
});
