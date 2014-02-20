//
//  editVoxels.js
//  hifi
//
//  Created by Philip Rosedale on February 8, 2014
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//
//  Captures mouse clicks and edits voxels accordingly.
//
//  click = create a new voxel on this face, same color as old (default color picker state)
//  right click or control + click = delete this voxel 
//  shift + click = recolor this voxel
//  1 - 8 = pick new color from palette
//  9 = create a new voxel in front of the camera 
//
//  Click and drag to create more new voxels in the same direction
//

var windowDimensions = Controller.getViewportDimensions();

var NEW_VOXEL_SIZE = 1.0;
var NEW_VOXEL_DISTANCE_FROM_CAMERA = 3.0;
var ORBIT_RATE_ALTITUDE = 200.0;
var ORBIT_RATE_AZIMUTH = 90.0;
var PIXELS_PER_EXTRUDE_VOXEL = 16;

var oldMode = Camera.getMode();

var key_alt = false;
var key_shift = false;
var isAdding = false; 
var isExtruding = false; 
var isOrbiting = false;
var orbitAzimuth = 0.0;
var orbitAltitude = 0.0;
var orbitCenter = { x: 0, y: 0, z: 0 };
var orbitPosition = { x: 0, y: 0, z: 0 };
var orbitRadius = 0.0;
var extrudeDirection = { x: 0, y: 0, z: 0 };
var extrudeScale = 0.0;
var lastVoxelPosition = { x: 0, y: 0, z: 0 };
var lastVoxelColor = { red: 0, green: 0, blue: 0 };
var lastVoxelScale = 0;
var dragStart = { x: 0, y: 0 };

var mouseX = 0;
var mouseY = 0; 



//  Create a table of the different colors you can choose
var colors = new Array();
colors[0] = { red: 237, green: 175, blue: 0 };
colors[1] = { red: 61,  green: 211, blue: 72 };
colors[2] = { red: 51,  green: 204, blue: 204 };
colors[3] = { red: 63,  green: 169, blue: 245 };
colors[4] = { red: 193, green: 99,  blue: 122 };
colors[5] = { red: 255, green: 54,  blue: 69 };
colors[6] = { red: 124, green: 36,  blue: 36 };
colors[7] = { red: 63,  green: 35,  blue: 19 };
var numColors = 8;
var whichColor = -1;            //  Starting color is 'Copy' mode

//  Create sounds for adding, deleting, recoloring voxels 
var addSound = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Voxels/voxel+create.raw");
var deleteSound = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Voxels/voxel+delete.raw");
var changeColorSound = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Voxels/voxel+edit.raw");
var clickSound = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Switches+and+sliders/toggle+switch+-+medium.raw");
var audioOptions = new AudioInjectionOptions(); 
audioOptions.volume = 0.5;

var editToolsOn = false; // starts out off


// previewAsVoxel - by default, we will preview adds/deletes/recolors as just 4 lines on the intersecting face. But if you
//                  the preview to show a full voxel then set this to true and the voxel will be displayed for voxel editing
var previewAsVoxel = false; 

var voxelPreview = Overlays.addOverlay("cube", {
                    position: { x: 0, y: 0, z: 0},
                    size: 1,
                    color: { red: 255, green: 0, blue: 0},
                    alpha: 1,
                    solid: false,
                    visible: false,
                    lineWidth: 4
                });
                
var linePreviewTop = Overlays.addOverlay("line3d", {
                    position: { x: 0, y: 0, z: 0},
                    end: { x: 0, y: 0, z: 0},
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false,
                    lineWidth: 1
                });

var linePreviewBottom = Overlays.addOverlay("line3d", {
                    position: { x: 0, y: 0, z: 0},
                    end: { x: 0, y: 0, z: 0},
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false,
                    lineWidth: 1
                });

var linePreviewLeft = Overlays.addOverlay("line3d", {
                    position: { x: 0, y: 0, z: 0},
                    end: { x: 0, y: 0, z: 0},
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false,
                    lineWidth: 1
                });

var linePreviewRight = Overlays.addOverlay("line3d", {
                    position: { x: 0, y: 0, z: 0},
                    end: { x: 0, y: 0, z: 0},
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false,
                    lineWidth: 1
                });


// These will be our "overlay IDs"
var swatches = new Array();
var swatchHeight = 54;
var swatchWidth = 31;
var swatchesWidth = swatchWidth * numColors;
var swatchesX = (windowDimensions.x - swatchesWidth) / 2;
var swatchesY = windowDimensions.y - swatchHeight;

