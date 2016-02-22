var lastDepthCheckTime = 0;

// Do some stuff regularly, like check for placement of various overlays
Script.update.connect(function(deltaTime) {
    var TIME_BETWEEN_DEPTH_CHECKS = 100;
    var timeSinceLastDepthCheck = Date.now() - lastDepthCheckTime;
    if (timeSinceLastDepthCheck > TIME_BETWEEN_DEPTH_CHECKS) {
        var reticlePosition = Reticle.position;
        var pickRay = Camera.computePickRay(reticlePosition.x, reticlePosition.y);
        var result = Overlays.findRayIntersection(pickRay);

        if (!result.intersects) {
            result = Entities.findRayIntersection(pickRay, true);
        }
        if (result.intersects) {
            // + JSON.stringify(result)
            print("something hovered!! result.distance:" +result.distance);
            Vec3.print("something hovered!! result.intersection:", result.intersection);
            Reticle.setDepth(result.distance);

        } else {
            Reticle.setDepth(100.0);
            print("NO INTERSECTION...");
            var pointAt2D = Reticle.position;
            var pointAt3D = HMD.worldPointFromOverlay(pointAt2D);
        }

    }
});
