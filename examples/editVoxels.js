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
var PIXELS_PER_EXTRUDE_VOXEL = 16;
var WHEEL_PIXELS_PER_SCALE_CHANGE = 100;
var MAX_VOXEL_SCALE = 1.0;
var MIN_VOXEL_SCALE = 1.0 / Math.pow(2.0, 8.0);
var WHITE_COLOR = { red: 255, green: 255, blue: 255 };

var MAX_PASTE_VOXEL_SCALE = 256;
var MIN_PASTE_VOXEL_SCALE = .256;

var zFightingSizeAdjust = 0.002; // used to adjust preview voxels to prevent z fighting
var previewLineWidth = 1.5;

var inspectJsIsRunning = false;
var isAdding = false; 
var isExtruding = false;
var extrudeDirection = { x: 0, y: 0, z: 0 };
var extrudeScale = 0.0;
var lastVoxelPosition = { x: 0, y: 0, z: 0 };
var lastVoxelColor = { red: 0, green: 0, blue: 0 };
var lastVoxelScale = 0;
var dragStart = { x: 0, y: 0 };
var wheelPixelsMoved = 0;


var mouseX = 0;
var mouseY = 0; 

//  Create a table of the different colors you can choose
var colors = new Array();
colors[0] = { red: 120, green: 181, blue: 126 };
colors[1] = { red: 75,  green: 155, blue: 103 };
colors[2] = { red: 56,  green: 132, blue: 86 };
colors[3] = { red: 83,  green: 211, blue: 83 };
colors[4] = { red: 236, green: 174,  blue: 0 };
colors[5] = { red: 234, green: 133,  blue: 0 };
colors[6] = { red: 211, green: 115,  blue: 0 };
colors[7] = { red: 48,  green: 116,  blue: 119 };
colors[8] = { red: 31,  green: 64,  blue: 64 };
var numColors = 9;
var whichColor = -1;            //  Starting color is 'Copy' mode

//  Create sounds for adding, deleting, recoloring voxels 
var addSound1 = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Voxels/voxel+create+2.raw");
var addSound2 = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Voxels/voxel+create+4.raw");

var addSound3 = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Voxels/voxel+create+3.raw");
var deleteSound = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Voxels/voxel+delete+2.raw");
var changeColorSound = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Voxels/voxel+edit+2.raw");
var clickSound = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Switches+and+sliders/toggle+switch+-+medium.raw");
var audioOptions = new AudioInjectionOptions();

audioOptions.volume = 0.5;
audioOptions.position = Vec3.sum(MyAvatar.position, { x: 0, y: 1, z: 0 }  ); // start with audio slightly above the avatar

var editToolsOn = true; // starts out off

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
                    lineWidth: previewLineWidth
                });

var linePreviewBottom = Overlays.addOverlay("line3d", {
                    position: { x: 0, y: 0, z: 0},
                    end: { x: 0, y: 0, z: 0},
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false,
                    lineWidth: previewLineWidth
                });

var linePreviewLeft = Overlays.addOverlay("line3d", {
                    position: { x: 0, y: 0, z: 0},
                    end: { x: 0, y: 0, z: 0},
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false,
                    lineWidth: previewLineWidth
                });

var linePreviewRight = Overlays.addOverlay("line3d", {
                    position: { x: 0, y: 0, z: 0},
                    end: { x: 0, y: 0, z: 0},
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false,
                    lineWidth: previewLineWidth
                });


// these will be used below
var sliderWidth = 154;
var sliderHeight = 37;

// These will be our "overlay IDs"
var swatches = new Array();
var swatchExtraPadding = 5;
var swatchHeight = 37;
var swatchWidth = 27;
var swatchesWidth = swatchWidth * numColors + numColors + swatchExtraPadding * 2;
var swatchesX = (windowDimensions.x - (swatchesWidth + sliderWidth)) / 2;
var swatchesY = windowDimensions.y - swatchHeight + 1;

var toolIconUrl = "http://highfidelity-public.s3-us-west-1.amazonaws.com/images/tools/";

// create the overlays, position them in a row, set their colors, and for the selected one, use a different source image
// location so that it displays the "selected" marker
for (s = 0; s < numColors; s++) {

    var extraWidth = 0;

    if (s == 0) {
        extraWidth = swatchExtraPadding;
    }

    var imageFromX = swatchExtraPadding - extraWidth + s * swatchWidth;
    var imageFromY = swatchHeight + 1;

    var swatchX = swatchExtraPadding - extraWidth + swatchesX + ((swatchWidth - 1) * s);

    if (s == (numColors - 1)) {
        extraWidth = swatchExtraPadding;
    }

    swatches[s] = Overlays.addOverlay("image", {
                    x: swatchX,
                    y: swatchesY,
                    width: swatchWidth + extraWidth,
                    height: swatchHeight,
                    subImage: { x: imageFromX, y: imageFromY, width: swatchWidth + extraWidth, height: swatchHeight },
                    imageURL: toolIconUrl + "swatches.svg",
                    color: colors[s],
                    alpha: 1,
                    visible: editToolsOn
                });
}


// These will be our tool palette overlays
var numberOfTools = 3;
var toolHeight = 50;
var toolWidth = 50;
var toolVerticalSpacing = 4;
var toolsHeight = toolHeight * numberOfTools + toolVerticalSpacing * (numberOfTools - 1);
var toolsX = 8;
var toolsY = (windowDimensions.y - toolsHeight) / 2;

var voxelToolAt = 0;
var recolorToolAt = 1;
var eyedropperToolAt = 2;

var pasteModeColor = { red: 132,  green: 61,  blue: 255 };

var voxelTool = Overlays.addOverlay("image", {
                    x: 0, y: 0, width: toolWidth, height: toolHeight,
                    subImage: { x: 0, y: toolHeight, width: toolWidth, height: toolHeight },
                    imageURL: toolIconUrl + "voxel-tool.svg",
                    visible: false,
                    alpha: 0.9
                });