// create the overlays, position them in a row, set their colors, and for the selected one, use a different source image
// location so that it displays the "selected" marker
for (s = 0; s < numColors; s++) {
    var imageFromX = 12 + (s * 27);
    var imageFromY = 0;
    if (s == whichColor) {
        imageFromY = 55;
    }
    var swatchX = swatchesX + (30 * s);

    swatches[s] = Overlays.addOverlay("image", {
                    x: swatchX,
                    y: swatchesY,
                    width: swatchWidth,
                    height: swatchHeight,
                    subImage: { x: imageFromX, y: imageFromY, width: (swatchWidth - 1), height: swatchHeight },
                    imageURL: "http://highfidelity-public.s3-us-west-1.amazonaws.com/images/testing-swatches.svg",
                    color: colors[s],
                    alpha: 1,
                    visible: editToolsOn
                });
}


// These will be our tool palette overlays
var numberOfTools = 5;
var toolHeight = 40;
var toolWidth = 62;
var toolsHeight = toolHeight * numberOfTools;
var toolsX = 0;
var toolsY = (windowDimensions.y - toolsHeight) / 2;

var addToolAt = 0;
var deleteToolAt = 1;
var recolorToolAt = 2;
var eyedropperToolAt = 3;
var selectToolAt = 4;
var toolSelectedColor = { red: 255, green: 255, blue: 255 };
var notSelectedColor = { red: 128, green: 128, blue: 128 };

var addTool = Overlays.addOverlay("image", {
                    x: 0, y: 0, width: toolWidth, height: toolHeight,
                    subImage: { x: 0, y: toolHeight * addToolAt, width: toolWidth, height: toolHeight },
                    imageURL: "https://s3-us-west-1.amazonaws.com/highfidelity-public/images/hifi-interface-tools.svg",
                    color: toolSelectedColor,
                    visible: false,
                    alpha: 0.9
                });

var deleteTool = Overlays.addOverlay("image", {
                    x: 0, y: 0, width: toolWidth, height: toolHeight,
                    subImage: { x: 0, y: toolHeight * deleteToolAt, width: toolWidth, height: toolHeight },
                    imageURL: "https://s3-us-west-1.amazonaws.com/highfidelity-public/images/hifi-interface-tools.svg",
                    color: toolSelectedColor,
                    visible: false,
                    alpha: 0.9
                });

var recolorTool = Overlays.addOverlay("image", {
                    x: 0, y: 0, width: toolWidth, height: toolHeight,
                    subImage: { x: 0, y: toolHeight * recolorToolAt, width: toolWidth, height: toolHeight },
                    imageURL: "https://s3-us-west-1.amazonaws.com/highfidelity-public/images/hifi-interface-tools.svg",
                    color: toolSelectedColor,
                    visible: false,
                    alpha: 0.9
                });

var eyedropperTool = Overlays.addOverlay("image", {
                    x: 0, y: 0, width: toolWidth, height: toolHeight,
                    subImage: { x: 0, y: toolHeight * eyedropperToolAt, width: toolWidth, height: toolHeight },
                    imageURL: "https://s3-us-west-1.amazonaws.com/highfidelity-public/images/hifi-interface-tools.svg",
                    color: toolSelectedColor,
                    visible: false,
                    alpha: 0.9
                });

var selectTool = Overlays.addOverlay("image", {
                    x: 0, y: 0, width: toolWidth, height: toolHeight,
                    subImage: { x: 0, y: toolHeight * selectToolAt, width: toolWidth, height: toolHeight },
                    imageURL: "https://s3-us-west-1.amazonaws.com/highfidelity-public/images/hifi-interface-tools.svg",
                    color: toolSelectedColor,
                    visible: false,
                    alpha: 0.9
                });


function setAudioPosition() {
    var camera = Camera.getPosition();
    var forwardVector = Quat.getFront(MyAvatar.orientation);
    audioOptions.position = Vec3.sum(camera, forwardVector);
}

function getNewVoxelPosition() { 
    var camera = Camera.getPosition();
    var forwardVector = Quat.getFront(MyAvatar.orientation);
    var newPosition = Vec3.sum(camera, Vec3.multiply(forwardVector, NEW_VOXEL_DISTANCE_FROM_CAMERA));
    return newPosition;
}

