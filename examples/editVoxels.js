//
//  editVoxels.js
//  examples
//
//  Created by Philip Rosedale on February 8, 2014
//  Copyright 2014 High Fidelity, Inc.
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
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("libraries/globals.js");

var editToolsOn = true; // starts out off

var windowDimensions = Controller.getViewportDimensions();
var WORLD_SCALE = Voxels.getTreeScale();

var NEW_VOXEL_SIZE = 1.0;
var NEW_VOXEL_DISTANCE_FROM_CAMERA = 3.0;
var PIXELS_PER_EXTRUDE_VOXEL = 16;
var WHEEL_PIXELS_PER_SCALE_CHANGE = 100;
var MAX_VOXEL_SCALE_POWER = 4;
var MIN_VOXEL_SCALE_POWER = -8;
var MAX_VOXEL_SCALE = Math.pow(2.0, MAX_VOXEL_SCALE_POWER);
var MIN_VOXEL_SCALE = Math.pow(2.0, MIN_VOXEL_SCALE_POWER);
var WHITE_COLOR = { red: 255, green: 255, blue: 255 };

var MAX_PASTE_VOXEL_SCALE = 256;
var MIN_PASTE_VOXEL_SCALE = .256;

var zFightingSizeAdjustRatio = 0.004; // used to adjust preview voxels to prevent z fighting
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
colors[8] = { red: 36,  green: 64,  blue: 64 };
var numColors = 9;
var whichColor = 0;            //  Starting color is 'Copy' mode

//  Create sounds for for every script actions that require one
// start with audio slightly above the avatar
var audioOptions = {
  position: Vec3.sum(MyAvatar.position, { x: 0, y: 1, z: 0 }  ),
  volume: 1.0
};

function SoundArray() {
    this.audioOptions = audioOptions
    this.sounds = new Array();
    this.addSound = function (soundURL) {
        this.sounds[this.sounds.length] = SoundCache.getSound(soundURL);
    }
    this.play = function (index) {
        if (0 <= index && index < this.sounds.length) {
            Audio.playSound(this.sounds[index], this.audioOptions);
        } else {
            print("[ERROR] editVoxels.js:randSound.play() : Index " + index + " out of range.");
        }
    }
    this.playRandom = function () {
        if (this.sounds.length > 0) {
            rand = Math.floor(Math.random() * this.sounds.length);
            Audio.playSound(this.sounds[rand], this.audioOptions);
        } else {
            print("[ERROR] editVoxels.js:randSound.playRandom() : Array is empty.");
        }
    }
}

var addVoxelSound = new SoundArray();
addVoxelSound.addSound(HIFI_PUBLIC_BUCKET + "sounds/Voxel+Editing/Voxel+Add/VA+1.raw");
addVoxelSound.addSound(HIFI_PUBLIC_BUCKET + "sounds/Voxel+Editing/Voxel+Add/VA+2.raw");
addVoxelSound.addSound(HIFI_PUBLIC_BUCKET + "sounds/Voxel+Editing/Voxel+Add/VA+3.raw");
addVoxelSound.addSound(HIFI_PUBLIC_BUCKET + "sounds/Voxel+Editing/Voxel+Add/VA+4.raw");
addVoxelSound.addSound(HIFI_PUBLIC_BUCKET + "sounds/Voxel+Editing/Voxel+Add/VA+5.raw");
addVoxelSound.addSound(HIFI_PUBLIC_BUCKET + "sounds/Voxel+Editing/Voxel+Add/VA+6.raw");

var delVoxelSound = new SoundArray();
delVoxelSound.addSound(HIFI_PUBLIC_BUCKET + "sounds/Voxel+Editing/Voxel+Del/VD+A1.raw");
delVoxelSound.addSound(HIFI_PUBLIC_BUCKET + "sounds/Voxel+Editing/Voxel+Del/VD+A2.raw");
delVoxelSound.addSound(HIFI_PUBLIC_BUCKET + "sounds/Voxel+Editing/Voxel+Del/VD+A3.raw");
delVoxelSound.addSound(HIFI_PUBLIC_BUCKET + "sounds/Voxel+Editing/Voxel+Del/VD+B1.raw");
delVoxelSound.addSound(HIFI_PUBLIC_BUCKET + "sounds/Voxel+Editing/Voxel+Del/VD+B2.raw");
delVoxelSound.addSound(HIFI_PUBLIC_BUCKET + "sounds/Voxel+Editing/Voxel+Del/VD+B3.raw");

var resizeVoxelSound = new SoundArray();
resizeVoxelSound.addSound(HIFI_PUBLIC_BUCKET + "sounds/Voxel+Editing/Voxel+Size/V+Size+Minus.raw");
resizeVoxelSound.addSound(HIFI_PUBLIC_BUCKET + "sounds/Voxel+Editing/Voxel+Size/V+Size+Plus.raw");
var voxelSizeMinus = 0;
var voxelSizePlus = 1;