var recolorTool = Overlays.addOverlay("image", {
                    x: 0, y: 0, width: toolWidth, height: toolHeight,
                    subImage: { x: 0, y: toolHeight, width: toolWidth, height: toolHeight },
                    imageURL: toolIconUrl + "paint-tool.svg",
                    visible: false,
                    alpha: 0.9
                });

var eyedropperTool = Overlays.addOverlay("image", {
                    x: 0, y: 0, width: toolWidth, height: toolHeight,
                    subImage: { x: 0, y: toolHeight, width: toolWidth, height: toolHeight },
                    imageURL: toolIconUrl + "eyedropper-tool.svg",
                    visible: false,
                    alpha: 0.9
                });
                
// This will create a couple of image overlays that make a "slider", we will demonstrate how to trap mouse messages to
// move the slider

// see above...
//var sliderWidth = 158;
//var sliderHeight = 35;

var sliderOffsetX = 17;
var sliderX = swatchesX - swatchWidth - sliderOffsetX;
var sliderY = windowDimensions.y - sliderHeight + 1;
var slider = Overlays.addOverlay("image", {
                    // alternate form of expressing bounds
                    bounds: { x: sliderX, y: sliderY, width: sliderWidth, height: sliderHeight},
                    imageURL: toolIconUrl + "voxel-size-slider-bg.svg",
                    alpha: 1,
                    visible: false
                });

// The slider is handled in the mouse event callbacks.
var isMovingSlider = false;
var thumbClickOffsetX = 0;

// This is the thumb of our slider
var minThumbX = 20; // relative to the x of the slider
var maxThumbX = minThumbX + 90;
var thumbExtents = maxThumbX - minThumbX;
var thumbX = (minThumbX + maxThumbX) / 2;
var thumbOffsetY = 11;
var thumbY = sliderY + thumbOffsetY;
var thumb = Overlays.addOverlay("image", {
                    x: sliderX + thumbX,
                    y: thumbY,
                    width: 17,
                    height: 17,
                    imageURL: toolIconUrl + "voxel-size-slider-handle.svg",
                    alpha: 1,
                    visible: false
                });

var pointerVoxelScale = Math.floor(MAX_VOXEL_SCALE + MIN_VOXEL_SCALE) / 2; // this is the voxel scale used for click to add or delete
var pointerVoxelScaleSet = false; // if voxel scale has not yet been set, we use the intersection size

var pointerVoxelScaleSteps = 8; // the number of slider position steps
var pointerVoxelScaleOriginStep = 8; // the position of slider for the 1 meter size voxel
var pointerVoxelScaleMin = Math.pow(2, (1-pointerVoxelScaleOriginStep));
var pointerVoxelScaleMax = Math.pow(2, (pointerVoxelScaleSteps-pointerVoxelScaleOriginStep));
var thumbDeltaPerStep = thumbExtents / (pointerVoxelScaleSteps - 1);



///////////////////////////////////// IMPORT MODULE ///////////////////////////////
// Move the following code to a separate file when include will be available.
var importTree;
var importPreview;
var importBoundaries;
var isImporting;
var importPosition;
var importScale;

function initImport() {
    importPreview = Overlays.addOverlay("localvoxels", {
                                        name: "import",
                                        position: { x: 0, y: 0, z: 0},
                                        scale: 1,
                                        visible: false
                                        });
    importBoundaries = Overlays.addOverlay("cube", {
                                           position: { x: 0, y: 0, z: 0 },
                                           scale: 1,
                                           color: { red: 128, blue: 128, green: 128 },
                                           solid: false,
                                           visible: false
                                           })
    isImporting = false;
    importPosition = { x: 0, y: 0, z: 0 };
    importScale = 0;
}

function importVoxels() {
    if (Clipboard.importVoxels()) {
        isImporting = true;
        if (importScale <= 0) {
            importScale = 1;
        }
    } else {
        isImporting = false;
    }

    return isImporting;
}

function moveImport(position) {
    if (0 < position.x && 0 < position.y && 0 < position.z) {
        importPosition = position;
        Overlays.editOverlay(importPreview, {
                             position: { x: importPosition.x, y: importPosition.y, z: importPosition.z }
                             });
        Overlays.editOverlay(importBoundaries, {
                             position: { x: importPosition.x, y: importPosition.y, z: importPosition.z }
                             });
    }
}

function rescaleImport(scale) {
    if (0 < scale) {
        importScale = scale;
        Overlays.editOverlay(importPreview, {
                             scale: importScale
                             });
        Overlays.editOverlay(importBoundaries, {
                             scale: importScale
                             });
    }
}

function showImport(doShow) {
    Overlays.editOverlay(importPreview, {
                         visible: doShow
                         });
    Overlays.editOverlay(importBoundaries, {
                         visible: doShow
                         });
}

function placeImport() {
    if (isImporting) {
        Clipboard.pasteVoxel(importPosition.x, importPosition.y, importPosition.z, importScale);
        isImporting = false;
    }
}

function cancelImport() {
    if (isImporting) {
        isImporting = false;
        showImport(false);
    }
}

function cleanupImport() {
    Overlays.deleteOverlay(importPreview);
    Overlays.deleteOverlay(importBoundaries);
    isImporting = false;
    importPostion = { x: 0, y: 0, z: 0 };
    importScale = 0;
}
/////////////////////////////////// END IMPORT MODULE /////////////////////////////
initImport();

if (editToolsOn) {
    moveTools();
}


function calcThumbFromScale(scale) {
    var scaleLog = Math.log(scale)/Math.log(2);
    var thumbStep = scaleLog + pointerVoxelScaleOriginStep;
    if (thumbStep < 1) {
        thumbStep = 1;
    }
    if (thumbStep > pointerVoxelScaleSteps) {
        thumbStep = pointerVoxelScaleSteps;
    }
    thumbX = (thumbDeltaPerStep * (thumbStep - 1)) + minThumbX;
    Overlays.editOverlay(thumb, { x: thumbX + sliderX } );
}