function fixEulerAngles(eulers) {
    var rVal = { x: 0, y: 0, z: eulers.z };
    if (eulers.x >= 90.0) {
        rVal.x = 180.0 - eulers.x;
        rVal.y = eulers.y - 180.0;
    } else if (eulers.x <= -90.0) {
        rVal.x = 180.0 - eulers.x;
        rVal.y = eulers.y - 180.0;
    }
    return rVal;
}

var trackLastMouseX = 0;
var trackLastMouseY = 0;
var trackAsDelete = false;
var trackAsRecolor = false;
var trackAsEyedropper = false;
var trackAsOrbit = false;

function showPreviewVoxel() {
    var voxelColor;

    var pickRay = Camera.computePickRay(trackLastMouseX, trackLastMouseY);
    var intersection = Voxels.findRayIntersection(pickRay);

    if (whichColor == -1) {
        //  Copy mode - use clicked voxel color
        voxelColor = { red: intersection.voxel.red,
                  green: intersection.voxel.green,
                  blue: intersection.voxel.blue };
    } else {
        voxelColor = { red: colors[whichColor].red,
                  green: colors[whichColor].green,
                  blue: colors[whichColor].blue };
    }

    var guidePosition;
    
    if (trackAsDelete) {
        guidePosition = { x: intersection.voxel.x,
                          y: intersection.voxel.y,
                          z: intersection.voxel.z };
                          
        Overlays.editOverlay(voxelPreview, { 
                position: guidePosition,
                size: intersection.voxel.s,
                visible: true,
                color: { red: 255, green: 0, blue: 0 },
                solid: false,
                alpha: 1
            });
    } else if (trackAsRecolor || trackAsEyedropper) {
        guidePosition = { x: intersection.voxel.x - 0.001,
                          y: intersection.voxel.y - 0.001,
                          z: intersection.voxel.z - 0.001 };

        Overlays.editOverlay(voxelPreview, { 
                position: guidePosition,
                size: intersection.voxel.s + 0.002,
                visible: true,
                color: voxelColor,
                solid: true,
                alpha: 0.8
            });
    } else if (trackAsOrbit) {
        Overlays.editOverlay(voxelPreview, { visible: false });
    } else if (!isExtruding) {
        guidePosition = { x: intersection.voxel.x,
                          y: intersection.voxel.y,
                          z: intersection.voxel.z };
        
        if (intersection.face == "MIN_X_FACE") {
            guidePosition.x -= intersection.voxel.s;
        } else if (intersection.face == "MAX_X_FACE") {
            guidePosition.x += intersection.voxel.s;
        } else if (intersection.face == "MIN_Y_FACE") {
            guidePosition.y -= intersection.voxel.s;
        } else if (intersection.face == "MAX_Y_FACE") {
            guidePosition.y += intersection.voxel.s;
        } else if (intersection.face == "MIN_Z_FACE") {
            guidePosition.z -= intersection.voxel.s;
        } else if (intersection.face == "MAX_Z_FACE") {
            guidePosition.z += intersection.voxel.s;
        }

        Overlays.editOverlay(voxelPreview, { 
                position: guidePosition,
                size: intersection.voxel.s,
                visible: true,
                color: voxelColor,
                solid: true,
                alpha: 0.7
            });
    } else if (isExtruding) {
        Overlays.editOverlay(voxelPreview, { visible: false });
    }
}