var swatchesSound = new SoundArray();
swatchesSound.addSound(HIFI_PUBLIC_BUCKET + "sounds/Voxel+Editing/Swatches/Swatch+1.raw");
swatchesSound.addSound(HIFI_PUBLIC_BUCKET + "sounds/Voxel+Editing/Swatches/Swatch+2.raw");
swatchesSound.addSound(HIFI_PUBLIC_BUCKET + "sounds/Voxel+Editing/Swatches/Swatch+3.raw");
swatchesSound.addSound(HIFI_PUBLIC_BUCKET + "sounds/Voxel+Editing/Swatches/Swatch+4.raw");
swatchesSound.addSound(HIFI_PUBLIC_BUCKET + "sounds/Voxel+Editing/Swatches/Swatch+5.raw");
swatchesSound.addSound(HIFI_PUBLIC_BUCKET + "sounds/Voxel+Editing/Swatches/Swatch+6.raw");
swatchesSound.addSound(HIFI_PUBLIC_BUCKET + "sounds/Voxel+Editing/Swatches/Swatch+7.raw");
swatchesSound.addSound(HIFI_PUBLIC_BUCKET + "sounds/Voxel+Editing/Swatches/Swatch+8.raw");
swatchesSound.addSound(HIFI_PUBLIC_BUCKET + "sounds/Voxel+Editing/Swatches/Swatch+9.raw");

var undoSound = new SoundArray();
undoSound.addSound(HIFI_PUBLIC_BUCKET + "sounds/Voxel+Editing/Undo/Undo+1.raw");
undoSound.addSound(HIFI_PUBLIC_BUCKET + "sounds/Voxel+Editing/Undo/Undo+2.raw");
undoSound.addSound(HIFI_PUBLIC_BUCKET + "sounds/Voxel+Editing/Undo/Undo+3.raw");

var scriptInitSound = new SoundArray();
scriptInitSound.addSound(HIFI_PUBLIC_BUCKET + "sounds/Voxel+Editing/Script+Init/Script+Init+A.raw");
scriptInitSound.addSound(HIFI_PUBLIC_BUCKET + "sounds/Voxel+Editing/Script+Init/Script+Init+B.raw");
scriptInitSound.addSound(HIFI_PUBLIC_BUCKET + "sounds/Voxel+Editing/Script+Init/Script+Init+C.raw");
scriptInitSound.addSound(HIFI_PUBLIC_BUCKET + "sounds/Voxel+Editing/Script+Init/Script+Init+D.raw");

var modeSwitchSound = new SoundArray();
modeSwitchSound.addSound(HIFI_PUBLIC_BUCKET + "sounds/Voxel+Editing/Mode+Switch/Mode+1.raw");
modeSwitchSound.addSound(HIFI_PUBLIC_BUCKET + "sounds/Voxel+Editing/Mode+Switch/Mode+2.raw");
modeSwitchSound.addSound(HIFI_PUBLIC_BUCKET + "sounds/Voxel+Editing/Mode+Switch/Mode+3.raw");

var initialVoxelSound = new SoundArray();
initialVoxelSound.addSound(HIFI_PUBLIC_BUCKET + "sounds/Voxel+Editing/Initial+Voxel/Initial+V.raw");

var colorInheritSound = new SoundArray();
colorInheritSound.addSound(HIFI_PUBLIC_BUCKET + "sounds/Voxel+Editing/Color+Inherit/Inherit+A.raw");
colorInheritSound.addSound(HIFI_PUBLIC_BUCKET + "sounds/Voxel+Editing/Color+Inherit/Inherit+B.raw");
colorInheritSound.addSound(HIFI_PUBLIC_BUCKET + "sounds/Voxel+Editing/Color+Inherit/Inherit+C.raw");

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

var linePreviewTop = [];
var linePreviewBottom = [];
var linePreviewLeft = [];
var linePreviewRight = [];

// Currend cursor index
var currentCursor = 0;
                        
function addLineOverlay() {
    return Overlays.addOverlay("line3d", {
                                         position: { x: 0, y: 0, z: 0},
                                         end: { x: 0, y: 0, z: 0},
                                         color: { red: 255, green: 255, blue: 255},
                                         alpha: 1,
                                         visible: false,
                                         lineWidth: previewLineWidth
                                         });
}
                  
//Cursor line previews for up to three cursors                  
linePreviewTop[0] = addLineOverlay();       
linePreviewTop[1] = addLineOverlay();
linePreviewTop[2] = addLineOverlay();

linePreviewBottom[0] = addLineOverlay();
linePreviewBottom[1] = addLineOverlay();
linePreviewBottom[2] = addLineOverlay();
                                            
linePreviewLeft[0] = addLineOverlay();
linePreviewLeft[1] = addLineOverlay();
linePreviewLeft[2] = addLineOverlay();
                                          
linePreviewRight[0] = addLineOverlay();
linePreviewRight[1] = addLineOverlay();
linePreviewRight[2] = addLineOverlay();

// these will be used below
var scaleSelectorWidth = 144;
var scaleSelectorHeight = 37;

// These will be our "overlay IDs"
var swatches = new Array();
var swatchExtraPadding = 5;
var swatchHeight = 37;
var swatchWidth = 27;
var swatchesWidth = swatchWidth * numColors + numColors + swatchExtraPadding * 2;
var swatchesX = (windowDimensions.x - (swatchesWidth + scaleSelectorWidth)) / 2;
var swatchesY = windowDimensions.y - swatchHeight + 1;

var toolIconUrl = HIFI_PUBLIC_BUCKET + "images/tools/";

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
                                    visible: editToolsOn,
                                    alpha: 0.9
                                    });

var recolorTool = Overlays.addOverlay("image", {
                                      x: 0, y: 0, width: toolWidth, height: toolHeight,
                                      subImage: { x: 0, y: toolHeight, width: toolWidth, height: toolHeight },
                                      imageURL: toolIconUrl + "paint-tool.svg",
                                      visible: editToolsOn,
                                      alpha: 0.9
                                      });