function calcScaleFromThumb(newThumbX) {
    // newThumbX is the pixel location relative to start of slider,
    // we need to figure out the actual offset in the allowed slider area
    thumbAt = newThumbX - minThumbX;
    thumbStep = Math.floor((thumbAt/ thumbExtents) * (pointerVoxelScaleSteps-1)) + 1;
    pointerVoxelScale = Math.pow(2, (thumbStep-pointerVoxelScaleOriginStep));

    // if importing, rescale import ...
    if (isImporting) {
        var importScale = (pointerVoxelScale / MAX_VOXEL_SCALE) * MAX_PASTE_VOXEL_SCALE;
        rescaleImport(importScale);
    }

    // now reset the display accordingly...
    calcThumbFromScale(pointerVoxelScale);
    
    // if the user moved the thumb, then they are fixing the voxel scale
    pointerVoxelScaleSet = true;
}

function setAudioPosition() {
    var position = MyAvatar.position;
    var forwardVector = Quat.getFront(MyAvatar.orientation);
    audioOptions.position = Vec3.sum(position, forwardVector);
}

function getNewPasteVoxel(pickRay) {

    var voxelSize = MIN_PASTE_VOXEL_SCALE + (MAX_PASTE_VOXEL_SCALE - MIN_PASTE_VOXEL_SCALE) * pointerVoxelScale - 1;
    var origin = { x: pickRay.direction.x, y: pickRay.direction.y, z: pickRay.direction.z };

    origin.x += pickRay.origin.x;
    origin.y += pickRay.origin.y;
    origin.z += pickRay.origin.z;

    origin.x -= voxelSize / 2;
    origin.y -= voxelSize / 2;
    origin.z += voxelSize / 2;

    return {origin: origin, voxelSize: voxelSize};
}

function getNewVoxelPosition() {
    var camera = Camera.getPosition();
    var forwardVector = Quat.getFront(MyAvatar.orientation);
    var newPosition = Vec3.sum(camera, Vec3.multiply(forwardVector, NEW_VOXEL_DISTANCE_FROM_CAMERA));
    return newPosition;
}

var trackLastMouseX = 0;
var trackLastMouseY = 0;
var trackAsDelete = false;
var trackAsRecolor = false;
var trackAsEyedropper = false;

var voxelToolSelected = true;
var recolorToolSelected = false;
var eyedropperToolSelected = false;
var pasteMode = false;

function playRandomAddSound(audioOptions) {
    if (Math.random() < 0.33) {
        Audio.playSound(addSound1, audioOptions);
    } else if (Math.random() < 0.5) {
        Audio.playSound(addSound2, audioOptions);
    } else {
        Audio.playSound(addSound3, audioOptions);
    }
}

