//
//  overlaysExample.js
//  hifi
//
//  Created by Brad Hefta-Gaub on 2/14/14.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//
//  This is an example script that demonstrates use of the Overlays class
//
//


var delayScale = 500.0;
var fanoutScale = 10.0;
var speedScale = 20;
var factorScale = 5.0;

// This will create a couple of image overlays that make a "slider", we will demonstrate how to trap mouse messages to
// move the slider
var delayY = 250;
var delaySlider = Overlays.addOverlay("image", {
                    // alternate form of expressing bounds
                    bounds: { x: 100, y: delayY, width: 150, height: 35},
                    subImage: { x: 46, y: 0, width: 200, height: 71  },
                    imageURL: "https://s3-us-west-1.amazonaws.com/highfidelity-public/images/slider.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });

// This is the thumb of our slider
var delayMinThumbX = 110;
var delayMaxThumbX = delayMinThumbX + 110;
var delayThumbX = (delayMinThumbX + delayMaxThumbX) / 2;
var delayThumb = Overlays.addOverlay("image", {
                    x: delayThumbX,
                    y: delayY + 9,
                    width: 18,
                    height: 17,
                    imageURL: "https://s3-us-west-1.amazonaws.com/highfidelity-public/images/thumb.png",
                    color: { red: 255, green: 0, blue: 0},
                    alpha: 1
                });

// This will create a couple of image overlays that make a "slider", we will demonstrate how to trap mouse messages to
// move the slider
var fanoutY = 300;
var fanoutSlider = Overlays.addOverlay("image", {
                    // alternate form of expressing bounds
                    bounds: { x: 100, y: fanoutY, width: 150, height: 35},
                    subImage: { x: 46, y: 0, width: 200, height: 71  },
                    imageURL: "https://s3-us-west-1.amazonaws.com/highfidelity-public/images/slider.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });

// This is the thumb of our slider
var fanoutMinThumbX = 110;
var fanoutMaxThumbX = fanoutMinThumbX + 110;
var fanoutThumbX = (fanoutMinThumbX + fanoutMaxThumbX) / 2;
var fanoutThumb = Overlays.addOverlay("image", {
                    x: fanoutThumbX,
                    y: fanoutY + 9,
                    width: 18,
                    height: 17,
                    imageURL: "https://s3-us-west-1.amazonaws.com/highfidelity-public/images/thumb.png",
                    color: { red: 255, green: 255, blue: 0},
                    alpha: 1
                });


// This will create a couple of image overlays that make a "slider", we will demonstrate how to trap mouse messages to
// move the slider
var speedY = 350;
var speedSlider = Overlays.addOverlay("image", {
                    // alternate form of expressing bounds
                    bounds: { x: 100, y: speedY, width: 150, height: 35},
                    subImage: { x: 46, y: 0, width: 200, height: 71  },
                    imageURL: "https://s3-us-west-1.amazonaws.com/highfidelity-public/images/slider.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });

// This is the thumb of our slider
var speedMinThumbX = 110;
var speedMaxThumbX = speedMinThumbX + 110;
var speedThumbX = (speedMinThumbX + speedMaxThumbX) / 2;
var speedThumb = Overlays.addOverlay("image", {
                    x: speedThumbX,
                    y: speedY+9,
                    width: 18,
                    height: 17,
                    imageURL: "https://s3-us-west-1.amazonaws.com/highfidelity-public/images/thumb.png",
                    color: { red: 0, green: 255, blue: 0},
                    alpha: 1
                });

// This will create a couple of image overlays that make a "slider", we will demonstrate how to trap mouse messages to
// move the slider
var factorY = 400;
var factorSlider = Overlays.addOverlay("image", {
                    // alternate form of expressing bounds
                    bounds: { x: 100, y: factorY, width: 150, height: 35},
                    subImage: { x: 46, y: 0, width: 200, height: 71  },
                    imageURL: "https://s3-us-west-1.amazonaws.com/highfidelity-public/images/slider.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });

// This is the thumb of our slider
var factorMinThumbX = 110;
var factorMaxThumbX = factorMinThumbX + 110;
var factorThumbX = (factorMinThumbX + factorMaxThumbX) / 2;
var factorThumb = Overlays.addOverlay("image", {
                    x: factorThumbX,
                    y: factorY+9,
                    width: 18,
                    height: 17,
                    imageURL: "https://s3-us-west-1.amazonaws.com/highfidelity-public/images/thumb.png",
                    color: { red: 0, green: 0, blue: 255},
                    alpha: 1
                });


// When our script shuts down, we should clean up all of our overlays
function scriptEnding() {
    Overlays.deleteOverlay(factorThumb);
    Overlays.deleteOverlay(factorSlider);
    Overlays.deleteOverlay(speedThumb);
    Overlays.deleteOverlay(speedSlider);
    Overlays.deleteOverlay(delayThumb);
    Overlays.deleteOverlay(delaySlider);
    Overlays.deleteOverlay(fanoutThumb);
    Overlays.deleteOverlay(fanoutSlider);
}
Script.scriptEnding.connect(scriptEnding);


var count = 0;

// Our update() function is called at approximately 60fps, and we will use it to animate our various overlays
function update(deltaTime) {
    count++;
}
Script.update.connect(update);


// The slider is handled in the mouse event callbacks.
var movingSliderDelay = false;
var movingSliderFanout = false;
var movingSliderSpeed = false;
var movingSliderFactor = false;
var thumbClickOffsetX = 0;
function mouseMoveEvent(event) {
    if (movingSliderDelay) {
        newThumbX = event.x - thumbClickOffsetX;
        if (newThumbX < delayMinThumbX) {
            newThumbX = delayMinThumbX;
        }
        if (newThumbX > delayMaxThumbX) {
            newThumbX = delayMaxThumbX;
        }
        Overlays.editOverlay(delayThumb, { x: newThumbX } );
        var delay = ((newThumbX - delayMinThumbX) / (delayMaxThumbX - delayMinThumbX)) * delayScale;
        print("delay="+delay);
        AudioReflector.setPreDelay(delay);
    }
    if (movingSliderFanout) {
        newThumbX = event.x - thumbClickOffsetX;
        if (newThumbX < fanoutMinThumbX) {
            newThumbX = fanoutMinThumbX;
        }
        if (newThumbX > fanoutMaxThumbX) {
            newThumbX = fanoutMaxThumbX;
        }
        Overlays.editOverlay(fanoutThumb, { x: newThumbX } );
        var fanout = Math.round(((newThumbX - fanoutMinThumbX) / (fanoutMaxThumbX - fanoutMinThumbX)) * fanoutScale);
        print("fanout="+fanout);
        AudioReflector.setDiffusionFanout(fanout);
    }
    if (movingSliderSpeed) {
        newThumbX = event.x - thumbClickOffsetX;
        if (newThumbX < speedMinThumbX) {
            newThumbX = speedMminThumbX;
        }
        if (newThumbX > speedMaxThumbX) {
            newThumbX = speedMaxThumbX;
        }
        Overlays.editOverlay(speedThumb, { x: newThumbX } );
        var speed = ((newThumbX - speedMinThumbX) / (speedMaxThumbX - speedMinThumbX)) * speedScale;
        AudioReflector.setSoundMsPerMeter(speed);
    }
    if (movingSliderFactor) {
        newThumbX = event.x - thumbClickOffsetX;
        if (newThumbX < factorMinThumbX) {
            newThumbX = factorMminThumbX;
        }
        if (newThumbX > factorMaxThumbX) {
            newThumbX = factorMaxThumbX;
        }
        Overlays.editOverlay(factorThumb, { x: newThumbX } );
        var factor = ((newThumbX - factorMinThumbX) / (factorMaxThumbX - factorMinThumbX)) * factorScale;
        AudioReflector.setDistanceAttenuationScalingFactor(factor);
    }
}

// we also handle click detection in our mousePressEvent()
function mousePressEvent(event) {
    var clickedOverlay = Overlays.getOverlayAtPoint({x: event.x, y: event.y});

    // If the user clicked on the thumb, handle the slider logic
    if (clickedOverlay == delayThumb) {
        movingSliderDelay = true;
        thumbClickOffsetX = event.x - delayThumbX;
    }
    // If the user clicked on the thumb, handle the slider logic
    if (clickedOverlay == fanoutThumb) {
        movingSliderFanout = true;
        thumbClickOffsetX = event.x - fanoutThumbX;
    }
    // If the user clicked on the thumb, handle the slider logic
    if (clickedOverlay == speedThumb) {
        movingSliderSpeed = true;
        thumbClickOffsetX = event.x - speedThumbX;
    }

    // If the user clicked on the thumb, handle the slider logic
    if (clickedOverlay == factorThumb) {
print("movingSliderFactor...");
        movingSliderFactor = true;
        thumbClickOffsetX = event.x - factorThumbX;
    }
}
function mouseReleaseEvent(event) {
    if (movingSliderDelay) {
        movingSliderDelay = false;
        var delay = ((newThumbX - delayMinThumbX) / (delayMaxThumbX - delayMinThumbX)) * delayScale;
        print("delay="+delay);
        AudioReflector.setPreDelay(delay);
        delayThumbX = newThumbX;
    }
    if (movingSliderFanout) {
        movingSliderFanout = false;
        var fanout = Math.round(((newThumbX - fanoutMinThumbX) / (fanoutMaxThumbX - fanoutMinThumbX)) * fanoutScale);
        print("fanout="+fanout);
        AudioReflector.setDiffusionFanout(fanout);
        fanoutThumbX = newThumbX;
    }
    if (movingSliderSpeed) {
        movingSliderSpeed = false;
        var speed = ((newThumbX - speedMinThumbX) / (speedMaxThumbX - speedMinThumbX)) * speedScale;
        AudioReflector.setSoundMsPerMeter(speed);
        speedThumbX = newThumbX;
    }
    if (movingSliderFactor) {
        movingSliderFactor = false;
        var factor = ((newThumbX - factorMinThumbX) / (factorMaxThumbX - factorMinThumbX)) * factorScale;
        AudioReflector.setDistanceAttenuationScalingFactor(factor);
        factorThumbX = newThumbX;
    }
}

Controller.mouseMoveEvent.connect(mouseMoveEvent);
Controller.mousePressEvent.connect(mousePressEvent);
Controller.mouseReleaseEvent.connect(mouseReleaseEvent);