function showPreviewLines() {

    var pickRay = Camera.computePickRay(trackLastMouseX, trackLastMouseY);
    var intersection = Voxels.findRayIntersection(pickRay);
    
    if (intersection.intersects) {
    
        // TODO: add support for changing size here, if you set this size to any arbitrary size, 
        // the preview should correctly handle it
        var previewVoxelSize = intersection.voxel.s; 

        var x = Math.floor(intersection.intersection.x / previewVoxelSize) * previewVoxelSize;
        var y = Math.floor(intersection.intersection.y / previewVoxelSize) * previewVoxelSize;
        var z = Math.floor(intersection.intersection.z / previewVoxelSize) * previewVoxelSize;
        previewVoxel = { x: x, y: y, z: z, s: previewVoxelSize };
    
        var bottomLeft;
        var bottomRight;
        var topLeft;
        var topRight;

        if (intersection.face == "MIN_X_FACE") {
            previewVoxel.x = Math.floor(intersection.voxel.x / previewVoxelSize) * previewVoxelSize;
            bottomLeft = {x: previewVoxel.x, y: previewVoxel.y, z: previewVoxel.z };
            bottomRight = {x: previewVoxel.x, y: previewVoxel.y, z: previewVoxel.z + previewVoxel.s };
            topLeft = {x: previewVoxel.x, y: previewVoxel.y + previewVoxel.s, z: previewVoxel.z };
            topRight = {x: previewVoxel.x, y: previewVoxel.y + previewVoxel.s, z: previewVoxel.z + previewVoxel.s };

        } else if (intersection.face == "MAX_X_FACE") {
            previewVoxel.x = Math.floor((intersection.voxel.x + intersection.voxel.s) / previewVoxelSize) * previewVoxelSize;
            bottomLeft = {x: previewVoxel.x, y: previewVoxel.y, z: previewVoxel.z };
            bottomRight = {x: previewVoxel.x, y: previewVoxel.y, z: previewVoxel.z + previewVoxel.s };
            topLeft = {x: previewVoxel.x, y: previewVoxel.y + previewVoxel.s, z: previewVoxel.z };
            topRight = {x: previewVoxel.x, y: previewVoxel.y + previewVoxel.s, z: previewVoxel.z + previewVoxel.s };

        } else if (intersection.face == "MIN_Y_FACE") {

            previewVoxel.y = Math.floor(intersection.voxel.y / previewVoxelSize) * previewVoxelSize;
            bottomLeft = {x: previewVoxel.x + previewVoxel.s, y: previewVoxel.y, z: previewVoxel.z };
            bottomRight = {x: previewVoxel.x, y: previewVoxel.y, z: previewVoxel.z };
            topLeft = {x: previewVoxel.x + previewVoxel.s, y: previewVoxel.y, z: previewVoxel.z + previewVoxel.s};
            topRight = {x: previewVoxel.x, y: previewVoxel.y, z: previewVoxel.z  + previewVoxel.s};

        } else if (intersection.face == "MAX_Y_FACE") {

            previewVoxel.y = Math.floor((intersection.voxel.y + intersection.voxel.s) / previewVoxelSize) * previewVoxelSize;
            bottomLeft = {x: previewVoxel.x + previewVoxel.s, y: previewVoxel.y, z: previewVoxel.z };
            bottomRight = {x: previewVoxel.x, y: previewVoxel.y, z: previewVoxel.z };
            topLeft = {x: previewVoxel.x + previewVoxel.s, y: previewVoxel.y, z: previewVoxel.z + previewVoxel.s};
            topRight = {x: previewVoxel.x, y: previewVoxel.y, z: previewVoxel.z  + previewVoxel.s};

        } else if (intersection.face == "MIN_Z_FACE") {

            previewVoxel.z = Math.floor(intersection.voxel.z / previewVoxelSize) * previewVoxelSize;
            bottomLeft = {x: previewVoxel.x + previewVoxel.s, y: previewVoxel.y, z: previewVoxel.z };
            bottomRight = {x: previewVoxel.x, y: previewVoxel.y, z: previewVoxel.z };
            topLeft = {x: previewVoxel.x + previewVoxel.s, y: previewVoxel.y + previewVoxel.s, z: previewVoxel.z};
            topRight = {x: previewVoxel.x, y: previewVoxel.y + previewVoxel.s, z: previewVoxel.z};

        } else if (intersection.face == "MAX_Z_FACE") {

            previewVoxel.z = Math.floor((intersection.voxel.z + intersection.voxel.s) / previewVoxelSize) * previewVoxelSize;
            bottomLeft = {x: previewVoxel.x + previewVoxel.s, y: previewVoxel.y, z: previewVoxel.z };
            bottomRight = {x: previewVoxel.x, y: previewVoxel.y, z: previewVoxel.z };
            topLeft = {x: previewVoxel.x + previewVoxel.s, y: previewVoxel.y + previewVoxel.s, z: previewVoxel.z};
            topRight = {x: previewVoxel.x, y: previewVoxel.y + previewVoxel.s, z: previewVoxel.z};

        }

        Overlays.editOverlay(linePreviewTop, { position: topLeft, end: topRight, visible: true });
        Overlays.editOverlay(linePreviewBottom, { position: bottomLeft, end: bottomRight, visible: true });
        Overlays.editOverlay(linePreviewLeft, { position: topLeft, end: bottomLeft, visible: true });
        Overlays.editOverlay(linePreviewRight, { position: topRight, end: bottomRight, visible: true });

    } else {
        Overlays.editOverlay(linePreviewTop, { visible: false });
        Overlays.editOverlay(linePreviewBottom, { visible: false });
        Overlays.editOverlay(linePreviewLeft, { visible: false });
        Overlays.editOverlay(linePreviewRight, { visible: false });
    }
}