function calculateVoxelFromIntersection(intersection, operation) {
    //print("calculateVoxelFromIntersection() operation="+operation);
    var resultVoxel;

    var wantDebug = false;
    if (wantDebug) {
        print(">>>>> calculateVoxelFromIntersection().... intersection voxel.red/green/blue=" + intersection.voxel.red + ", " 
                                + intersection.voxel.green + ", " + intersection.voxel.blue);
        print("   intersection voxel.x/y/z/s=" + intersection.voxel.x + ", " 
                                + intersection.voxel.y + ", " + intersection.voxel.z+ ": " + intersection.voxel.s);
        print("   intersection face=" + intersection.face);
        print("   intersection distance=" + intersection.distance);
        print("   intersection intersection.x/y/z=" + intersection.intersection.x + ", " 
                                + intersection.intersection.y + ", " + intersection.intersection.z);
    }
    
    var voxelSize;
    if (pointerVoxelScaleSet) {
        voxelSize = pointerVoxelScale; 
    } else {
        voxelSize = intersection.voxel.s; 
    }

    var x;
    var y;
    var z;
    
    // if our "target voxel size" is larger than the voxel we intersected with, then we need to find the closest
    // ancestor voxel of our target size that contains our intersected voxel.
    if (voxelSize > intersection.voxel.s) {
        if (wantDebug) {
            print("voxelSize > intersection.voxel.s.... choose the larger voxel that encompasses the one selected");
        }
        x = Math.floor(intersection.voxel.x / voxelSize) * voxelSize;
        y = Math.floor(intersection.voxel.y / voxelSize) * voxelSize;
        z = Math.floor(intersection.voxel.z / voxelSize) * voxelSize;
    } else {
        // otherwise, calculate the enclosed voxel of size voxelSize that the intersection point falls inside of.
        // if you have a voxelSize that's smaller than the voxel you're intersecting, this calculation will result
        // in the subvoxel that the intersection point falls in, if the target voxelSize matches the intersecting
        // voxel this still works and results in returning the intersecting voxel which is what we want
        var adjustToCenter = Vec3.multiply(Voxels.getFaceVector(intersection.face), (voxelSize * -0.5));
        if (wantDebug) {
            print("adjustToCenter=" + adjustToCenter.x + "," + adjustToCenter.y + "," + adjustToCenter.z);
        }
        var centerOfIntersectingVoxel = Vec3.sum(intersection.intersection, adjustToCenter);
        x = Math.floor(centerOfIntersectingVoxel.x / voxelSize) * voxelSize;
        y = Math.floor(centerOfIntersectingVoxel.y / voxelSize) * voxelSize;
        z = Math.floor(centerOfIntersectingVoxel.z / voxelSize) * voxelSize;
    }
    resultVoxel = { x: x, y: y, z: z, s: voxelSize };
    highlightAt = { x: x, y: y, z: z, s: voxelSize };

    // we only do the "add to the face we're pointing at" adjustment, if the operation is an add
    // operation, and the target voxel size is equal to or smaller than the intersecting voxel.
    var wantAddAdjust = (operation == "add" && (voxelSize <= intersection.voxel.s));
    if (wantDebug) {
        print("wantAddAdjust="+wantAddAdjust);
    }

    // now we also want to calculate the "edge square" for the face for this voxel
    if (intersection.face == "MIN_X_FACE") {

        highlightAt.x = x - zFightingSizeAdjust;
        if (wantAddAdjust) {
            resultVoxel.x -= voxelSize;
        }
        
        resultVoxel.bottomLeft = {x: highlightAt.x, y: highlightAt.y + zFightingSizeAdjust, z: highlightAt.z + zFightingSizeAdjust };
        resultVoxel.bottomRight = {x: highlightAt.x, y: highlightAt.y + zFightingSizeAdjust, z: highlightAt.z + voxelSize - zFightingSizeAdjust };
        resultVoxel.topLeft = {x: highlightAt.x, y: highlightAt.y + voxelSize - zFightingSizeAdjust, z: highlightAt.z + zFightingSizeAdjust };
        resultVoxel.topRight = {x: highlightAt.x, y: highlightAt.y + voxelSize - zFightingSizeAdjust, z: highlightAt.z + voxelSize - zFightingSizeAdjust };

    } else if (intersection.face == "MAX_X_FACE") {

        highlightAt.x = x + voxelSize + zFightingSizeAdjust;
        if (wantAddAdjust) {
            resultVoxel.x += resultVoxel.s;
        }

        resultVoxel.bottomRight = {x: highlightAt.x, y: highlightAt.y + zFightingSizeAdjust, z: highlightAt.z + zFightingSizeAdjust };
        resultVoxel.bottomLeft = {x: highlightAt.x, y: highlightAt.y + zFightingSizeAdjust, z: highlightAt.z + voxelSize - zFightingSizeAdjust };
        resultVoxel.topRight = {x: highlightAt.x, y: highlightAt.y + voxelSize - zFightingSizeAdjust, z: highlightAt.z + zFightingSizeAdjust };
        resultVoxel.topLeft = {x: highlightAt.x, y: highlightAt.y + voxelSize - zFightingSizeAdjust, z: highlightAt.z + voxelSize - zFightingSizeAdjust };

    } else if (intersection.face == "MIN_Y_FACE") {

        highlightAt.y = y - zFightingSizeAdjust;
        if (wantAddAdjust) {
            resultVoxel.y -= voxelSize;
        }
        
        resultVoxel.topRight = {x: highlightAt.x + zFightingSizeAdjust , y: highlightAt.y, z: highlightAt.z + zFightingSizeAdjust  };
        resultVoxel.topLeft = {x: highlightAt.x + voxelSize - zFightingSizeAdjust, y: highlightAt.y, z: highlightAt.z + zFightingSizeAdjust };
        resultVoxel.bottomRight = {x: highlightAt.x + zFightingSizeAdjust , y: highlightAt.y, z: highlightAt.z  + voxelSize - zFightingSizeAdjust };
        resultVoxel.bottomLeft = {x: highlightAt.x + voxelSize - zFightingSizeAdjust , y: highlightAt.y, z: highlightAt.z + voxelSize - zFightingSizeAdjust };

    } else if (intersection.face == "MAX_Y_FACE") {

        highlightAt.y = y + voxelSize + zFightingSizeAdjust;
        if (wantAddAdjust) {
            resultVoxel.y += voxelSize;
        }
        
        resultVoxel.bottomRight = {x: highlightAt.x + zFightingSizeAdjust, y: highlightAt.y, z: highlightAt.z + zFightingSizeAdjust };
        resultVoxel.bottomLeft = {x: highlightAt.x + voxelSize - zFightingSizeAdjust, y: highlightAt.y, z: highlightAt.z + zFightingSizeAdjust};
        resultVoxel.topRight = {x: highlightAt.x + zFightingSizeAdjust, y: highlightAt.y, z: highlightAt.z  + voxelSize - zFightingSizeAdjust};
        resultVoxel.topLeft = {x: highlightAt.x + voxelSize - zFightingSizeAdjust, y: highlightAt.y, z: highlightAt.z + voxelSize - zFightingSizeAdjust};

    } else if (intersection.face == "MIN_Z_FACE") {

        highlightAt.z = z - zFightingSizeAdjust;
        if (wantAddAdjust) {
            resultVoxel.z -= voxelSize;
        }
        
        resultVoxel.bottomRight = {x: highlightAt.x + zFightingSizeAdjust, y: highlightAt.y + zFightingSizeAdjust, z: highlightAt.z };
        resultVoxel.bottomLeft = {x: highlightAt.x + voxelSize - zFightingSizeAdjust, y: highlightAt.y + zFightingSizeAdjust, z: highlightAt.z};
        resultVoxel.topRight = {x: highlightAt.x + zFightingSizeAdjust, y: highlightAt.y + voxelSize - zFightingSizeAdjust, z: highlightAt.z };
        resultVoxel.topLeft = {x: highlightAt.x + voxelSize - zFightingSizeAdjust, y: highlightAt.y + voxelSize - zFightingSizeAdjust, z: highlightAt.z};

    } else if (intersection.face == "MAX_Z_FACE") {

        highlightAt.z = z + voxelSize + zFightingSizeAdjust;
        if (wantAddAdjust) {
            resultVoxel.z += voxelSize;
        }

        resultVoxel.bottomLeft = {x: highlightAt.x + zFightingSizeAdjust, y: highlightAt.y + zFightingSizeAdjust, z: highlightAt.z };
        resultVoxel.bottomRight = {x: highlightAt.x + voxelSize - zFightingSizeAdjust, y: highlightAt.y + zFightingSizeAdjust, z: highlightAt.z};
        resultVoxel.topLeft = {x: highlightAt.x + zFightingSizeAdjust, y: highlightAt.y + voxelSize - zFightingSizeAdjust, z: highlightAt.z };
        resultVoxel.topRight = {x: highlightAt.x + voxelSize - zFightingSizeAdjust, y: highlightAt.y + voxelSize - zFightingSizeAdjust, z: highlightAt.z};

    }
    
    return resultVoxel;
}

function showPreviewVoxel() {
    var voxelColor;

    var pickRay = Camera.computePickRay(trackLastMouseX, trackLastMouseY);
    var intersection = Voxels.findRayIntersection(pickRay);

    // if the user hasn't updated the 
    if (!pointerVoxelScaleSet) {
        calcThumbFromScale(intersection.voxel.s);
    }

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
    if (trackAsRecolor || recolorToolSelected || trackAsEyedropper || eyedropperToolSelected) {
        Overlays.editOverlay(voxelPreview, { visible: true });
    } else if (voxelToolSelected && !isExtruding) {
        Overlays.editOverlay(voxelPreview, { visible: true });
    } else if (isExtruding) {
        Overlays.editOverlay(voxelPreview, { visible: false });
    }
}