var eyedropperTool = Overlays.addOverlay("image", {
                                         x: 0, y: 0, width: toolWidth, height: toolHeight,
                                         subImage: { x: 0, y: toolHeight, width: toolWidth, height: toolHeight },
                                         imageURL: toolIconUrl + "eyedropper-tool.svg",
                                         visible: editToolsOn,
                                         alpha: 0.9
                                         });


var copyScale = true;
function ScaleSelector() {
    this.x = swatchesX + swatchesWidth;
    this.y = swatchesY;
    this.width = scaleSelectorWidth;
    this.height = scaleSelectorHeight;
    
    this.displayPower = false;
    this.scale = 1.0;
    this.power = 0;
    
    this.FIRST_PART = this.width * 40.0 / 100.0;
    this.SECOND_PART = this.width * 37.0 / 100.0;
    
    this.buttonsOverlay = Overlays.addOverlay("image", {
                                              x: this.x, y: this.y,
                                              width: this.width, height: this.height,
                                              //subImage: { x: 0, y: toolHeight, width: toolWidth, height: toolHeight },
                                              imageURL: toolIconUrl + "voxel-size-selector.svg",
                                              alpha: 0.9,
                                              visible: editToolsOn
                                              });
    this.textOverlay = Overlays.addOverlay("text", {
                                           x: this.x + this.FIRST_PART, y: this.y,
                                           width: this.SECOND_PART, height: this.height,
                                           topMargin: 13,
                                           text: this.scale.toString(),
                                           backgroundAlpha: 0.0,
                                           visible: editToolsOn
                                           });
    this.powerOverlay = Overlays.addOverlay("text", {
                                            x: this.x + this.FIRST_PART, y: this.y,
                                            width: this.SECOND_PART, height: this.height,
                                            leftMargin: 28,
                                            text: this.power.toString(),
                                            backgroundAlpha: 0.0,
                                            visible: false
                                            });
    this.setScale = function(scale) {
        if (scale > MAX_VOXEL_SCALE) {
            scale = MAX_VOXEL_SCALE;
        }
        if (scale < MIN_VOXEL_SCALE) {
            scale = MIN_VOXEL_SCALE;
        }
        
        this.scale = scale;
        this.power = Math.floor(Math.log(scale) / Math.log(2));
        rescaleImport();
        this.update();
    }
    
    this.show = function(doShow) {
        Overlays.editOverlay(this.buttonsOverlay, {visible: doShow});
        Overlays.editOverlay(this.textOverlay, {visible: doShow});
        Overlays.editOverlay(this.powerOverlay, {visible: doShow && this.displayPower});
    }
    
    this.move = function() {
        this.x = swatchesX + swatchesWidth;
        this.y = swatchesY;
        
        Overlays.editOverlay(this.buttonsOverlay, {
                             x: this.x, y: this.y,
                             });
        Overlays.editOverlay(this.textOverlay, {
                             x: this.x + this.FIRST_PART, y: this.y,
                             });
        Overlays.editOverlay(this.powerOverlay, {
                             x: this.x + this.FIRST_PART, y: this.y,
                             });
    }
    
    
    this.switchDisplay = function() {
        this.displayPower = !this.displayPower;
        
        if (this.displayPower) {
            Overlays.editOverlay(this.textOverlay, {
                                 leftMargin: 18,
                                 text: "2"
                                 });
            Overlays.editOverlay(this.powerOverlay, {
                                 text: this.power.toString(),
                                 visible: editToolsOn
                                 });
        } else {
            Overlays.editOverlay(this.textOverlay, {
                                 leftMargin: 13,
                                 text: this.scale.toString()
                                 });
            Overlays.editOverlay(this.powerOverlay, {
                                 visible: false
                                 });
        }
    }
    
    this.update = function() {
        if (this.displayPower) {
            Overlays.editOverlay(this.powerOverlay, {text: this.power.toString()});
        } else {
            Overlays.editOverlay(this.textOverlay, {text: this.scale.toString()});
        }
    }
    
    this.incrementScale = function() {
        copyScale = false;
        if (this.power < MAX_VOXEL_SCALE_POWER) {
            ++this.power;
            this.scale *= 2.0;
            this.update();
            rescaleImport();
            resizeVoxelSound.play(voxelSizePlus);
        }
    }
    
    this.decrementScale = function() {
        copyScale = false;
        if (MIN_VOXEL_SCALE_POWER < this.power) {
            --this.power;
            this.scale /= 2.0;
            this.update();
            rescaleImport();
            resizeVoxelSound.play(voxelSizePlus);
        }
    }
    
    this.clicked = function(x, y) {
        if (this.x < x  && x < this.x + this.width &&
            this.y < y && y < this.y + this.height) {
            
            if (x < this.x + this.FIRST_PART) {
                this.decrementScale();
            } else if (x < this.x + this.FIRST_PART + this.SECOND_PART) {
                this.switchDisplay();
            } else {
                this.incrementScale();
            }
            return true;
        }
        return false;
    }
    
    this.cleanup = function() {
        Overlays.deleteOverlay(this.buttonsOverlay);
        Overlays.deleteOverlay(this.textOverlay);
        Overlays.deleteOverlay(this.powerOverlay);
    }
    
}
var scaleSelector = new ScaleSelector();


///////////////////////////////////// IMPORT MODULE ///////////////////////////////
// Move the following code to a separate file when include will be available.
var importTree;
var importPreview;
var importBoundaries;
var xImportGuide;
var yImportGuide;
var zImportGuide;
var isImporting;
var importPosition;
var importDistance;