function showPreviewGuides() {
    if (editToolsOn) {
        if (previewAsVoxel) {
            showPreviewVoxel();

            // make sure alternative is hidden
            Overlays.editOverlay(linePreviewTop, { visible: false });
            Overlays.editOverlay(linePreviewBottom, { visible: false });
            Overlays.editOverlay(linePreviewLeft, { visible: false });
            Overlays.editOverlay(linePreviewRight, { visible: false });
        } else {
            showPreviewLines();

            // make sure alternative is hidden
            Overlays.editOverlay(voxelPreview, { visible: false });
        }
    } else {
        // make sure all previews are off
        Overlays.editOverlay(voxelPreview, { visible: false });
        Overlays.editOverlay(linePreviewTop, { visible: false });
        Overlays.editOverlay(linePreviewBottom, { visible: false });
        Overlays.editOverlay(linePreviewLeft, { visible: false });
        Overlays.editOverlay(linePreviewRight, { visible: false });
    }
}

function trackMouseEvent(event) {
    trackLastMouseX = event.x;
    trackLastMouseY = event.y;
    trackAsDelete = event.isControl;
    trackAsRecolor = event.isShifted;
    trackAsEyedropper = event.isMeta;
    trackAsOrbit = event.isAlt;
    showPreviewGuides();
}

function trackKeyPressEvent(event) {
    if (event.text == "CONTROL") {
        trackAsDelete = true;
        moveTools();
    }
    if (event.text == "SHIFT") {
        trackAsRecolor = true;
        moveTools();
    }
    if (event.text == "META") {
        trackAsEyedropper = true;
        moveTools();
    }
    if (event.text == "ALT") {
        trackAsOrbit = true;
        moveTools();
    }
    showPreviewGuides();
}

function trackKeyReleaseEvent(event) {
    if (event.text == "CONTROL") {
        trackAsDelete = false;
        moveTools();
    }
    if (event.text == "SHIFT") {
        trackAsRecolor = false;
        moveTools();
    }
    if (event.text == "META") {
        trackAsEyedropper = false;
        moveTools();
    }
    if (event.text == "ALT") {
        trackAsOrbit = false;
        moveTools();
    }
    
    // on TAB release, toggle our tool state
    if (event.text == "TAB") {
        editToolsOn = !editToolsOn;
        moveTools();
        Audio.playSound(clickSound, audioOptions);
    }

    // on F1 toggle the preview mode between cubes and lines
    if (event.text == "F1") {
        previewAsVoxel = !previewAsVoxel;
    }

    showPreviewGuides();
}