function showPreviewLines() {

    var pickRay = Camera.computePickRay(trackLastMouseX, trackLastMouseY);

    if (pasteMode) { // free voxel pasting

        Overlays.editOverlay(voxelPreview, { visible: false });
        Overlays.editOverlay(linePreviewLeft, { visible: false });

        var pasteVoxel = getNewPasteVoxel(pickRay);

        // X axis
        Overlays.editOverlay(linePreviewBottom, {
            position: pasteVoxel.origin,
            end: {x: pasteVoxel.origin.x + pasteVoxel.voxelSize, y: pasteVoxel.origin.y, z: pasteVoxel.origin.z },
            visible: true
        });

        // Y axis
        Overlays.editOverlay(linePreviewRight, {
            position: pasteVoxel.origin,
            end: {x: pasteVoxel.origin.x, y: pasteVoxel.origin.y + pasteVoxel.voxelSize, z: pasteVoxel.origin.z },
            visible: true
        });

        // Z axis
        Overlays.editOverlay(linePreviewTop, {
            position: pasteVoxel.origin,
            end: {x: pasteVoxel.origin.x, y: pasteVoxel.origin.y, z: pasteVoxel.origin.z - pasteVoxel.voxelSize },
            visible: true
        });

        return;
    }

    var intersection = Voxels.findRayIntersection(pickRay);
    
    if (intersection.intersects) {

        // if the user hasn't updated the 
        if (!pointerVoxelScaleSet) {
            calcThumbFromScale(intersection.voxel.s);
        }

        resultVoxel = calculateVoxelFromIntersection(intersection,"");
        Overlays.editOverlay(voxelPreview, { visible: false });
        Overlays.editOverlay(linePreviewTop, { position: resultVoxel.topLeft, end: resultVoxel.topRight, visible: true });
        Overlays.editOverlay(linePreviewBottom, { position: resultVoxel.bottomLeft, end: resultVoxel.bottomRight, visible: true });
        Overlays.editOverlay(linePreviewLeft, { position: resultVoxel.topLeft, end: resultVoxel.bottomLeft, visible: true });
        Overlays.editOverlay(linePreviewRight, { position: resultVoxel.topRight, end: resultVoxel.bottomRight, visible: true });
    } else {
        Overlays.editOverlay(voxelPreview, { visible: false });
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
    showPreviewGuides();
}

function trackKeyPressEvent(event) {
    if (!editToolsOn) {
        return;
    }

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
        inspectJsIsRunning = true;
    }
    showPreviewGuides();
}

function trackKeyReleaseEvent(event) {
    // on TAB release, toggle our tool state
    if (event.text == "TAB") {
        editToolsOn = !editToolsOn;
        moveTools();
        setAudioPosition(); // make sure we set the audio position before playing sounds
        showPreviewGuides();
        Audio.playSound(clickSound, audioOptions);
    }
                          
    if (event.text == "ALT") {
        inspectJsIsRunning = false;
    }

    if (editToolsOn) {
        if (event.text == "ESC") {
            pointerVoxelScaleSet = false;
            pasteMode = false;
            moveTools();
        }
        if (event.text == "-") {
            thumbX -= thumbDeltaPerStep;
            calcScaleFromThumb(thumbX);
        }
        if (event.text == "+") {
            thumbX += thumbDeltaPerStep;
            calcScaleFromThumb(thumbX);
        }
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
        
        // on F1 toggle the preview mode between cubes and lines
        if (event.text == "F1") {
            previewAsVoxel = !previewAsVoxel;
        }

        showPreviewGuides();
    }    
}

