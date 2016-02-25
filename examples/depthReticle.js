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
var lastDepthCheckTime = 0;

Script.update.connect(function(deltaTime) {
    var TIME_BETWEEN_DEPTH_CHECKS = 100;
    var timeSinceLastDepthCheck = Date.now() - lastDepthCheckTime;
    if (timeSinceLastDepthCheck > TIME_BETWEEN_DEPTH_CHECKS) {
        var reticlePosition = Reticle.position;

        // first check the 2D Overlays
        if (Reticle.pointingAtSystemOverlay || Overlays.getOverlayAtPoint(reticlePosition)) {
            Reticle.setDepth(APPARENT_2D_OVERLAY_DEPTH);
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
                Reticle.setDepth(result.distance);
            } else {
                // if nothing intersects... set the depth to some sufficiently large depth
                Reticle.setDepth(APPARENT_MAXIMUM_DEPTH);
            }
        }
    }
});