function mousePressEvent(event) {

    // if our tools are off, then don't do anything
    if (!editToolsOn) {
        return; 
    }
    
    var clickedOnSwatch = false;
    var clickedOverlay = Overlays.getOverlayAtPoint({x: event.x, y: event.y});
    
    // if the user clicked on one of the color swatches, update the selectedSwatch
    for (s = 0; s < numColors; s++) {
        if (clickedOverlay == swatches[s]) {
            whichColor = s;
            moveTools();
            clickedOnSwatch = true;
        }
    }
    if (clickedOnSwatch) {
        return; // no further processing
    }
    

    trackMouseEvent(event); // used by preview support
    mouseX = event.x;
    mouseY = event.y;
    var pickRay = Camera.computePickRay(event.x, event.y);
    var intersection = Voxels.findRayIntersection(pickRay);
    audioOptions.position = Vec3.sum(pickRay.origin, pickRay.direction);
    if (intersection.intersects) {
        if (event.isAlt) {
            // start orbit camera! 
            var cameraPosition = Camera.getPosition();
            oldMode = Camera.getMode();
            Camera.setMode("independent");
            isOrbiting = true;
            Camera.keepLookingAt(intersection.intersection);
            // get position for initial azimuth, elevation
            orbitCenter = intersection.intersection; 
            var orbitVector = Vec3.subtract(cameraPosition, orbitCenter);
            orbitRadius = Vec3.length(orbitVector); 
            orbitAzimuth = Math.atan2(orbitVector.z, orbitVector.x);
            orbitAltitude = Math.asin(orbitVector.y / Vec3.length(orbitVector));

        } else if (trackAsDelete || (event.isRightButton && !trackAsEyedropper)) {
            //  Delete voxel
            Voxels.eraseVoxel(intersection.voxel.x, intersection.voxel.y, intersection.voxel.z, intersection.voxel.s);
            Audio.playSound(deleteSound, audioOptions);
            Overlays.editOverlay(voxelPreview, { visible: false });
        } else if (trackAsEyedropper) {
        
            print("Grab color!!!");
            if (whichColor != -1) {
                colors[whichColor].red = intersection.voxel.red;
                colors[whichColor].green = intersection.voxel.green;
                colors[whichColor].blue = intersection.voxel.blue;
                moveTools();
            }
            
        } else if (trackAsRecolor) {
            //  Recolor Voxel
            Voxels.setVoxel(intersection.voxel.x, 
                            intersection.voxel.y, 
                            intersection.voxel.z, 
                            intersection.voxel.s, 
                            colors[whichColor].red, colors[whichColor].green, colors[whichColor].blue);
            Audio.playSound(changeColorSound, audioOptions);
            Overlays.editOverlay(voxelPreview, { visible: false });
        } else {
            //  Add voxel on face
            if (whichColor == -1) {
                //  Copy mode - use clicked voxel color
                var newVoxel = {    
                    x: intersection.voxel.x,
                    y: intersection.voxel.y,
                    z: intersection.voxel.z,
                    s: intersection.voxel.s,
                    red: intersection.voxel.red,
                    green: intersection.voxel.green,
                    blue: intersection.voxel.blue };
            } else {
                var newVoxel = {    
                    x: intersection.voxel.x,
                    y: intersection.voxel.y,
                    z: intersection.voxel.z,    
                    s: intersection.voxel.s,
                    red: colors[whichColor].red,
                    green: colors[whichColor].green,
                    blue: colors[whichColor].blue };
            }
                    
            if (intersection.face == "MIN_X_FACE") {
                newVoxel.x -= newVoxel.s;
            } else if (intersection.face == "MAX_X_FACE") {
                newVoxel.x += newVoxel.s;
            } else if (intersection.face == "MIN_Y_FACE") {
                newVoxel.y -= newVoxel.s;
            } else if (intersection.face == "MAX_Y_FACE") {
                newVoxel.y += newVoxel.s;
            } else if (intersection.face == "MIN_Z_FACE") {
                newVoxel.z -= newVoxel.s;
            } else if (intersection.face == "MAX_Z_FACE") {
                newVoxel.z += newVoxel.s;
            }
                    
            Voxels.setVoxel(newVoxel.x, newVoxel.y, newVoxel.z, newVoxel.s, newVoxel.red, newVoxel.green, newVoxel.blue);
            lastVoxelPosition = { x: newVoxel.x, y: newVoxel.y, z: newVoxel.z };
            lastVoxelColor = { red: newVoxel.red, green: newVoxel.green, blue: newVoxel.blue };
            lastVoxelScale = newVoxel.s;

            Audio.playSound(addSound, audioOptions);
            Overlays.editOverlay(voxelPreview, { visible: false });
            dragStart = { x: event.x, y: event.y };
            isAdding = true;
        }       
    }
}

function keyPressEvent(event) {
    // if our tools are off, then don't do anything
    if (editToolsOn) {
        key_alt = event.isAlt;
        key_shift = event.isShifted;
        var nVal = parseInt(event.text);
        if (event.text == "0") {
            print("Color = Copy");
            whichColor = -1;
            Audio.playSound(clickSound, audioOptions);
            moveTools();
        } else if ((nVal > 0) && (nVal <= numColors)) {
            whichColor = nVal - 1;
            print("Color = " + (whichColor + 1));
            Audio.playSound(clickSound, audioOptions);
            moveTools();
        } else if (event.text == "9") {
            // Create a brand new 1 meter voxel in front of your avatar 
            var color = whichColor; 
            if (color == -1) color = 0;
            var newPosition = getNewVoxelPosition();
            var newVoxel = {    
                        x: newPosition.x,
                        y: newPosition.y ,
                        z: newPosition.z,    
                        s: NEW_VOXEL_SIZE,
                        red: colors[color].red,
                        green: colors[color].green,
                        blue: colors[color].blue };
            Voxels.setVoxel(newVoxel.x, newVoxel.y, newVoxel.z, newVoxel.s, newVoxel.red, newVoxel.green, newVoxel.blue);
            setAudioPosition();
            Audio.playSound(addSound, audioOptions);
        }
    }
    
    // do this even if not in edit tools
    if (event.text == " ") {
        //  Reset my orientation!
        var orientation = { x:0, y:0, z:0, w:1 };
        Camera.setOrientation(orientation);
        MyAvatar.orientation = orientation;
    }
    trackKeyPressEvent(event); // used by preview support
}