function mousePressEvent(event) {
    // if our tools are off, then don't do anything
    if (!editToolsOn) {
        return; 
    }
    if (inspectJsIsRunning) {
        return;
    }
    
    var clickedOnSomething = false;
    var clickedOverlay = Overlays.getOverlayAtPoint({x: event.x, y: event.y});
    
    // If the user clicked on the thumb, handle the slider logic
    if (clickedOverlay == thumb) {
        isMovingSlider = true;
        thumbClickOffsetX = event.x - (sliderX + thumbX); // this should be the position of the mouse relative to the thumb
        clickedOnSomething = true;
        
        Overlays.editOverlay(thumb, { imageURL: toolIconUrl + "voxel-size-slider-handle.svg", });
        
    } else if (clickedOverlay == voxelTool) {
        voxelToolSelected = true;
        recolorToolSelected = false;
        eyedropperToolSelected = false;
        moveTools();
        clickedOnSomething = true;
    } else if (clickedOverlay == recolorTool) {
        voxelToolSelected = false;
        recolorToolSelected = true;
        eyedropperToolSelected = false;
        moveTools();
        clickedOnSomething = true;
    } else if (clickedOverlay == eyedropperTool) {
        voxelToolSelected = false;
        recolorToolSelected = false;
        eyedropperToolSelected = true;
        moveTools();
        clickedOnSomething = true;
    } else if (clickedOverlay == slider) {
        
        if (event.x < sliderX + minThumbX) {
            thumbX -= thumbDeltaPerStep;
            calcScaleFromThumb(thumbX);
        }
        
        if (event.x > sliderX + maxThumbX) {
            thumbX += thumbDeltaPerStep;
            calcScaleFromThumb(thumbX);
        }
        
        moveTools();
        clickedOnSomething = true;
    } else {
        // if the user clicked on one of the color swatches, update the selectedSwatch
        for (s = 0; s < numColors; s++) {
            if (clickedOverlay == swatches[s]) {
                whichColor = s;
                moveTools();
                clickedOnSomething = true;
                break;
            }
        }
    }
    if (clickedOnSomething) {
        return; // no further processing
    }

    // TODO: does any of this stuff need to execute if we're panning or orbiting?
    trackMouseEvent(event); // used by preview support
    mouseX = event.x;
    mouseY = event.y;
    var pickRay = Camera.computePickRay(event.x, event.y);
    var intersection = Voxels.findRayIntersection(pickRay);
    audioOptions.position = Vec3.sum(pickRay.origin, pickRay.direction);

    if (isImporting) {
        print("placing import...");
        placeImport();
        showImport(false);
        moveTools();
        return;
    }

    if (pasteMode) {
        var pasteVoxel = getNewPasteVoxel(pickRay);
        Clipboard.pasteVoxel(pasteVoxel.origin.x, pasteVoxel.origin.y, pasteVoxel.origin.z, pasteVoxel.voxelSize);
        pasteMode = false;
        moveTools();
        return;
    }

    if (intersection.intersects) {
        // if the user hasn't updated the 
        if (!pointerVoxelScaleSet) {
            calcThumbFromScale(intersection.voxel.s);
        }
        
        if (trackAsDelete || event.isRightButton && !trackAsEyedropper) {
            //  Delete voxel
            voxelDetails = calculateVoxelFromIntersection(intersection,"delete");
            Voxels.eraseVoxel(voxelDetails.x, voxelDetails.y, voxelDetails.z, voxelDetails.s);
            Audio.playSound(deleteSound, audioOptions);
            Overlays.editOverlay(voxelPreview, { visible: false });
        } else if (eyedropperToolSelected || trackAsEyedropper) {
            if (whichColor != -1) {
                colors[whichColor].red = intersection.voxel.red;
                colors[whichColor].green = intersection.voxel.green;
                colors[whichColor].blue = intersection.voxel.blue;
                moveTools();
            }
            
        } else if (recolorToolSelected || trackAsRecolor) {
            //  Recolor Voxel
            voxelDetails = calculateVoxelFromIntersection(intersection,"recolor");

            // doing this erase then set will make sure we only recolor just the target voxel
            Voxels.eraseVoxel(voxelDetails.x, voxelDetails.y, voxelDetails.z, voxelDetails.s);
            Voxels.setVoxel(voxelDetails.x, voxelDetails.y, voxelDetails.z, voxelDetails.s, 
                            colors[whichColor].red, colors[whichColor].green, colors[whichColor].blue);
            Audio.playSound(changeColorSound, audioOptions);
            Overlays.editOverlay(voxelPreview, { visible: false });
        } else if (voxelToolSelected) {
            //  Add voxel on face
            if (whichColor == -1) {
                //  Copy mode - use clicked voxel color
                newColor = {    
                    red: intersection.voxel.red,
                    green: intersection.voxel.green,
                    blue: intersection.voxel.blue };
            } else {
                newColor = {    
                    red: colors[whichColor].red,
                    green: colors[whichColor].green,
                    blue: colors[whichColor].blue };
            }
                    
            voxelDetails = calculateVoxelFromIntersection(intersection,"add");
            Voxels.setVoxel(voxelDetails.x, voxelDetails.y, voxelDetails.z, voxelDetails.s,
                newColor.red, newColor.green, newColor.blue);
            lastVoxelPosition = { x: voxelDetails.x, y: voxelDetails.y, z: voxelDetails.z };
            lastVoxelColor = { red: newColor.red, green: newColor.green, blue: newColor.blue };
            lastVoxelScale = voxelDetails.s;

            playRandomAddSound(audioOptions);
            
            Overlays.editOverlay(voxelPreview, { visible: false });
            dragStart = { x: event.x, y: event.y };
            isAdding = true;
        } 
    }
}

function keyPressEvent(event) {
    // if our tools are off, then don't do anything
    if (editToolsOn) {
        var nVal = parseInt(event.text);
        if (event.text == "`") {
            print("Color = Copy");
            whichColor = -1;
            Audio.playSound(clickSound, audioOptions);
            moveTools();
        } else if ((nVal > 0) && (nVal <= numColors)) {
            whichColor = nVal - 1;
            print("Color = " + (whichColor + 1));
            Audio.playSound(clickSound, audioOptions);
            moveTools();
        } else if (event.text == "0") {
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
            Voxels.eraseVoxel(newVoxel.x, newVoxel.y, newVoxel.z, newVoxel.s);
            Voxels.setVoxel(newVoxel.x, newVoxel.y, newVoxel.z, newVoxel.s, newVoxel.red, newVoxel.green, newVoxel.blue);
            setAudioPosition();
            playRandomAddSound(audioOptions);
        }
    }
    
    trackKeyPressEvent(event); // used by preview support
}

function keyReleaseEvent(event) {
    trackKeyReleaseEvent(event); // used by preview support
}

function setupMenus() {
    // hook up menus
    Menu.menuItemEvent.connect(menuItemEvent);

    // add our menuitems
    Menu.addMenuItem({ menuName: "Edit", menuItemName: "Voxels", isSeparator: true, beforeItem: "Physics" });
    Menu.addMenuItem({ menuName: "Edit", menuItemName: "Cut", shortcutKey: "CTRL+X", afterItem: "Voxels" });
    Menu.addMenuItem({ menuName: "Edit", menuItemName: "Copy", shortcutKey: "CTRL+C", afterItem: "Cut" });
    Menu.addMenuItem({ menuName: "Edit", menuItemName: "Paste", shortcutKey: "CTRL+V", afterItem: "Copy" });
    Menu.addMenuItem({ menuName: "Edit", menuItemName: "Nudge", shortcutKey: "CTRL+N", afterItem: "Paste" });
    Menu.addMenuItem({ menuName: "Edit", menuItemName: "Delete", shortcutKeyEvent: { text: "backspace" }, afterItem: "Nudge" });

    Menu.addMenuItem({ menuName: "File", menuItemName: "Voxels", isSeparator: true, beforeItem: "Settings" });
    Menu.addMenuItem({ menuName: "File", menuItemName: "Export Voxels", shortcutKey: "CTRL+E", afterItem: "Voxels" });
    Menu.addMenuItem({ menuName: "File", menuItemName: "Import Voxels", shortcutKey: "CTRL+I", afterItem: "Export Voxels" });
}