function initImport() {
    importPreview = Overlays.addOverlay("localvoxels", {
                                        name: "import",
                                        position: { x: 0, y: 0, z: 0},
                                        scale: 1,
                                        visible: false
                                        });
    importBoundaries = Overlays.addOverlay("cube", {
                                           position: { x: 0, y: 0, z: 0 },
                                           size: 1,
                                           color: { red: 128, blue: 128, green: 128 },
                                           lineWIdth: 4,
                                           solid: false,
                                           visible: false
                                           });
    
    xImportGuide = Overlays.addOverlay("line3d", {
                                       position: { x: 0, y: 0, z: 0},
                                       end: { x: 0, y: 0, z: 0},
                                       color: { red: 255, green: 0, blue: 0},
                                       alpha: 1,
                                       visible: false,
                                       lineWidth: previewLineWidth
                                       });
    yImportGuide = Overlays.addOverlay("line3d", {
                                       position: { x: 0, y: 0, z: 0},
                                       end: { x: 0, y: 0, z: 0},
                                       color: { red: 0, green: 255, blue: 0},
                                       alpha: 1,
                                       visible: false,
                                       lineWidth: previewLineWidth
                                       });
    zImportGuide = Overlays.addOverlay("line3d", {
                                       position: { x: 0, y: 0, z: 0},
                                       end: { x: 0, y: 0, z: 0},
                                       color: { red: 0, green: 0, blue: 255},
                                       alpha: 1,
                                       visible: false,
                                       lineWidth: previewLineWidth
                                       });
    
    
    isImporting = false;
    importPosition = { x: 0, y: 0, z: 0 };
}

function importVoxels() {
    isImporting = Clipboard.importVoxels();
    return isImporting;
}

function moveImport(position) {
    importPosition = position;
    Overlays.editOverlay(importPreview, {
                         position: { x: importPosition.x, y: importPosition.y, z: importPosition.z }
                         });
    Overlays.editOverlay(importBoundaries, {
                         position: { x: importPosition.x, y: importPosition.y, z: importPosition.z }
                         });
    
    
    Overlays.editOverlay(xImportGuide, {
                         position: { x: importPosition.x, y: 0, z: importPosition.z },
                         end: { x: importPosition.x + scaleSelector.scale, y: 0, z: importPosition.z }
                         });
    Overlays.editOverlay(yImportGuide, {
                         position: { x: importPosition.x, y: importPosition.y, z: importPosition.z },
                         end: { x: importPosition.x, y: 0, z: importPosition.z }
                         });
    Overlays.editOverlay(zImportGuide, {
                         position: { x: importPosition.x, y: 0, z: importPosition.z },
                         end: { x: importPosition.x, y: 0, z: importPosition.z + scaleSelector.scale }
                         });
    rescaleImport();
}

