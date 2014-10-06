//
//  audioReflectorTools.js
//  hifi
//
//  Created by Brad Hefta-Gaub on 2/14/14.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//
//  Tools for manipulating the attributes of the AudioReflector behavior
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("libraries/globals.js");

var delayScale = 100.0;
var fanoutScale = 10.0;
var speedScale = 20;
var factorScale = 5.0;
var localFactorScale = 1.0;
var reflectiveScale = 100.0;
var diffusionScale = 100.0;
var absorptionScale = 100.0;
var combFilterScale = 50.0;
var originalScale = 2.0;
var echoesScale = 2.0;

// these three properties are bound together, if you change one, the others will also change
var reflectiveRatio = AudioReflector.getReflectiveRatio();
var diffusionRatio = AudioReflector.getDiffusionRatio();
var absorptionRatio = AudioReflector.getAbsorptionRatio();

var reflectiveThumbX;
var diffusionThumbX;
var absorptionThumbX;

function setReflectiveRatio(reflective) {
    var total = diffusionRatio + absorptionRatio + (reflective / reflectiveScale);
    diffusionRatio = diffusionRatio / total;
    absorptionRatio = absorptionRatio / total;
    reflectiveRatio = (reflective / reflectiveScale) / total;
    updateRatioValues();
}

function setDiffusionRatio(diffusion) {
    var total = (diffusion / diffusionScale) + absorptionRatio + reflectiveRatio;
    diffusionRatio = (diffusion / diffusionScale) / total;
    absorptionRatio = absorptionRatio / total;
    reflectiveRatio = reflectiveRatio / total;
    updateRatioValues();
}

function setAbsorptionRatio(absorption) {
    var total = diffusionRatio + (absorption / absorptionScale) + reflectiveRatio;
    diffusionRatio = diffusionRatio / total;
    absorptionRatio = (absorption / absorptionScale) / total;
    reflectiveRatio = reflectiveRatio / total;
    updateRatioValues();
}

function updateRatioSliders() {
    reflectiveThumbX = reflectiveMinThumbX + ((reflectiveMaxThumbX - reflectiveMinThumbX) * reflectiveRatio);
    diffusionThumbX = diffusionMinThumbX + ((diffusionMaxThumbX - diffusionMinThumbX) * diffusionRatio);
    absorptionThumbX = absorptionMinThumbX + ((absorptionMaxThumbX - absorptionMinThumbX) * absorptionRatio);

    Overlays.editOverlay(reflectiveThumb, { x: reflectiveThumbX } );
    Overlays.editOverlay(diffusionThumb, { x: diffusionThumbX } );
    Overlays.editOverlay(absorptionThumb, { x: absorptionThumbX } );
}

function updateRatioValues() {
    AudioReflector.setReflectiveRatio(reflectiveRatio);
    AudioReflector.setDiffusionRatio(diffusionRatio);
    AudioReflector.setAbsorptionRatio(absorptionRatio);
}

var topY = 250;
var sliderHeight = 35;

var delayY = topY;
topY += sliderHeight;
var delayLabel = Overlays.addOverlay("text", {
                    x: 40,
                    y: delayY,
                    width: 60,
                    height: sliderHeight,
                    color: { red: 0, green: 0, blue: 0},
                    textColor: { red: 255, green: 255, blue: 255},
                    topMargin: 12,
                    leftMargin: 5,
                    text: "Delay:"
                });