function cleanupMenus() {
    // delete our menuitems
    Menu.removeSeparator("Edit", "Voxels");
    Menu.removeMenuItem("Edit", "Cut");
    Menu.removeMenuItem("Edit", "Copy");
    Menu.removeMenuItem("Edit", "Paste");
    Menu.removeMenuItem("Edit", "Nudge");
    Menu.removeMenuItem("Edit", "Delete");
    Menu.removeSeparator("File", "Voxels");
    Menu.removeMenuItem("File", "Export Voxels");
    Menu.removeMenuItem("File", "Import Voxels");
}

function menuItemEvent(menuItem) {

    // handle clipboard items
    if (editToolsOn) {

        var pickRay = Camera.computePickRay(trackLastMouseX, trackLastMouseY);
        var intersection = Voxels.findRayIntersection(pickRay);
        selectedVoxel = calculateVoxelFromIntersection(intersection,"select");
        if (menuItem == "Copy") {
            print("copying...");
            Clipboard.copyVoxel(selectedVoxel.x, selectedVoxel.y, selectedVoxel.z, selectedVoxel.s);
            pasteMode = true;
            moveTools();
        }
        if (menuItem == "Cut") {
            print("cutting...");
            Clipboard.cutVoxel(selectedVoxel.x, selectedVoxel.y, selectedVoxel.z, selectedVoxel.s);
            pasteMode = true;
            moveTools();
        }
        if (menuItem == "Paste") {
            if (isImporting) {
                print("placing import...");
                placeImport();
                showImport(false);
            } else {
                print("pasting...");
                Clipboard.pasteVoxel(selectedVoxel.x, selectedVoxel.y, selectedVoxel.z, selectedVoxel.s);
            }
            pasteMode = false;
            moveTools();
        }
        if (menuItem == "Delete") {
            print("deleting...");
            if (isImporting) {
                cancelImport();
            } else {
                Clipboard.deleteVoxel(selectedVoxel.x, selectedVoxel.y, selectedVoxel.z, selectedVoxel.s);
            }
        }
    
        if (menuItem == "Export Voxels") {
            print("export");
            Clipboard.exportVoxel(selectedVoxel.x, selectedVoxel.y, selectedVoxel.z, selectedVoxel.s);
        }
        if (menuItem == "Import Voxels") {
            print("importing...");
            if (importVoxels()) {
                showImport(true);
            }
            moveTools();
        }
        if (menuItem == "Nudge") {
            print("nudge");
            Clipboard.nudgeVoxel(selectedVoxel.x, selectedVoxel.y, selectedVoxel.z, selectedVoxel.s, { x: -1, y: 0, z: 0 });
        }
    }
}