function rescaleImport() {
    if (0 < scaleSelector.scale) {
        Overlays.editOverlay(importPreview, {
                             scale: scaleSelector.scale
                             });
        Overlays.editOverlay(importBoundaries, {
                             size: scaleSelector.scale
                             });
        
        Overlays.editOverlay(xImportGuide, {
                             end: { x: importPosition.x + scaleSelector.scale, y: 0, z: importPosition.z }
                             });
        Overlays.editOverlay(yImportGuide, {
                             end: { x: importPosition.x, y: 0, z: importPosition.z }
                             });
        Overlays.editOverlay(zImportGuide, {
                             end: { x: importPosition.x, y: 0, z: importPosition.z + scaleSelector.scale }
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
    
    Overlays.editOverlay(xImportGuide, {
                         visible: doShow
                         });
    Overlays.editOverlay(yImportGuide, {
                         visible: doShow
                         });
    Overlays.editOverlay(zImportGuide, {
                         visible: doShow
                         });
}

function placeImport() {
    if (isImporting) {
        Clipboard.pasteVoxel(importPosition.x, importPosition.y, importPosition.z, scaleSelector.scale);
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
    Overlays.deleteOverlay(xImportGuide);
    Overlays.deleteOverlay(yImportGuide);
    Overlays.deleteOverlay(zImportGuide);
    isImporting = false;
    importPostion = { x: 0, y: 0, z: 0 };
}
/////////////////////////////////// END IMPORT MODULE /////////////////////////////
initImport();

if (editToolsOn) {
    moveTools();
}

function setAudioPosition() {
    var position = MyAvatar.position;
    var forwardVector = Quat.getFront(MyAvatar.orientation);
    audioOptions.position = Vec3.sum(position, forwardVector);
}

function getNewPasteVoxel(pickRay) {
    
    var voxelSize = scaleSelector.scale;
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

var voxelToolSelected = false;
var recolorToolSelected = false;
var eyedropperToolSelected = false;
var pasteMode = false;


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
    
    var voxelSize = scaleSelector.scale;
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

    var zFightingSizeAdjust = zFightingSizeAdjustRatio * intersection.distance;
    
    // now we also want to calculate the "edge square" for the face for this voxel
    if (intersection.face == "MIN_X_FACE") {

        highlightAt.x = x - zFightingSizeAdjust;
        highlightAt.y = y + zFightingSizeAdjust;
        highlightAt.z = z + zFightingSizeAdjust;
        voxelSize -= 2 * zFightingSizeAdjust;
        if (wantAddAdjust) {
            resultVoxel.x -= voxelSize;
        }
        
        resultVoxel.bottomLeft = {x: highlightAt.x, y: highlightAt.y, z: highlightAt.z };
        resultVoxel.bottomRight = {x: highlightAt.x, y: highlightAt.y, z: highlightAt.z + voxelSize };
        resultVoxel.topLeft = {x: highlightAt.x, y: highlightAt.y + voxelSize, z: highlightAt.z };
        resultVoxel.topRight = {x: highlightAt.x, y: highlightAt.y + voxelSize, z: highlightAt.z + voxelSize };
        
    } else if (intersection.face == "MAX_X_FACE") {
        
        highlightAt.x = x + voxelSize + zFightingSizeAdjust;
        highlightAt.y = y + zFightingSizeAdjust;
        highlightAt.z = z + zFightingSizeAdjust;
        voxelSize -= 2 * zFightingSizeAdjust;
        if (wantAddAdjust) {
            resultVoxel.x += resultVoxel.s;
        }
        
        resultVoxel.bottomRight = {x: highlightAt.x, y: highlightAt.y, z: highlightAt.z };
        resultVoxel.bottomLeft = {x: highlightAt.x, y: highlightAt.y, z: highlightAt.z + voxelSize };
        resultVoxel.topRight = {x: highlightAt.x, y: highlightAt.y + voxelSize, z: highlightAt.z };
        resultVoxel.topLeft = {x: highlightAt.x, y: highlightAt.y + voxelSize, z: highlightAt.z + voxelSize };
        
    } else if (intersection.face == "MIN_Y_FACE") {
        
        highlightAt.x = x + zFightingSizeAdjust;
        highlightAt.y = y - zFightingSizeAdjust;
        highlightAt.z = z + zFightingSizeAdjust;
        voxelSize -= 2 * zFightingSizeAdjust;
        if (wantAddAdjust) {
            resultVoxel.y -= voxelSize;
        }
        
        resultVoxel.topRight = {x: highlightAt.x , y: highlightAt.y, z: highlightAt.z  };
        resultVoxel.topLeft = {x: highlightAt.x + voxelSize, y: highlightAt.y, z: highlightAt.z };
        resultVoxel.bottomRight = {x: highlightAt.x , y: highlightAt.y, z: highlightAt.z  + voxelSize };
        resultVoxel.bottomLeft = {x: highlightAt.x + voxelSize , y: highlightAt.y, z: highlightAt.z + voxelSize };
        
    } else if (intersection.face == "MAX_Y_FACE") {
        
        highlightAt.x = x + zFightingSizeAdjust;
        highlightAt.y = y + voxelSize + zFightingSizeAdjust;
        highlightAt.z = z + zFightingSizeAdjust;
        voxelSize -= 2 * zFightingSizeAdjust;
        if (wantAddAdjust) {
            resultVoxel.y += resultVoxel.s;
        }
        
        resultVoxel.bottomRight = {x: highlightAt.x, y: highlightAt.y, z: highlightAt.z };
        resultVoxel.bottomLeft = {x: highlightAt.x + voxelSize, y: highlightAt.y, z: highlightAt.z};
        resultVoxel.topRight = {x: highlightAt.x, y: highlightAt.y, z: highlightAt.z  + voxelSize};
        resultVoxel.topLeft = {x: highlightAt.x + voxelSize, y: highlightAt.y, z: highlightAt.z + voxelSize};
        
    } else if (intersection.face == "MIN_Z_FACE") {
        
        highlightAt.x = x + zFightingSizeAdjust;
        highlightAt.y = y + zFightingSizeAdjust;
        highlightAt.z = z - zFightingSizeAdjust;
        voxelSize -= 2 * zFightingSizeAdjust;
        if (wantAddAdjust) {
            resultVoxel.z -= voxelSize;
        }
        
        resultVoxel.bottomRight = {x: highlightAt.x, y: highlightAt.y, z: highlightAt.z };
        resultVoxel.bottomLeft = {x: highlightAt.x + voxelSize, y: highlightAt.y, z: highlightAt.z};
        resultVoxel.topRight = {x: highlightAt.x, y: highlightAt.y + voxelSize, z: highlightAt.z };
        resultVoxel.topLeft = {x: highlightAt.x + voxelSize, y: highlightAt.y + voxelSize, z: highlightAt.z};
        
    } else if (intersection.face == "MAX_Z_FACE") {
        
        highlightAt.x = x + zFightingSizeAdjust;
        highlightAt.y = y + zFightingSizeAdjust;
        highlightAt.z = z + voxelSize + zFightingSizeAdjust;
        voxelSize -= 2 * zFightingSizeAdjust;
        if (wantAddAdjust) {
            resultVoxel.z += resultVoxel.s;
        }
        
        resultVoxel.bottomLeft = {x: highlightAt.x, y: highlightAt.y, z: highlightAt.z };
        resultVoxel.bottomRight = {x: highlightAt.x + voxelSize, y: highlightAt.y, z: highlightAt.z};
        resultVoxel.topLeft = {x: highlightAt.x, y: highlightAt.y + voxelSize, z: highlightAt.z };
        resultVoxel.topRight = {x: highlightAt.x + voxelSize, y: highlightAt.y + voxelSize, z: highlightAt.z};
        
    }
    
    return resultVoxel;
}

function showPreviewVoxel() {
    var voxelColor;
    
    var pickRay = Camera.computePickRay(trackLastMouseX, trackLastMouseY);
    var intersection = Voxels.findRayIntersection(pickRay);
    
    voxelColor = { red: colors[whichColor].red,
        green: colors[whichColor].green,
        blue: colors[whichColor].blue };
    
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
        Overlays.editOverlay(linePreviewBottom[currentCursor], {
                             position: pasteVoxel.origin,
                             end: {x: pasteVoxel.origin.x + pasteVoxel.voxelSize, y: pasteVoxel.origin.y, z: pasteVoxel.origin.z },
                             visible: true
                             });
        
        // Y axis
        Overlays.editOverlay(linePreviewRight[currentCursor], {
                             position: pasteVoxel.origin,
                             end: {x: pasteVoxel.origin.x, y: pasteVoxel.origin.y + pasteVoxel.voxelSize, z: pasteVoxel.origin.z },
                             visible: true
                             });
        
        // Z axis
        Overlays.editOverlay(linePreviewTop[currentCursor], {
                             position: pasteVoxel.origin,
                             end: {x: pasteVoxel.origin.x, y: pasteVoxel.origin.y, z: pasteVoxel.origin.z - pasteVoxel.voxelSize },
                             visible: true
                             });
        
        return;
    }
    
    var intersection = Voxels.findRayIntersection(pickRay);
    
    if (intersection.intersects) {
        resultVoxel = calculateVoxelFromIntersection(intersection,"");
        Overlays.editOverlay(voxelPreview, { visible: false });
        Overlays.editOverlay(linePreviewTop[currentCursor], { position: resultVoxel.topLeft, end: resultVoxel.topRight, visible: true });
        Overlays.editOverlay(linePreviewBottom[currentCursor], { position: resultVoxel.bottomLeft, end: resultVoxel.bottomRight, visible: true });
        Overlays.editOverlay(linePreviewLeft[currentCursor], { position: resultVoxel.topLeft, end: resultVoxel.bottomLeft, visible: true });
        Overlays.editOverlay(linePreviewRight[currentCursor], { position: resultVoxel.topRight, end: resultVoxel.bottomRight, visible: true });
        colors[0] = {red: intersection.voxel.red, green: intersection.voxel.green , blue: intersection.voxel.blue };
        
        if (copyScale) {
            scaleSelector.setScale(intersection.voxel.s);
        }
        moveTools();
    } else if (intersection.accurate) {
        Overlays.editOverlay(voxelPreview, { visible: false });
        Overlays.editOverlay(linePreviewTop[currentCursor], { visible: false });
        Overlays.editOverlay(linePreviewBottom[currentCursor], { visible: false });
        Overlays.editOverlay(linePreviewLeft[currentCursor], { visible: false });
        Overlays.editOverlay(linePreviewRight[currentCursor], { visible: false });
    }
}

function showPreviewGuides() {
    if (editToolsOn && !isImporting && (voxelToolSelected || recolorToolSelected || eyedropperToolSelected)) {
        if (previewAsVoxel) {
            showPreviewVoxel();
            
            // make sure alternative is hidden
            Overlays.editOverlay(linePreviewTop[currentCursor], { visible: false });
            Overlays.editOverlay(linePreviewBottom[currentCursor], { visible: false });
            Overlays.editOverlay(linePreviewLeft[currentCursor], { visible: false });
            Overlays.editOverlay(linePreviewRight[currentCursor], { visible: false });
        } else {
            showPreviewLines();
        }
    } else {
        // make sure all previews are off
        Overlays.editOverlay(voxelPreview, { visible: false });
        Overlays.editOverlay(linePreviewTop[currentCursor], { visible: false });
        Overlays.editOverlay(linePreviewBottom[currentCursor], { visible: false });
        Overlays.editOverlay(linePreviewLeft[currentCursor], { visible: false });
        Overlays.editOverlay(linePreviewRight[currentCursor], { visible: false });
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
        scaleSelector.show(editToolsOn);
        scriptInitSound.playRandom();
    }
    
    if (event.text == "ALT") {
        inspectJsIsRunning = false;
    }
    
    if (editToolsOn) {
        if (event.text == "ESC") {
            pasteMode = false;
            moveTools();
        }
        if (event.text == "-") {
            scaleSelector.decrementScale();
        }
        if (event.text == "+") {
            scaleSelector.incrementScale();
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
    
    if (event.deviceID == 1500) { // Left Hydra Controller
        currentCursor = 0;
    } else if (event.deviceID == 1501) { // Right Hydra Controller
        currentCursor = 1;
    } else {
        currentCursor = 2;
    }
    
    var clickedOnSomething = false;
    var clickedOverlay = Overlays.getOverlayAtPoint({x: event.x, y: event.y});
    
    if (clickedOverlay == voxelTool) {
        modeSwitchSound.play(0);
        voxelToolSelected = !voxelToolSelected;
        recolorToolSelected = false;
        eyedropperToolSelected = false;
        moveTools();
        clickedOnSomething = true;
    } else if (clickedOverlay == recolorTool) {
        modeSwitchSound.play(1);
        voxelToolSelected = false;
        recolorToolSelected = !recolorToolSelected;
        eyedropperToolSelected = false;
        moveTools();
        clickedOnSomething = true;
    } else if (clickedOverlay == eyedropperTool) {
        modeSwitchSound.play(2);
        voxelToolSelected = false;
        recolorToolSelected = false;
        eyedropperToolSelected = !eyedropperToolSelected;
        moveTools();
        clickedOnSomething = true;
    } else if (scaleSelector.clicked(event.x, event.y)) {
        if (isImporting) {
            rescaleImport();
        }
        clickedOnSomething = true;
    } else {
        // if the user clicked on one of the color swatches, update the selectedSwatch
        for (s = 0; s < numColors; s++) {
            if (clickedOverlay == swatches[s]) {
                whichColor = s;
                moveTools();
                clickedOnSomething = true;
                swatchesSound.play(whichColor);
                break;
            }
        }
    }
    if (clickedOnSomething || isImporting || (!voxelToolSelected && !recolorToolSelected && !eyedropperToolSelected)) {
        return; // no further processing
    }
    
    // TODO: does any of this stuff need to execute if we're panning or orbiting?
    trackMouseEvent(event); // used by preview support
    var pickRay = Camera.computePickRay(event.x, event.y);
    var intersection = Voxels.findRayIntersection(pickRay);
    audioOptions.position = Vec3.sum(pickRay.origin, pickRay.direction);
    
    if (pasteMode) {
        var pasteVoxel = getNewPasteVoxel(pickRay);
        Clipboard.pasteVoxel(pasteVoxel.origin.x, pasteVoxel.origin.y, pasteVoxel.origin.z, pasteVoxel.voxelSize);
        pasteMode = false;
        moveTools();
        return;
    }
    
    if (intersection.intersects) {
        if (trackAsDelete || event.isRightButton && !trackAsEyedropper) {
            //  Delete voxel
            voxelDetails = calculateVoxelFromIntersection(intersection,"delete");
            Voxels.eraseVoxel(voxelDetails.x, voxelDetails.y, voxelDetails.z, voxelDetails.s);
            delVoxelSound.playRandom();
            Overlays.editOverlay(voxelPreview, { visible: false });
        } else if (eyedropperToolSelected || trackAsEyedropper) {
            colors[whichColor].red = intersection.voxel.red;
            colors[whichColor].green = intersection.voxel.green;
            colors[whichColor].blue = intersection.voxel.blue;
            moveTools();
            swatchesSound.play(whichColor);
            
        } else if (recolorToolSelected || trackAsRecolor) {
            //  Recolor Voxel
            voxelDetails = calculateVoxelFromIntersection(intersection,"recolor");
            
            // doing this erase then set will make sure we only recolor just the target voxel
            Voxels.setVoxel(voxelDetails.x, voxelDetails.y, voxelDetails.z, voxelDetails.s,
                            colors[whichColor].red, colors[whichColor].green, colors[whichColor].blue);
            swatchesSound.play(whichColor);
            Overlays.editOverlay(voxelPreview, { visible: false });
        } else if (voxelToolSelected) {
            //  Add voxel on face
            newColor = { red: colors[whichColor].red,
                        green: colors[whichColor].green,
                        blue: colors[whichColor].blue
            };
            
            voxelDetails = calculateVoxelFromIntersection(intersection,"add");
            Voxels.setVoxel(voxelDetails.x, voxelDetails.y, voxelDetails.z, voxelDetails.s,
                            newColor.red, newColor.green, newColor.blue);
            lastVoxelPosition = { x: voxelDetails.x, y: voxelDetails.y, z: voxelDetails.z };
            lastVoxelColor = { red: newColor.red, green: newColor.green, blue: newColor.blue };
            lastVoxelScale = voxelDetails.s;
            if (lastVoxelScale > MAX_VOXEL_SCALE) {
              lastVoxelScale = MAX_VOXEL_SCALE; 
            }
            
            addVoxelSound.playRandom();
            
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
            copyScale = true;
        } else if ((nVal > 0) && (nVal <= numColors)) {
            whichColor = nVal - 1;
            print("Color = " + (whichColor + 1));
            swatchesSound.play(whichColor);
            moveTools();
        } else if (event.text == "0" && voxelToolSelected) {
            // Create a brand new 1 meter voxel in front of your avatar
            var newPosition = getNewVoxelPosition();
            var newVoxel = {
            x: newPosition.x,
            y: newPosition.y ,
            z: newPosition.z,
            s: NEW_VOXEL_SIZE,
            red: colors[whichColor].red,
            green: colors[whichColor].green,
                blue: colors[whichColor].blue };
            Voxels.setVoxel(newVoxel.x, newVoxel.y, newVoxel.z, newVoxel.s, newVoxel.red, newVoxel.green, newVoxel.blue);
            setAudioPosition();
            initialVoxelSound.playRandom();
        } else if (event.text == "z") {
            undoSound.playRandom();
        }
    }
    
    trackKeyPressEvent(event); // used by preview support
}

function keyReleaseEvent(event) {
    trackKeyReleaseEvent(event); // used by preview support
}


// In order for editVoxels and editModels to play nice together, they each check to see if a "delete" menu item already
// exists. If it doesn't they add it. If it does they don't. They also only delete the menu item if they were the one that
// added it.
var voxelMenuAddedDelete = false;
function setupVoxelMenus() {
    // hook up menus
    Menu.menuItemEvent.connect(menuItemEvent);
    
    // add our menuitems
    Menu.addMenuItem({ menuName: "Edit", menuItemName: "Voxels", isSeparator: true, beforeItem: "Physics" });
    Menu.addMenuItem({ menuName: "Edit", menuItemName: "Cut", shortcutKey: "CTRL+X", afterItem: "Voxels" });
    Menu.addMenuItem({ menuName: "Edit", menuItemName: "Copy", shortcutKey: "CTRL+C", afterItem: "Cut" });
    Menu.addMenuItem({ menuName: "Edit", menuItemName: "Paste", shortcutKey: "CTRL+V", afterItem: "Copy" });
    Menu.addMenuItem({ menuName: "Edit", menuItemName: "Nudge", shortcutKey: "CTRL+N", afterItem: "Paste" });


    if (!Menu.menuItemExists("Edit","Delete")) {
        Menu.addMenuItem({ menuName: "Edit", menuItemName: "Delete", 
            shortcutKeyEvent: { text: "backspace" }, afterItem: "Nudge" });
        voxelMenuAddedDelete = true;
    }
    
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
    if (voxelMenuAddedDelete) {
        Menu.removeMenuItem("Edit", "Delete");
    }
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
            } else if (voxelToolSelected) {
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
    if (!editToolsOn || inspectJsIsRunning) {
      return;
    }
    
    if (event.deviceID == 1500) { // Left Hydra Controller
        currentCursor = 0;
    } else if (event.deviceID == 1501) { // Right Hydra Controller
        currentCursor = 1;
    } else {
        currentCursor = 2;
    }
    
    // Move Import Preview
    if (isImporting) {
        var pickRay = Camera.computePickRay(event.x, event.y);
        var intersection = Voxels.findRayIntersection(pickRay);
        
        var distance = 2 * scaleSelector.scale;
        
        if (intersection.intersects) {
            var intersectionDistance = Vec3.length(Vec3.subtract(pickRay.origin, intersection.intersection));
            if (intersectionDistance < distance) {
                distance = intersectionDistance * 0.99;
            }
            
        }
        
        var targetPosition = { x: pickRay.direction.x * distance,
                               y: pickRay.direction.y * distance,
                               z: pickRay.direction.z * distance
        };
        targetPosition.x += pickRay.origin.x;
        targetPosition.y += pickRay.origin.y;
        targetPosition.z += pickRay.origin.z;

        if (targetPosition.x < 0) targetPosition.x = 0;
        if (targetPosition.y < 0) targetPosition.y = 0;
        if (targetPosition.z < 0) targetPosition.z = 0;
        
        var nudgeFactor = scaleSelector.scale;
        var newPosition = {
        x: Math.floor(targetPosition.x / nudgeFactor) * nudgeFactor,
        y: Math.floor(targetPosition.y / nudgeFactor) * nudgeFactor,
        z: Math.floor(targetPosition.z / nudgeFactor) * nudgeFactor
        }
        
        moveImport(newPosition);
    }

    if (isAdding) {
        var pickRay = Camera.computePickRay(event.x, event.y);
        var distance = Vec3.length(Vec3.subtract(pickRay.origin, lastVoxelPosition));
        var mouseSpot = Vec3.sum(Vec3.multiply(pickRay.direction, distance), pickRay.origin);
        var delta = Vec3.subtract(mouseSpot, lastVoxelPosition);

        if (!isExtruding) {
            //  Use the drag direction to tell which way to 'extrude' this voxel
            extrudeScale = lastVoxelScale;
            extrudeDirection = { x: 0, y: 0, z: 0 };
            isExtruding = true;
            if (delta.x > lastVoxelScale) extrudeDirection.x = 1;
            else if (delta.x < -lastVoxelScale) extrudeDirection.x = -1;
            else if (delta.y > lastVoxelScale) extrudeDirection.y = 1;
            else if (delta.y < -lastVoxelScale) extrudeDirection.y = -1;
            else if (delta.z > lastVoxelScale) extrudeDirection.z = 1;
            else if (delta.z < -lastVoxelScale) extrudeDirection.z = -1;
            else isExtruding = false;
        } else {
            //  Extrude if mouse has moved by a voxel in the extrude direction
            var distanceInDirection = Vec3.dot(delta, extrudeDirection);
            if (distanceInDirection > extrudeScale) {
                lastVoxelPosition = Vec3.sum(lastVoxelPosition, Vec3.multiply(extrudeDirection, extrudeScale));
                Voxels.setVoxel(lastVoxelPosition.x, lastVoxelPosition.y, lastVoxelPosition.z, extrudeScale,
                                lastVoxelColor.red, lastVoxelColor.green, lastVoxelColor.blue);
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
    
    isAdding = false;
    isExtruding = false;
}

function moveTools() {
    // move the swatches
    swatchesX = (windowDimensions.x - (swatchesWidth + scaleSelectorWidth)) / 2;
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
    } else if (voxelToolSelected) {
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
    
    scaleSelector.move();
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
                newColor = { red: colors[whichColor].red, green: colors[whichColor].green, blue: colors[whichColor].blue };
                
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
    }
}

function wheelEvent(event) {
    wheelPixelsMoved += event.delta;
    if (Math.abs(wheelPixelsMoved) > WHEEL_PIXELS_PER_SCALE_CHANGE)
    {
        
        if (wheelPixelsMoved > 0) {
            scaleSelector.decrementScale();
        } else {
            scaleSelector.incrementScale();
        }
        trackMouseEvent(event);
        wheelPixelsMoved = 0;
        
        if (isImporting) {
            rescaleImport();
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
    for (var i = 0; i < linePreviewTop.length; i++) {
        Overlays.deleteOverlay(linePreviewTop[i]);
        Overlays.deleteOverlay(linePreviewBottom[i]);
        Overlays.deleteOverlay(linePreviewLeft[i]);
        Overlays.deleteOverlay(linePreviewRight[i]);
    }
    for (s = 0; s < numColors; s++) {
        Overlays.deleteOverlay(swatches[s]);
    }
    Overlays.deleteOverlay(voxelTool);
    Overlays.deleteOverlay(recolorTool);
    Overlays.deleteOverlay(eyedropperTool);
    Controller.releaseKeyEvents({ text: "+" });
    Controller.releaseKeyEvents({ text: "-" });
    cleanupImport();
    scaleSelector.cleanup();
    cleanupMenus();
}
Script.scriptEnding.connect(scriptEnding);

Script.update.connect(update);

setupVoxelMenus();