function keyReleaseEvent(event) {
    trackKeyReleaseEvent(event); // used by preview support
    key_alt = false;
    key_shift = false; 
}
function mouseMoveEvent(event) {
    if (isOrbiting) {
        var cameraOrientation = Camera.getOrientation();
        var origEulers = Quat.safeEulerAngles(cameraOrientation);
        var newEulers = fixEulerAngles(Quat.safeEulerAngles(cameraOrientation));
        var dx = event.x - mouseX; 
        var dy = event.y - mouseY;
        orbitAzimuth += dx / ORBIT_RATE_AZIMUTH;
        orbitAltitude += dy / ORBIT_RATE_ALTITUDE;
         var orbitVector = { x:(Math.cos(orbitAltitude) * Math.cos(orbitAzimuth)) * orbitRadius, 
                            y:Math.sin(orbitAltitude) * orbitRadius,
                            z:(Math.cos(orbitAltitude) * Math.sin(orbitAzimuth)) * orbitRadius }; 
        orbitPosition = Vec3.sum(orbitCenter, orbitVector);
        Camera.setPosition(orbitPosition);
        mouseX = event.x; 
        mouseY = event.y;
    }
    if (isAdding) {
        //  Watch the drag direction to tell which way to 'extrude' this voxel
        if (!isExtruding) {
            var pickRay = Camera.computePickRay(event.x, event.y);
            var lastVoxelDistance = { x: pickRay.origin.x - lastVoxelPosition.x, 
                                    y: pickRay.origin.y - lastVoxelPosition.y, 
                                    z: pickRay.origin.z - lastVoxelPosition.z };
            var distance = Vec3.length(lastVoxelDistance);
            var mouseSpot = { x: pickRay.direction.x * distance, y: pickRay.direction.y * distance, z: pickRay.direction.z * distance };
            mouseSpot.x += pickRay.origin.x;
            mouseSpot.y += pickRay.origin.y;
            mouseSpot.z += pickRay.origin.z;
            var dx = mouseSpot.x - lastVoxelPosition.x;
            var dy = mouseSpot.y - lastVoxelPosition.y;
            var dz = mouseSpot.z - lastVoxelPosition.z;
            extrudeScale = lastVoxelScale;
            extrudeDirection = { x: 0, y: 0, z: 0 };
            isExtruding = true;
            if (dx > lastVoxelScale) extrudeDirection.x = extrudeScale;
            else if (dx < -lastVoxelScale) extrudeDirection.x = -extrudeScale;
            else if (dy > lastVoxelScale) extrudeDirection.y = extrudeScale;
            else if (dy < -lastVoxelScale) extrudeDirection.y = -extrudeScale;
            else if (dz > lastVoxelScale) extrudeDirection.z = extrudeScale;
            else if (dz < -lastVoxelScale) extrudeDirection.z = -extrudeScale;
            else isExtruding = false; 
        } else {
            //  We have got an extrusion direction, now look for mouse move beyond threshold to add new voxel
            var dx = event.x - mouseX; 
            var dy = event.y - mouseY;
            if (Math.sqrt(dx*dx + dy*dy) > PIXELS_PER_EXTRUDE_VOXEL)  {
                lastVoxelPosition = Vec3.sum(lastVoxelPosition, extrudeDirection);
                Voxels.setVoxel(lastVoxelPosition.x, lastVoxelPosition.y, lastVoxelPosition.z, 
                            extrudeScale, lastVoxelColor.red, lastVoxelColor.green, lastVoxelColor.blue);
                mouseX = event.x;
                mouseY = event.y;
            }
        }
    }
    
    // update the add voxel/delete voxel overlay preview
    trackMouseEvent(event);
}

function mouseReleaseEvent(event) {
    // if our tools are off, then don't do anything
    if (!editToolsOn) {
        return; 
    }

    if (isOrbiting) {
        var cameraOrientation = Camera.getOrientation();
        var eulers = Quat.safeEulerAngles(cameraOrientation);
        MyAvatar.position = Camera.getPosition();
        MyAvatar.orientation = cameraOrientation;
        Camera.stopLooking();
        Camera.setMode(oldMode);
        Camera.setOrientation(cameraOrientation);
    }
    isAdding = false;
    isOrbiting = false; 
    isExtruding = false; 
}

