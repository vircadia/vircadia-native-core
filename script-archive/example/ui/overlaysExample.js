//
//  overlaysExample.js
//  examples
//
//  Created by Brad Hefta-Gaub on 2/14/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  This is an example script that demonstrates use of the Overlays class
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
// The "Swatches" example of this script will create 9 different image overlays, that use the color feature to
// display different colors as color swatches. The overlays can be clicked on, to change the "selectedSwatch" variable
// and update the image used for the overlay so that it appears to have a selected indicator.
// These are our colors...
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

// The location of the placement of these overlays
var swatchesX = 100;
var swatchesY = 200;

// These will be our "overlay IDs"
var swatches = new Array();
var numberOfSwatches = 9;
var selectedSwatch = 0;

// create the overlays, position them in a row, set their colors, and for the selected one, use a different source image
// location so that it displays the "selected" marker
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
                    imageURL: Script.getExternalPath(Script.ExternalPaths.Assets, "images/testing-swatches.svg"),
                    color: swatchColors[s],
                    alpha: 1
                });
}

// This will create a text overlay that when you click on it, the text will change
var text = Overlays.addOverlay("text", {
                    x: 200,
                    y: 100,
                    width: 150,
                    height: 50,
                    backgroundColor: { red: 255, green: 255, blue: 255},
                    color: { red: 255, green: 0, blue: 0},
                    topMargin: 4,
                    leftMargin: 4,
                    text: "Here is some text.\nAnd a second line.",
                    alpha: 0.7,
                    backgroundAlpha: 0.5
                });

// This will create an image overlay, which starts out as invisible
var toolA = Overlays.addOverlay("image", {
                    x: 100,
                    y: 100,
                    width: 62,
                    height: 40,
                    subImage: { x: 0, y: 0, width: 62, height: 40 },
                    imageURL: Script.getExternalPath(Script.ExternalPaths.Assets, "images/hifi-interface-tools.svg"),
                    color: { red: 255, green: 255, blue: 255},
                    visible: false
                });

// This will create a couple of image overlays that make a "slider", we will demonstrate how to trap mouse messages to
// move the slider
var slider = Overlays.addOverlay("image", {
                    // alternate form of expressing bounds
                    bounds: { x: 100, y: 300, width: 158, height: 35},
                    imageURL: Script.getExternalPath(Script.ExternalPaths.Assets, "images/slider.png"),
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });

// This is the thumb of our slider
var minThumbX = 130;
var maxThumbX = minThumbX + 65;
var thumbX = (minThumbX + maxThumbX) / 2;
var thumb = Overlays.addOverlay("image", {
                    x: thumbX,
                    y: 309,
                    width: 18,
                    height: 17,
                    imageURL: Script.getExternalPath(Script.ExternalPaths.Assets, "images/thumb.png"),
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });


// We will also demonstrate some 3D overlays. We will create a couple of cubes, spheres, and lines
// our 3D cube that moves around...
var cubePosition = { x: 2, y: 0, z: 2 };
var cubeSize = 5;
var cubeMove = 0.1;
var minCubeX = 1;
var maxCubeX = 20;

var cube = Overlays.addOverlay("cube", {
                    position: cubePosition,
                    size: cubeSize,
                    color: { red: 255, green: 0, blue: 0},
                    alpha: 1,
                    solid: false
                });

var solidCubePosition = { x: 0, y: 5, z: 0 };
var solidCubeSize = 2;
var minSolidCubeX = 0;
var maxSolidCubeX = 10;
var solidCubeMove = 0.05;
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

var line3d = Overlays.addOverlay("line3d", {
                    start: { x: 0, y: 0, z:0 },
                    end: { x: 10, y: 10, z:10 },
                    color: { red: 0, green: 255, blue: 255},
                    alpha: 1,
                    lineWidth: 5
                });

// this will display the content of your clipboard at the origin of the domain
var clipboardPreview = Overlays.addOverlay("clipboard", {
                                           position: { x: 0, y: 0, z: 0},
                                           size: 1 / 32,
                                           visible: true
                                           });