function mouseMoveEvent(event) {
    if (!editToolsOn) {
        return;
    }
    if (inspectJsIsRunning) {
        return;
    }
    
    if (isMovingSlider) {
        thumbX = (event.x - thumbClickOffsetX) - sliderX;
        if (thumbX < minThumbX) {
            thumbX = minThumbX;
        }
        if (thumbX > maxThumbX) {
            thumbX = maxThumbX;
        }
        calcScaleFromThumb(thumbX);
        
    } else if (isAdding) {
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
    if (inspectJsIsRunning) {
        return;
    }
                          
    if (isMovingSlider) {
        isMovingSlider = false;
    }
    isAdding = false;
    isExtruding = false; 
}

function moveTools() {
    // move the swatches
    swatchesX = (windowDimensions.x - (swatchesWidth + sliderWidth)) / 2;
    swatchesY = windowDimensions.y - swatchHeight + 1;

    // create the overlays, position them in a row, set their colors, and for the selected one, use a different source image
    // location so that it displays the "selected" marker
    for (s = 0; s < numColors; s++) {
	    var extraWidth = 0;

	    if (s == 0) {
	        extraWidth = swatchExtraPadding;
	    }

	    var imageFromX = swatchExtraPadding - extraWidth + s * swatchWidth;
	    var imageFromY = swatchHeight + 1;
	    if (s == whichColor) {
	        imageFromY = 0;
	    }

	    var swatchX = swatchExtraPadding - extraWidth + swatchesX + ((swatchWidth - 1) * s);

	    if (s == (numColors - 1)) {
	        extraWidth = swatchExtraPadding;
	    }
		
        Overlays.editOverlay(swatches[s], {
                        x: swatchX,
                        y: swatchesY,
                        subImage: { x: imageFromX, y: imageFromY, width: swatchWidth + extraWidth, height: swatchHeight },
                        color: colors[s],
                        alpha: 1,
                        visible: editToolsOn
                    });
    }

    // move the tools
    toolsY = (windowDimensions.y - toolsHeight) / 2;

    var voxelToolOffset = 1,
        recolorToolOffset = 1,
        eyedropperToolOffset = 1;

    var voxelToolColor = WHITE_COLOR;

    if (recolorToolSelected) {
        recolorToolOffset = 2;
    } else if (eyedropperToolSelected) {
        eyedropperToolOffset = 2;
    } else {
        if (pasteMode) {
            voxelToolColor = pasteModeColor;
        }
        voxelToolOffset = 2;
    }

    Overlays.editOverlay(voxelTool, {
                    subImage: { x: 0, y: toolHeight * voxelToolOffset, width: toolWidth, height: toolHeight },
                    x: toolsX, y: toolsY + ((toolHeight + toolVerticalSpacing) * voxelToolAt), width: toolWidth, height: toolHeight,
                    color: voxelToolColor,
                    visible: editToolsOn
                });

    Overlays.editOverlay(recolorTool, {
                    subImage: { x: 0, y: toolHeight * recolorToolOffset, width: toolWidth, height: toolHeight },
                    x: toolsX, y: toolsY + ((toolHeight + toolVerticalSpacing) * recolorToolAt), width: toolWidth, height: toolHeight,
                    visible: editToolsOn
                });

    Overlays.editOverlay(eyedropperTool, {
                    subImage: { x: 0, y: toolHeight * eyedropperToolOffset, width: toolWidth, height: toolHeight },
                    x: toolsX, y: toolsY + ((toolHeight + toolVerticalSpacing) * eyedropperToolAt), width: toolWidth, height: toolHeight,
                    visible: editToolsOn
                });

    sliderX = swatchesX + swatchesWidth - sliderOffsetX;
    sliderY = windowDimensions.y - sliderHeight + 1;
    thumbY = sliderY + thumbOffsetY;
    Overlays.editOverlay(slider, { x: sliderX, y: sliderY, visible: editToolsOn });

    // This is the thumb of our slider
    Overlays.editOverlay(thumb, { x: sliderX + thumbX, y: thumbY, visible: editToolsOn });

}

var lastFingerAddVoxel = { x: -1, y: -1, z: -1}; // off of the build-able area
var lastFingerDeleteVoxel = { x: -1, y: -1, z: -1}; // off of the build-able area

function checkControllers() {
    var controllersPerPalm = 2; // palm and finger
    for (var palm = 0; palm < 2; palm++) {
        var palmController = palm * controllersPerPalm; 
        var fingerTipController = palmController + 1; 
        var fingerTipPosition = Controller.getSpatialControlPosition(fingerTipController);
        
        var BUTTON_COUNT = 6;
        var BUTTON_BASE = palm * BUTTON_COUNT;
        var BUTTON_1 = BUTTON_BASE + 1;
        var BUTTON_2 = BUTTON_BASE + 2;
        var FINGERTIP_VOXEL_SIZE = 0.05;

        if (Controller.isButtonPressed(BUTTON_1)) {
            if (Vec3.length(Vec3.subtract(fingerTipPosition,lastFingerAddVoxel)) > (FINGERTIP_VOXEL_SIZE / 2)) {
                if (whichColor == -1) {
                    newColor = { red: colors[0].red, green: colors[0].green, blue: colors[0].blue };
                } else {
                    newColor = { red: colors[whichColor].red, green: colors[whichColor].green, blue: colors[whichColor].blue };
                }

                Voxels.eraseVoxel(fingerTipPosition.x, fingerTipPosition.y, fingerTipPosition.z, FINGERTIP_VOXEL_SIZE);
                Voxels.setVoxel(fingerTipPosition.x, fingerTipPosition.y, fingerTipPosition.z, FINGERTIP_VOXEL_SIZE,
                    newColor.red, newColor.green, newColor.blue);
                
                lastFingerAddVoxel = fingerTipPosition;
            }
        } else if (Controller.isButtonPressed(BUTTON_2)) {
            if (Vec3.length(Vec3.subtract(fingerTipPosition,lastFingerDeleteVoxel)) > (FINGERTIP_VOXEL_SIZE / 2)) {
                Voxels.eraseVoxel(fingerTipPosition.x, fingerTipPosition.y, fingerTipPosition.z, FINGERTIP_VOXEL_SIZE);
                lastFingerDeleteVoxel = fingerTipPosition;
            }
        }
    }
}

function update(deltaTime) {
    if (editToolsOn) {
        var newWindowDimensions = Controller.getViewportDimensions();
        if (newWindowDimensions.x != windowDimensions.x || newWindowDimensions.y != windowDimensions.y) {
            windowDimensions = newWindowDimensions;
            moveTools();
        }

        checkControllers();

        // Move Import Preview
        if (isImporting) {
            var position = MyAvatar.position;
            var forwardVector = Quat.getFront(MyAvatar.orientation);
            var targetPosition = Vec3.sum(position, Vec3.multiply(forwardVector, importScale));
            var newPosition = {
                x: Math.floor(targetPosition.x / importScale) * importScale,
                y: Math.floor(targetPosition.y / importScale) * importScale,
                z: Math.floor(targetPosition.z / importScale) * importScale
            }
            moveImport(newPosition);
        }
    }
}

function wheelEvent(event) {
    wheelPixelsMoved += event.delta;
    if (Math.abs(wheelPixelsMoved) > WHEEL_PIXELS_PER_SCALE_CHANGE)
    {
        if (!pointerVoxelScaleSet) {
            pointerVoxelScale = 1.0;
            pointerVoxelScaleSet = true;
        }
        if (wheelPixelsMoved > 0) {
            pointerVoxelScale /= 2.0;
            if (pointerVoxelScale < MIN_VOXEL_SCALE) {
                pointerVoxelScale = MIN_VOXEL_SCALE;
            }  
        } else {
            pointerVoxelScale *= 2.0;
            if (pointerVoxelScale > MAX_VOXEL_SCALE) {
                pointerVoxelScale = MAX_VOXEL_SCALE;
            }
        }
        calcThumbFromScale(pointerVoxelScale);
        trackMouseEvent(event);
        wheelPixelsMoved = 0;

        if (isImporting) {
            var importScale = (pointerVoxelScale / MAX_VOXEL_SCALE) * MAX_PASTE_VOXEL_SCALE;
            rescaleImport(importScale);
        }
    }
}

// Controller.wheelEvent.connect(wheelEvent);
Controller.mousePressEvent.connect(mousePressEvent);
Controller.mouseReleaseEvent.connect(mouseReleaseEvent);
Controller.mouseMoveEvent.connect(mouseMoveEvent);
Controller.keyPressEvent.connect(keyPressEvent);
Controller.keyReleaseEvent.connect(keyReleaseEvent);
Controller.captureKeyEvents({ text: "+" });
Controller.captureKeyEvents({ text: "-" });


function scriptEnding() {
    Overlays.deleteOverlay(voxelPreview);
    Overlays.deleteOverlay(linePreviewTop);
    Overlays.deleteOverlay(linePreviewBottom);
    Overlays.deleteOverlay(linePreviewLeft);
    Overlays.deleteOverlay(linePreviewRight);
    for (s = 0; s < numColors; s++) {
        Overlays.deleteOverlay(swatches[s]);
    }
    Overlays.deleteOverlay(voxelTool);
    Overlays.deleteOverlay(recolorTool);
    Overlays.deleteOverlay(eyedropperTool);
    Overlays.deleteOverlay(slider);
    Overlays.deleteOverlay(thumb);
    Controller.releaseKeyEvents({ text: "+" });
    Controller.releaseKeyEvents({ text: "-" });
    cleanupImport();
    cleanupMenus();
}
Script.scriptEnding.connect(scriptEnding);

Script.update.connect(update);

setupMenus();