var delaySlider = Overlays.addOverlay("image", {
                    // alternate form of expressing bounds
                    bounds: { x: 100, y: delayY, width: 150, height: sliderHeight},
                    subImage: { x: 46, y: 0, width: 200, height: 71  },
                    imageURL: HIFI_PUBLIC_BUCKET + "images/slider.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });


var delayMinThumbX = 110;
var delayMaxThumbX = delayMinThumbX + 110;
var delayThumbX = delayMinThumbX + ((delayMaxThumbX - delayMinThumbX) * (AudioReflector.getPreDelay() / delayScale));
var delayThumb = Overlays.addOverlay("image", {
                    x: delayThumbX,
                    y: delayY + 9,
                    width: 18,
                    height: 17,
                    imageURL: HIFI_PUBLIC_BUCKET + "images/thumb.png",
                    color: { red: 255, green: 0, blue: 0},
                    alpha: 1
                });

var fanoutY = topY;
topY += sliderHeight;

var fanoutLabel = Overlays.addOverlay("text", {
                    x: 40,
                    y: fanoutY,
                    width: 60,
                    height: sliderHeight,
                    color: { red: 0, green: 0, blue: 0},
                    textColor: { red: 255, green: 255, blue: 255},
                    topMargin: 12,
                    leftMargin: 5,
                    text: "Fanout:"
                });

var fanoutSlider = Overlays.addOverlay("image", {
                    // alternate form of expressing bounds
                    bounds: { x: 100, y: fanoutY, width: 150, height: sliderHeight},
                    subImage: { x: 46, y: 0, width: 200, height: 71  },
                    imageURL: HIFI_PUBLIC_BUCKET + "images/slider.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });


var fanoutMinThumbX = 110;
var fanoutMaxThumbX = fanoutMinThumbX + 110;
var fanoutThumbX = fanoutMinThumbX + ((fanoutMaxThumbX - fanoutMinThumbX) * (AudioReflector.getDiffusionFanout() / fanoutScale));
var fanoutThumb = Overlays.addOverlay("image", {
                    x: fanoutThumbX,
                    y: fanoutY + 9,
                    width: 18,
                    height: 17,
                    imageURL: HIFI_PUBLIC_BUCKET + "images/thumb.png",
                    color: { red: 255, green: 255, blue: 0},
                    alpha: 1
                });


var speedY = topY;
topY += sliderHeight;

var speedLabel = Overlays.addOverlay("text", {
                    x: 40,
                    y: speedY,
                    width: 60,
                    height: sliderHeight,
                    color: { red: 0, green: 0, blue: 0},
                    textColor: { red: 255, green: 255, blue: 255},
                    topMargin: 6,
                    leftMargin: 5,
                    text: "Speed\nin ms/m:"
                });

var speedSlider = Overlays.addOverlay("image", {
                    // alternate form of expressing bounds
                    bounds: { x: 100, y: speedY, width: 150, height: sliderHeight},
                    subImage: { x: 46, y: 0, width: 200, height: 71  },
                    imageURL: HIFI_PUBLIC_BUCKET + "images/slider.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });


var speedMinThumbX = 110;
var speedMaxThumbX = speedMinThumbX + 110;
var speedThumbX = speedMinThumbX + ((speedMaxThumbX - speedMinThumbX) * (AudioReflector.getSoundMsPerMeter() / speedScale));
var speedThumb = Overlays.addOverlay("image", {
                    x: speedThumbX,
                    y: speedY+9,
                    width: 18,
                    height: 17,
                    imageURL: HIFI_PUBLIC_BUCKET + "images/thumb.png",
                    color: { red: 0, green: 255, blue: 0},
                    alpha: 1
                });

var factorY = topY;
topY += sliderHeight;

var factorLabel = Overlays.addOverlay("text", {
                    x: 40,
                    y: factorY,
                    width: 60,
                    height: sliderHeight,
                    color: { red: 0, green: 0, blue: 0},
                    textColor: { red: 255, green: 255, blue: 255},
                    topMargin: 6,
                    leftMargin: 5,
                    text: "Attenuation\nFactor:"
                });


var factorSlider = Overlays.addOverlay("image", {
                    // alternate form of expressing bounds
                    bounds: { x: 100, y: factorY, width: 150, height: sliderHeight},
                    subImage: { x: 46, y: 0, width: 200, height: 71  },
                    imageURL: HIFI_PUBLIC_BUCKET + "images/slider.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });


var factorMinThumbX = 110;
var factorMaxThumbX = factorMinThumbX + 110;
var factorThumbX = factorMinThumbX + ((factorMaxThumbX - factorMinThumbX) * (AudioReflector.getDistanceAttenuationScalingFactor() / factorScale));
var factorThumb = Overlays.addOverlay("image", {
                    x: factorThumbX,
                    y: factorY+9,
                    width: 18,
                    height: 17,
                    imageURL: HIFI_PUBLIC_BUCKET + "images/thumb.png",
                    color: { red: 0, green: 0, blue: 255},
                    alpha: 1
                });

var localFactorY = topY;
topY += sliderHeight;

var localFactorLabel = Overlays.addOverlay("text", {
                    x: 40,
                    y: localFactorY,
                    width: 60,
                    height: sliderHeight,
                    color: { red: 0, green: 0, blue: 0},
                    textColor: { red: 255, green: 255, blue: 255},
                    topMargin: 6,
                    leftMargin: 5,
                    text: "Local\nFactor:"
                });


var localFactorSlider = Overlays.addOverlay("image", {
                    // alternate form of expressing bounds
                    bounds: { x: 100, y: localFactorY, width: 150, height: sliderHeight},
                    subImage: { x: 46, y: 0, width: 200, height: 71  },
                    imageURL: HIFI_PUBLIC_BUCKET + "images/slider.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });


var localFactorMinThumbX = 110;
var localFactorMaxThumbX = localFactorMinThumbX + 110;
var localFactorThumbX = localFactorMinThumbX + ((localFactorMaxThumbX - localFactorMinThumbX) * (AudioReflector.getLocalAudioAttenuationFactor() / localFactorScale));
var localFactorThumb = Overlays.addOverlay("image", {
                    x: localFactorThumbX,
                    y: localFactorY+9,
                    width: 18,
                    height: 17,
                    imageURL: HIFI_PUBLIC_BUCKET + "images/thumb.png",
                    color: { red: 0, green: 128, blue: 128},
                    alpha: 1
                });

var combFilterY = topY;
topY += sliderHeight;

var combFilterLabel = Overlays.addOverlay("text", {
                    x: 40,
                    y: combFilterY,
                    width: 60,
                    height: sliderHeight,
                    color: { red: 0, green: 0, blue: 0},
                    textColor: { red: 255, green: 255, blue: 255},
                    topMargin: 6,
                    leftMargin: 5,
                    text: "Comb Filter\nWindow:"
                });


var combFilterSlider = Overlays.addOverlay("image", {
                    // alternate form of expressing bounds
                    bounds: { x: 100, y: combFilterY, width: 150, height: sliderHeight},
                    subImage: { x: 46, y: 0, width: 200, height: 71  },
                    imageURL: HIFI_PUBLIC_BUCKET + "images/slider.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });


var combFilterMinThumbX = 110;
var combFilterMaxThumbX = combFilterMinThumbX + 110;
var combFilterThumbX = combFilterMinThumbX + ((combFilterMaxThumbX - combFilterMinThumbX) * (AudioReflector.getCombFilterWindow() / combFilterScale));
var combFilterThumb = Overlays.addOverlay("image", {
                    x: combFilterThumbX,
                    y: combFilterY+9,
                    width: 18,
                    height: 17,
                    imageURL: HIFI_PUBLIC_BUCKET + "images/thumb.png",
                    color: { red: 128, green: 128, blue: 0},
                    alpha: 1
                });


var reflectiveY = topY;
topY += sliderHeight;

var reflectiveLabel = Overlays.addOverlay("text", {
                    x: 40,
                    y: reflectiveY,
                    width: 60,
                    height: sliderHeight,
                    color: { red: 0, green: 0, blue: 0},
                    textColor: { red: 255, green: 255, blue: 255},
                    topMargin: 6,
                    leftMargin: 5,
                    text: "Reflective\nRatio:"
                });


var reflectiveSlider = Overlays.addOverlay("image", {
                    // alternate form of expressing bounds
                    bounds: { x: 100, y: reflectiveY, width: 150, height: sliderHeight},
                    subImage: { x: 46, y: 0, width: 200, height: 71  },
                    imageURL: HIFI_PUBLIC_BUCKET + "images/slider.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });


var reflectiveMinThumbX = 110;
var reflectiveMaxThumbX = reflectiveMinThumbX + 110;
reflectiveThumbX = reflectiveMinThumbX + ((reflectiveMaxThumbX - reflectiveMinThumbX) * AudioReflector.getReflectiveRatio());
var reflectiveThumb = Overlays.addOverlay("image", {
                    x: reflectiveThumbX,
                    y: reflectiveY+9,
                    width: 18,
                    height: 17,
                    imageURL: HIFI_PUBLIC_BUCKET + "images/thumb.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });

var diffusionY = topY;
topY += sliderHeight;

var diffusionLabel = Overlays.addOverlay("text", {
                    x: 40,
                    y: diffusionY,
                    width: 60,
                    height: sliderHeight,
                    color: { red: 0, green: 0, blue: 0},
                    textColor: { red: 255, green: 255, blue: 255},
                    topMargin: 6,
                    leftMargin: 5,
                    text: "Diffusion\nRatio:"
                });


var diffusionSlider = Overlays.addOverlay("image", {
                    // alternate form of expressing bounds
                    bounds: { x: 100, y: diffusionY, width: 150, height: sliderHeight},
                    subImage: { x: 46, y: 0, width: 200, height: 71  },
                    imageURL: HIFI_PUBLIC_BUCKET + "images/slider.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });


var diffusionMinThumbX = 110;
var diffusionMaxThumbX = diffusionMinThumbX + 110;
diffusionThumbX = diffusionMinThumbX + ((diffusionMaxThumbX - diffusionMinThumbX) * AudioReflector.getDiffusionRatio());
var diffusionThumb = Overlays.addOverlay("image", {
                    x: diffusionThumbX,
                    y: diffusionY+9,
                    width: 18,
                    height: 17,
                    imageURL: HIFI_PUBLIC_BUCKET + "images/thumb.png",
                    color: { red: 0, green: 255, blue: 255},
                    alpha: 1
                });

var absorptionY = topY;
topY += sliderHeight;

var absorptionLabel = Overlays.addOverlay("text", {
                    x: 40,
                    y: absorptionY,
                    width: 60,
                    height: sliderHeight,
                    color: { red: 0, green: 0, blue: 0},
                    textColor: { red: 255, green: 255, blue: 255},
                    topMargin: 6,
                    leftMargin: 5,
                    text: "Absorption\nRatio:"
                });


var absorptionSlider = Overlays.addOverlay("image", {
                    // alternate form of expressing bounds
                    bounds: { x: 100, y: absorptionY, width: 150, height: sliderHeight},
                    subImage: { x: 46, y: 0, width: 200, height: 71  },
                    imageURL: HIFI_PUBLIC_BUCKET + "images/slider.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });


var absorptionMinThumbX = 110;
var absorptionMaxThumbX = absorptionMinThumbX + 110;
absorptionThumbX = absorptionMinThumbX + ((absorptionMaxThumbX - absorptionMinThumbX) * AudioReflector.getAbsorptionRatio());
var absorptionThumb = Overlays.addOverlay("image", {
                    x: absorptionThumbX,
                    y: absorptionY+9,
                    width: 18,
                    height: 17,
                    imageURL: HIFI_PUBLIC_BUCKET + "images/thumb.png",
                    color: { red: 255, green: 0, blue: 255},
                    alpha: 1
                });

var originalY = topY;
topY += sliderHeight;

var originalLabel = Overlays.addOverlay("text", {
                    x: 40,
                    y: originalY,
                    width: 60,
                    height: sliderHeight,
                    color: { red: 0, green: 0, blue: 0},
                    textColor: { red: 255, green: 255, blue: 255},
                    topMargin: 6,
                    leftMargin: 5,
                    text: "Original\nMix:"
                });


var originalSlider = Overlays.addOverlay("image", {
                    // alternate form of expressing bounds
                    bounds: { x: 100, y: originalY, width: 150, height: sliderHeight},
                    subImage: { x: 46, y: 0, width: 200, height: 71  },
                    imageURL: HIFI_PUBLIC_BUCKET + "images/slider.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });


var originalMinThumbX = 110;
var originalMaxThumbX = originalMinThumbX + 110;
var originalThumbX = originalMinThumbX + ((originalMaxThumbX - originalMinThumbX) * (AudioReflector.getOriginalSourceAttenuation() / originalScale));
var originalThumb = Overlays.addOverlay("image", {
                    x: originalThumbX,
                    y: originalY+9,
                    width: 18,
                    height: 17,
                    imageURL: HIFI_PUBLIC_BUCKET + "images/thumb.png",
                    color: { red: 128, green: 128, blue: 0},
                    alpha: 1
                });

var echoesY = topY;
topY += sliderHeight;

var echoesLabel = Overlays.addOverlay("text", {
                    x: 40,
                    y: echoesY,
                    width: 60,
                    height: sliderHeight,
                    color: { red: 0, green: 0, blue: 0},
                    textColor: { red: 255, green: 255, blue: 255},
                    topMargin: 6,
                    leftMargin: 5,
                    text: "Echoes\nMix:"
                });


var echoesSlider = Overlays.addOverlay("image", {
                    // alternate form of expressing bounds
                    bounds: { x: 100, y: echoesY, width: 150, height: sliderHeight},
                    subImage: { x: 46, y: 0, width: 200, height: 71  },
                    imageURL: HIFI_PUBLIC_BUCKET + "images/slider.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });


var echoesMinThumbX = 110;
var echoesMaxThumbX = echoesMinThumbX + 110;
var echoesThumbX = echoesMinThumbX + ((echoesMaxThumbX - echoesMinThumbX) * (AudioReflector.getEchoesAttenuation() / echoesScale));
var echoesThumb = Overlays.addOverlay("image", {
                    x: echoesThumbX,
                    y: echoesY+9,
                    width: 18,
                    height: 17,
                    imageURL: HIFI_PUBLIC_BUCKET + "images/thumb.png",
                    color: { red: 128, green: 128, blue: 0},
                    alpha: 1
                });


// When our script shuts down, we should clean up all of our overlays
function scriptEnding() {
    Overlays.deleteOverlay(factorLabel);
    Overlays.deleteOverlay(factorThumb);
    Overlays.deleteOverlay(factorSlider);

    Overlays.deleteOverlay(combFilterLabel);
    Overlays.deleteOverlay(combFilterThumb);
    Overlays.deleteOverlay(combFilterSlider);

    Overlays.deleteOverlay(localFactorLabel);
    Overlays.deleteOverlay(localFactorThumb);
    Overlays.deleteOverlay(localFactorSlider);

    Overlays.deleteOverlay(speedLabel);
    Overlays.deleteOverlay(speedThumb);
    Overlays.deleteOverlay(speedSlider);
    
    Overlays.deleteOverlay(delayLabel);
    Overlays.deleteOverlay(delayThumb);
    Overlays.deleteOverlay(delaySlider);

    Overlays.deleteOverlay(fanoutLabel);
    Overlays.deleteOverlay(fanoutThumb);
    Overlays.deleteOverlay(fanoutSlider);

    Overlays.deleteOverlay(reflectiveLabel);
    Overlays.deleteOverlay(reflectiveThumb);
    Overlays.deleteOverlay(reflectiveSlider);

    Overlays.deleteOverlay(diffusionLabel);
    Overlays.deleteOverlay(diffusionThumb);
    Overlays.deleteOverlay(diffusionSlider);

    Overlays.deleteOverlay(absorptionLabel);
    Overlays.deleteOverlay(absorptionThumb);
    Overlays.deleteOverlay(absorptionSlider);

    Overlays.deleteOverlay(echoesLabel);
    Overlays.deleteOverlay(echoesThumb);
    Overlays.deleteOverlay(echoesSlider);

    Overlays.deleteOverlay(originalLabel);
    Overlays.deleteOverlay(originalThumb);
    Overlays.deleteOverlay(originalSlider);

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
var movingSliderCombFilter = false;
var movingSliderLocalFactor = false;
var movingSliderReflective = false;
var movingSliderDiffusion = false;
var movingSliderAbsorption = false;
var movingSliderOriginal = false;
var movingSliderEchoes = false;

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
    if (movingSliderCombFilter) {
        newThumbX = event.x - thumbClickOffsetX;
        if (newThumbX < combFilterMinThumbX) {
            newThumbX = combFilterMminThumbX;
        }
        if (newThumbX > combFilterMaxThumbX) {
            newThumbX = combFilterMaxThumbX;
        }
        Overlays.editOverlay(combFilterThumb, { x: newThumbX } );
        var combFilter = ((newThumbX - combFilterMinThumbX) / (combFilterMaxThumbX - combFilterMinThumbX)) * combFilterScale;
        AudioReflector.setCombFilterWindow(combFilter);
    }
    if (movingSliderLocalFactor) {
        newThumbX = event.x - thumbClickOffsetX;
        if (newThumbX < localFactorMinThumbX) {
            newThumbX = localFactorMminThumbX;
        }
        if (newThumbX > localFactorMaxThumbX) {
            newThumbX = localFactorMaxThumbX;
        }
        Overlays.editOverlay(localFactorThumb, { x: newThumbX } );
        var localFactor = ((newThumbX - localFactorMinThumbX) / (localFactorMaxThumbX - localFactorMinThumbX)) * localFactorScale;
        AudioReflector.setLocalAudioAttenuationFactor(localFactor);
    }

    if (movingSliderAbsorption) {
        newThumbX = event.x - thumbClickOffsetX;
        if (newThumbX < absorptionMinThumbX) {
            newThumbX = absorptionMminThumbX;
        }
        if (newThumbX > absorptionMaxThumbX) {
            newThumbX = absorptionMaxThumbX;
        }
        Overlays.editOverlay(absorptionThumb, { x: newThumbX } );
        var absorption = ((newThumbX - absorptionMinThumbX) / (absorptionMaxThumbX - absorptionMinThumbX)) * absorptionScale;
        setAbsorptionRatio(absorption);
    }

    if (movingSliderReflective) {
        newThumbX = event.x - thumbClickOffsetX;
        if (newThumbX < reflectiveMinThumbX) {
            newThumbX = reflectiveMminThumbX;
        }
        if (newThumbX > reflectiveMaxThumbX) {
            newThumbX = reflectiveMaxThumbX;
        }
        Overlays.editOverlay(reflectiveThumb, { x: newThumbX } );
        var reflective = ((newThumbX - reflectiveMinThumbX) / (reflectiveMaxThumbX - reflectiveMinThumbX)) * reflectiveScale;
        setReflectiveRatio(reflective);
    }

    if (movingSliderDiffusion) {
        newThumbX = event.x - thumbClickOffsetX;
        if (newThumbX < diffusionMinThumbX) {
            newThumbX = diffusionMminThumbX;
        }
        if (newThumbX > diffusionMaxThumbX) {
            newThumbX = diffusionMaxThumbX;
        }
        Overlays.editOverlay(diffusionThumb, { x: newThumbX } );
        var diffusion = ((newThumbX - diffusionMinThumbX) / (diffusionMaxThumbX - diffusionMinThumbX)) * diffusionScale;
        setDiffusionRatio(diffusion);
    }
    if (movingSliderEchoes) {
        newThumbX = event.x - thumbClickOffsetX;
        if (newThumbX < echoesMinThumbX) {
            newThumbX = echoesMminThumbX;
        }
        if (newThumbX > echoesMaxThumbX) {
            newThumbX = echoesMaxThumbX;
        }
        Overlays.editOverlay(echoesThumb, { x: newThumbX } );
        var echoes = ((newThumbX - echoesMinThumbX) / (echoesMaxThumbX - echoesMinThumbX)) * echoesScale;
        AudioReflector.setEchoesAttenuation(echoes);
    }
    if (movingSliderOriginal) {
        newThumbX = event.x - thumbClickOffsetX;
        if (newThumbX < originalMinThumbX) {
            newThumbX = originalMminThumbX;
        }
        if (newThumbX > originalMaxThumbX) {
            newThumbX = originalMaxThumbX;
        }
        Overlays.editOverlay(originalThumb, { x: newThumbX } );
        var original = ((newThumbX - originalMinThumbX) / (originalMaxThumbX - originalMinThumbX)) * originalScale;
        AudioReflector.setOriginalSourceAttenuation(original);
    }

}

// we also handle click detection in our mousePressEvent()
function mousePressEvent(event) {
    var clickedOverlay = Overlays.getOverlayAtPoint({x: event.x, y: event.y});
    if (clickedOverlay == delayThumb) {
        movingSliderDelay = true;
        thumbClickOffsetX = event.x - delayThumbX;
    }
    if (clickedOverlay == fanoutThumb) {
        movingSliderFanout = true;
        thumbClickOffsetX = event.x - fanoutThumbX;
    }
    if (clickedOverlay == speedThumb) {
        movingSliderSpeed = true;
        thumbClickOffsetX = event.x - speedThumbX;
    }
    if (clickedOverlay == factorThumb) {
        movingSliderFactor = true;
        thumbClickOffsetX = event.x - factorThumbX;
    }
    if (clickedOverlay == localFactorThumb) {
        movingSliderLocalFactor = true;
        thumbClickOffsetX = event.x - localFactorThumbX;
    }
    if (clickedOverlay == combFilterThumb) {
        movingSliderCombFilter = true;
        thumbClickOffsetX = event.x - combFilterThumbX;
    }
    if (clickedOverlay == diffusionThumb) {
        movingSliderDiffusion = true;
        thumbClickOffsetX = event.x - diffusionThumbX;
    }
    if (clickedOverlay == absorptionThumb) {
        movingSliderAbsorption = true;
        thumbClickOffsetX = event.x - absorptionThumbX;
    }
    if (clickedOverlay == reflectiveThumb) {
        movingSliderReflective = true;
        thumbClickOffsetX = event.x - reflectiveThumbX;
    }
    if (clickedOverlay == originalThumb) {
        movingSliderOriginal = true;
        thumbClickOffsetX = event.x - originalThumbX;
    }
    if (clickedOverlay == echoesThumb) {
        movingSliderEchoes = true;
        thumbClickOffsetX = event.x - echoesThumbX;
    }
}

function mouseReleaseEvent(event) {
    if (movingSliderDelay) {
        movingSliderDelay = false;
        var delay = ((newThumbX - delayMinThumbX) / (delayMaxThumbX - delayMinThumbX)) * delayScale;
        AudioReflector.setPreDelay(delay);
        delayThumbX = newThumbX;
    }
    if (movingSliderFanout) {
        movingSliderFanout = false;
        var fanout = Math.round(((newThumbX - fanoutMinThumbX) / (fanoutMaxThumbX - fanoutMinThumbX)) * fanoutScale);
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
    if (movingSliderCombFilter) {
        movingSliderCombFilter = false;
        var combFilter = ((newThumbX - combFilterMinThumbX) / (combFilterMaxThumbX - combFilterMinThumbX)) * combFilterScale;
        AudioReflector.setCombFilterWindow(combFilter);
        combFilterThumbX = newThumbX;
    }
    if (movingSliderLocalFactor) {
        movingSliderLocalFactor = false;
        var localFactor = ((newThumbX - localFactorMinThumbX) / (localFactorMaxThumbX - localFactorMinThumbX)) * localFactorScale;
        AudioReflector.setLocalAudioAttenuationFactor(localFactor);
        localFactorThumbX = newThumbX;
    }
    if (movingSliderReflective) {
        movingSliderReflective = false;
        var reflective = ((newThumbX - reflectiveMinThumbX) / (reflectiveMaxThumbX - reflectiveMinThumbX)) * reflectiveScale;
        setReflectiveRatio(reflective);
        reflectiveThumbX = newThumbX;
        updateRatioSliders();
    }
    if (movingSliderDiffusion) {
        movingSliderDiffusion = false;
        var diffusion = ((newThumbX - diffusionMinThumbX) / (diffusionMaxThumbX - diffusionMinThumbX)) * diffusionScale;
        setDiffusionRatio(diffusion);
        diffusionThumbX = newThumbX;
        updateRatioSliders();
    }
    if (movingSliderAbsorption) {
        movingSliderAbsorption = false;
        var absorption = ((newThumbX - absorptionMinThumbX) / (absorptionMaxThumbX - absorptionMinThumbX)) * absorptionScale;
        setAbsorptionRatio(absorption);
        absorptionThumbX = newThumbX;
        updateRatioSliders();
    }
    if (movingSliderEchoes) {
        movingSliderEchoes = false;
        var echoes = ((newThumbX - echoesMinThumbX) / (echoesMaxThumbX - echoesMinThumbX)) * echoesScale;
        AudioReflector.setEchoesAttenuation(echoes);
        echoesThumbX = newThumbX;
    }
    if (movingSliderOriginal) {
        movingSliderOriginal = false;
        var original = ((newThumbX - originalMinThumbX) / (originalMaxThumbX - originalMinThumbX)) * originalScale;
        AudioReflector.setOriginalSourceAttenuation(original);
        originalThumbX = newThumbX;
    }
}

Controller.mouseMoveEvent.connect(mouseMoveEvent);
Controller.mousePressEvent.connect(mousePressEvent);
Controller.mouseReleaseEvent.connect(mouseReleaseEvent);