function moveTools() {
    // move the swatches
    swatchesX = (windowDimensions.x - swatchesWidth) / 2;
    swatchesY = windowDimensions.y - swatchHeight;

    // create the overlays, position them in a row, set their colors, and for the selected one, use a different source image
    // location so that it displays the "selected" marker
    for (s = 0; s < numColors; s++) {
        var imageFromX = 12 + (s * 27);
        var imageFromY = 0;
        if (s == whichColor) {
            imageFromY = 55;
        }
        var swatchX = swatchesX + ((swatchWidth - 1) * s);

        Overlays.editOverlay(swatches[s], {
                        x: swatchX,
                        y: swatchesY,
                        subImage: { x: imageFromX, y: imageFromY, width: (swatchWidth - 1), height: swatchHeight },
                        color: colors[s],
                        alpha: 1,
                        visible: editToolsOn
                    });
    }

    // move the tools
    toolsY = (windowDimensions.y - toolsHeight) / 2;
    addToolColor = notSelectedColor;
    deleteToolColor = notSelectedColor;
    recolorToolColor = notSelectedColor;
    eyedropperToolColor = notSelectedColor;
    selectToolColor = notSelectedColor;

    if (trackAsDelete) {
        deleteToolColor = toolSelectedColor;
    } else if (trackAsRecolor) {
        recolorToolColor = toolSelectedColor;
    } else if (trackAsEyedropper) {
        eyedropperToolColor = toolSelectedColor;
    } else if (trackAsOrbit) {
        // nothing gets selected in this case...
    } else {
        addToolColor = toolSelectedColor;
    }

    Overlays.editOverlay(addTool, {
                    x: 0, y: toolsY + (toolHeight * addToolAt), width: toolWidth, height: toolHeight,
                    color: addToolColor,
                    visible: editToolsOn
                });

    Overlays.editOverlay(deleteTool, {
                    x: 0, y: toolsY + (toolHeight * deleteToolAt), width: toolWidth, height: toolHeight,
                    color: deleteToolColor,
                    visible: editToolsOn
                });

    Overlays.editOverlay(recolorTool, {
                    x: 0, y: toolsY + (toolHeight * recolorToolAt), width: toolWidth, height: toolHeight,
                    color: recolorToolColor,
                    visible: editToolsOn
                });

    Overlays.editOverlay(eyedropperTool, {
                    x: 0, y: toolsY + (toolHeight * eyedropperToolAt), width: toolWidth, height: toolHeight,
                    color: eyedropperToolColor,
                    visible: editToolsOn
                });

    Overlays.editOverlay(selectTool, {
                    x: 0, y: toolsY + (toolHeight * selectToolAt), width: toolWidth, height: toolHeight,
                    color: selectToolColor,
                    visible: editToolsOn
                });

}


function update() {
    var newWindowDimensions = Controller.getViewportDimensions();
    if (newWindowDimensions.x != windowDimensions.x || newWindowDimensions.y != windowDimensions.y) {
        windowDimensions = newWindowDimensions;
        print("window resized...");
        moveTools();
    }
}

Controller.mousePressEvent.connect(mousePressEvent);
Controller.mouseReleaseEvent.connect(mouseReleaseEvent);
Controller.mouseMoveEvent.connect(mouseMoveEvent);
Controller.keyPressEvent.connect(keyPressEvent);
Controller.keyReleaseEvent.connect(keyReleaseEvent);

function scriptEnding() {
    Overlays.deleteOverlay(voxelPreview);
    Overlays.deleteOverlay(linePreviewTop);
    Overlays.deleteOverlay(linePreviewBottom);
    Overlays.deleteOverlay(linePreviewLeft);
    Overlays.deleteOverlay(linePreviewRight);
    for (s = 0; s < numColors; s++) {
        Overlays.deleteOverlay(swatches[s]);
    }
    Overlays.deleteOverlay(addTool);
    Overlays.deleteOverlay(deleteTool);
    Overlays.deleteOverlay(recolorTool);
    Overlays.deleteOverlay(eyedropperTool);
    Overlays.deleteOverlay(selectTool);
}
Script.scriptEnding.connect(scriptEnding);


Script.willSendVisualDataCallback.connect(update);