// Demonstrate retrieving overlay properties
print("Text overlay text property value =\n" + Overlays.getProperty(text, "text"));
print("Text overlay alpha =\n" + Overlays.getProperty(text, "alpha"));
print("Text overlay background alpha =\n" + Overlays.getProperty(text, "backgroundAlpha"));
print("Text overlay visible =\n" + Overlays.getProperty(text, "visible"));
print("Text overlay font size =\n" + Overlays.getProperty(text, "font").size);
print("Text overlay anchor =\n" + Overlays.getProperty(text, "anchor"));
print("Text overlay unknown property value =\n" + Overlays.getProperty(text, "unknown"));  // value = undefined
var sliderBounds = Overlays.getProperty(slider, "bounds");
print("Slider overlay bounds =\n"
    + "x: " + sliderBounds.x + "\n"
    + "y: " + sliderBounds.y + "\n"
    + "width: " + sliderBounds.width + "\n"
    + "height: " + sliderBounds.height
    );

var cubePosition = Overlays.getProperty(cube, "position");
print("Cube overlay position =\n"
    + "x: " + cubePosition.x + "\n"
    + "y: " + cubePosition.y + "\n"
    + "z: " + cubePosition.z
    );
var cubeColor = Overlays.getProperty(cube, "color");
print("Cube overlay color =\n"
    + "red:   " + cubeColor.red + "\n"
    + "green: " + cubeColor.green + "\n"
    + "blue:  " + cubeColor.blue
    );

// This model overlay example causes intermittent crashes in NetworkGeometry::setTextureWithNameToURL()
//var modelOverlayProperties = {
//    textures: {
//        filename1: Script.getExternalPath(Script.ExternalPaths.Assets, "images/testing-swatches.svg",)
//        filename2: Script.getExternalPath(Script.ExternalPaths.Assets, "images/hifi-interface-tools.svg")
//    }
//}
//var modelOverlay = Overlays.addOverlay("model", modelOverlayProperties);
//var textures = Overlays.getProperty(modelOverlay, "textures");
//var textureValues = "";
//for (key in textures) {
//    textureValues += "\n" + key + ": " + textures[key];
//}
//print("Model overlay textures =" + textureValues);
//Overlays.deleteOverlay(modelOverlay);

print("Unknown overlay property =\n" + Overlays.getProperty(1000, "text"));  // value = undefined

// When our script shuts down, we should clean up all of our overlays
function scriptEnding() {
    Overlays.deleteOverlay(toolA);
    for (s = 0; s < numberOfSwatches; s++) {
        Overlays.deleteOverlay(swatches[s]);
    }
    Overlays.deleteOverlay(thumb);
    Overlays.deleteOverlay(slider);
    Overlays.deleteOverlay(text);
    Overlays.deleteOverlay(cube);
    Overlays.deleteOverlay(solidCube);
    Overlays.deleteOverlay(sphere);
    Overlays.deleteOverlay(line3d);
    Overlays.deleteOverlay(clipboardPreview);
}
Script.scriptEnding.connect(scriptEnding);


var toolAVisible = false;
var count = 0;

// Our update() function is called at approximately 60fps, and we will use it to animate our various overlays
function update(deltaTime) {
    count++;
    
    // every second or so, toggle the visibility our our blinking tool
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


    // update our 3D line to go from origin to our avatar's position
    Overlays.editOverlay(line3d, { end: MyAvatar.position } );
}
Script.update.connect(update);


// The slider is handled in the mouse event callbacks.
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

// we also handle click detection in our mousePressEvent()
function mousePressEvent(event) {
    var clickedText = false;
    var clickedOverlay = Overlays.getOverlayAtPoint({x: event.x, y: event.y});
    
    // If the user clicked on the thumb, handle the slider logic
    if (clickedOverlay == thumb) {
        movingSlider = true;
        thumbClickOffsetX = event.x - thumbX;
        
    } else if (clickedOverlay == text) { // if the user clicked on the text, update text with where you clicked
    
        Overlays.editOverlay(text, { text: "you clicked here:\n   " + event.x + "," + event.y } );
        clickedText = true;
        
    } else { // if the user clicked on one of the color swatches, update the selectedSwatch
    
        for (s = 0; s < numberOfSwatches; s++) {
            if (clickedOverlay == swatches[s]) {
                Overlays.editOverlay(swatches[selectedSwatch], { subImage: { y: 0 } } );
                Overlays.editOverlay(swatches[s], { subImage: { y: 55 } } );
                selectedSwatch = s;
            }
        }
    }
    if (!clickedText) { // if you didn't click on the text, then update the text accordningly
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

