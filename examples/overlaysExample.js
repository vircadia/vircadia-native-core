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

var swatchColors = new Array();
swatchColors[0] = { red: 255, green: 0, blue: 0};
swatchColors[1] = { red: 0, green: 255, blue: 0};
swatchColors[2] = { red: 0, green: 0, blue: 255};
swatchColors[3] = { red: 255, green: 255, blue: 0};
swatchColors[4] = { red: 255, green: 0, blue: 255};
swatchColors[5] = { red: 0, green: 255, blue: 255};
swatchColors[6] = { red: 128, green: 128, blue: 128};
swatchColors[7] = { red: 128, green: 0, blue: 0};
swatchColors[8] = { red: 0, green: 240, blue: 240};

var swatches = new Array();
var numberOfSwatches = 9;
var swatchesX = 100;
var swatchesY = 200;
var selectedSwatch = 0;
for (s = 0; s < numberOfSwatches; s++) {
    var imageFromX = 12 + (s * 27);
    var imageFromY = 0;
    if (s == selectedSwatch) {
        imageFromY = 55;
    }

    swatches[s] = Overlays.addOverlay("image", {
                    x: 100 + (30 * s),
                    y: 200,
                    width: 31,
                    height: 54,
                    subImage: { x: imageFromX, y: imageFromY, width: 30, height: 54 },
                    imageURL: "http://highfidelity-public.s3-us-west-1.amazonaws.com/images/testing-swatches.svg",
                    color: swatchColors[s],
                    alpha: 1
                });
}

var text = Overlays.addOverlay("text", {
                    x: 200,
                    y: 100,
                    width: 150,
                    height: 50,
                    color: { red: 0, green: 0, blue: 0},
                    textColor: { red: 255, green: 0, blue: 0},
                    topMargin: 4,
                    leftMargin: 4,
                    text: "Here is some text.\nAnd a second line."
                });

var toolA = Overlays.addOverlay("image", {
                    x: 100,
                    y: 100,
                    width: 62,
                    height: 40,
                    subImage: { x: 0, y: 0, width: 62, height: 40 },
                    imageURL: "https://s3-us-west-1.amazonaws.com/highfidelity-public/images/hifi-interface-tools.svg",
                    color: { red: 255, green: 255, blue: 255},
                    visible: false
                });

var slider = Overlays.addOverlay("image", {
                    // alternate form of expressing bounds
                    bounds: { x: 100, y: 300, width: 158, height: 35},
                    imageURL: "https://s3-us-west-1.amazonaws.com/highfidelity-public/images/slider.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });

var minThumbX = 130;
var maxThumbX = minThumbX + 65;
var thumbX = (minThumbX + maxThumbX) / 2;
var thumb = Overlays.addOverlay("image", {
                    x: thumbX,
                    y: 309,
                    width: 18,
                    height: 17,
                    imageURL: "https://s3-us-west-1.amazonaws.com/highfidelity-public/images/thumb.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });


// our 3D cube that moves around...
var cubePosition = { x: 2, y: 0, z: 2 };
var cubeSize = 5;
var cubeMove = 0.1;
var minCubeX = 1;
var maxCubeX = 20;
var solidCubePosition = { x: 0, y: 5, z: 0 };
var solidCubeSize = 2;
var minSolidCubeX = 0;
var maxSolidCubeX = 10;
var solidCubeMove = 0.05;

var cube = Overlays.addOverlay("cube", {
                    position: cubePosition,
                    size: cubeSize,
                    color: { red: 255, green: 0, blue: 0},
                    alpha: 1,
                    solid: false
                });

var solidCube = Overlays.addOverlay("cube", {
                    position: solidCubePosition,
                    size: solidCubeSize,
                    color: { red: 0, green: 255, blue: 0},
                    alpha: 1,
                    solid: true
                });

var spherePosition = { x: 5, y: 5, z: 5 };
var sphereSize = 1;
var minSphereSize = 0.5;
var maxSphereSize = 10;
var sphereSizeChange = 0.05;

var sphere = Overlays.addOverlay("sphere", {
                    position: spherePosition,
                    size: sphereSize,
                    color: { red: 0, green: 0, blue: 255},
                    alpha: 1,
                    solid: false
                });


function scriptEnding() {
    Overlays.deleteOverlay(toolA);
    for (s = 0; s < numberOfSwatches; s++) {
        Overlays.deleteOverlay(swatches[s]);
    }
    Overlays.deleteOverlay(thumb);
    Overlays.deleteOverlay(slider);
    Overlays.deleteOverlay(text);
}
Script.scriptEnding.connect(scriptEnding);


var toolAVisible = false;
var count = 0;
function update() {
    count++;
    if (count % 60 == 0) {
        if (toolAVisible) {
            toolAVisible = false;
        } else {
            toolAVisible = true;
        }
        Overlays.editOverlay(toolA, { visible: toolAVisible } );
    }
    
    // move our 3D cube
    cubePosition.x += cubeMove;
    cubePosition.z += cubeMove;
    if (cubePosition.x > maxCubeX || cubePosition.x < minCubeX) {
        cubeMove = cubeMove * -1;
    }
    Overlays.editOverlay(cube, { position: cubePosition } );

    // move our solid 3D cube
    solidCubePosition.x += solidCubeMove;
    solidCubePosition.z += solidCubeMove;
    if (solidCubePosition.x > maxSolidCubeX || solidCubePosition.x < minSolidCubeX) {
        solidCubeMove = solidCubeMove * -1;
    }
    Overlays.editOverlay(solidCube, { position: solidCubePosition } );

    // adjust our 3D sphere
    sphereSize += sphereSizeChange;
    if (sphereSize > maxSphereSize || sphereSize < minSphereSize) {
        sphereSizeChange = sphereSizeChange * -1;
    }
    Overlays.editOverlay(sphere, { size: sphereSize, solid: (sphereSizeChange < 0) } );
}
Script.willSendVisualDataCallback.connect(update);


var movingSlider = false;
var thumbClickOffsetX = 0;
function mouseMoveEvent(event) {
    if (movingSlider) {
        newThumbX = event.x - thumbClickOffsetX;
        if (newThumbX < minThumbX) {
            newThumbX = minThumbX;
        }
        if (newThumbX > maxThumbX) {
            newThumbX = maxThumbX;
        }
        Overlays.editOverlay(thumb, { x: newThumbX } );
    }
}
function mousePressEvent(event) {
    var clickedText = false;
    var clickedOverlay = Overlays.getOverlayAtPoint({x: event.x, y: event.y});
    if (clickedOverlay == thumb) {
        movingSlider = true;
        thumbClickOffsetX = event.x - thumbX;
    } else if (clickedOverlay == text) {
        Overlays.editOverlay(text, { text: "you clicked here:\n   " + event.x + "," + event.y } );
        clickedText = true;
    } else {
        for (s = 0; s < numberOfSwatches; s++) {
            if (clickedOverlay == swatches[s]) {
                Overlays.editOverlay(swatches[selectedSwatch], { subImage: { y: 0 } } );
                Overlays.editOverlay(swatches[s], { subImage: { y: 55 } } );
                selectedSwatch = s;
            }
        }
    }
    if (!clickedText) {
        Overlays.editOverlay(text, { text: "you didn't click here" } );
    }
}

function mouseReleaseEvent(event) {
    if (movingSlider) {
        movingSlider = false;
    }
}

Controller.mouseMoveEvent.connect(mouseMoveEvent);
Controller.mousePressEvent.connect(mousePressEvent);
Controller.mouseReleaseEvent.connect(mouseReleaseEvent);

