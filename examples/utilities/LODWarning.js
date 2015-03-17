var DISPLAY_WARNING_FOR = 3; // in seconds
var DISTANCE_FROM_CAMERA = 2;
var SHOW_LOD_UP_MESSAGE = false; // By default we only display the LOD message when reducing LOD


var warningIsVisible = false; // initially the warning is hidden
var warningShownAt = 0;
var billboardPosition = Vec3.sum(Camera.getPosition(), 
                                    Vec3.multiply(DISTANCE_FROM_CAMERA, Quat.getFront(Camera.getOrientation())));
                                    
var warningOverlay = Overlays.addOverlay("text3d", {
                    position: billboardPosition,
                    dimensions: { x: 2, y: 1.25 },
                    width: 2,
                    height: 1.25,
                    backgroundColor: { red: 0, green: 0, blue: 0 },
                    color: { red: 255, green: 255, blue: 255},
                    topMargin: 0.1,
                    leftMargin: 0.1,
                    lineHeight: 0.07,
                    text: "",
                    alpha: 0.5,
                    backgroundAlpha: 0.7,
                    isFacingAvatar: true,
                    visible: warningIsVisible,
                });

// Handle moving the billboard to remain in front of the camera
var billboardNeedsMoving = false;
Script.update.connect(function() {

    if (warningIsVisible) {
        var bestBillboardPosition = Vec3.sum(Camera.getPosition(), 
                                                Vec3.multiply(DISTANCE_FROM_CAMERA, Quat.getFront(Camera.getOrientation())));
    
        var MAX_DISTANCE = 0.5;
        var CLOSE_ENOUGH = 0.01;
        if (!billboardNeedsMoving && Vec3.distance(bestBillboardPosition, billboardPosition) > MAX_DISTANCE) {
            billboardNeedsMoving = true;
        }

        if (billboardNeedsMoving && Vec3.distance(bestBillboardPosition, billboardPosition) <= CLOSE_ENOUGH) {
            billboardNeedsMoving = false;
        }
    
        if (billboardNeedsMoving) {
            // slurp the billboard to the best location
            moveVector = Vec3.multiply(0.05, Vec3.subtract(bestBillboardPosition, billboardPosition));
            billboardPosition = Vec3.sum(billboardPosition, moveVector);
            Overlays.editOverlay(warningOverlay, { position: billboardPosition });
        }

        var now = new Date();
        var sinceWarningShown = now - warningShownAt;
        if (sinceWarningShown > 1000 * DISPLAY_WARNING_FOR) {
            warningIsVisible = false;
            Overlays.editOverlay(warningOverlay, { visible: warningIsVisible });
        }
    }
});

LODManager.LODIncreased.connect(function() {
    if (SHOW_LOD_UP_MESSAGE) {
        // if the warning wasn't visible, then move it before showing it.
        if (!warningIsVisible) {
            billboardPosition = Vec3.sum(Camera.getPosition(), 
                                                    Vec3.multiply(DISTANCE_FROM_CAMERA, Quat.getFront(Camera.getOrientation())));
            Overlays.editOverlay(warningOverlay, { position: billboardPosition });
        }

        warningShownAt = new Date();
        warningIsVisible = true;
        warningText = "Level of detail has been increased. \n"
                + "You can now see: \n" 
                + LODManager.getLODFeedbackText();

        Overlays.editOverlay(warningOverlay, { visible: warningIsVisible, text: warningText });
    }
});

LODManager.LODDecreased.connect(function() {
    // if the warning wasn't visible, then move it before showing it.
    if (!warningIsVisible) {
        billboardPosition = Vec3.sum(Camera.getPosition(), 
                                                Vec3.multiply(DISTANCE_FROM_CAMERA, Quat.getFront(Camera.getOrientation())));
        Overlays.editOverlay(warningOverlay, { position: billboardPosition });
    }

    warningShownAt = new Date();
    warningIsVisible = true;
    warningText = "\n"
            + "Due to the complexity of the content, the \n"
            + "level of detail has been decreased. \n"
            + "You can now see: \n" 
            + LODManager.getLODFeedbackText();

    Overlays.editOverlay(warningOverlay, { visible: warningIsVisible, text: warningText });
});


Script.scriptEnding.connect(function() {
    Overlays.deleteOverlay(warningOverlay);
});