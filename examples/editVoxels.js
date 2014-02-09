//
//  editVoxels.js
//  hifi
//
//  Created by Philip Rosedale on February 8, 2014
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//
//  Captures mouse clicks and edits voxels accordingly.
//
//  click = create a new voxel on this face, same color as old 
//  Alt + click = delete this voxel 
//  shift + click = recolor this voxel
//
//  Click and drag to create more new voxels in the same direction
//

function vLength(v) {
    return Math.sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

var key_alt = false;
var key_shift = false;
var isAdding = false; 

var lastVoxelPosition = { x: 0, y: 0, z: 0 };
var lastVoxelColor = { red: 0, green: 0, blue: 0 };
var lastVoxelScale = 0;
var dragStart = { x: 0, y: 0 };

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
var numColors = 6;
var whichColor = 0;

//  Create sounds for adding, deleting, recoloring voxels 
var addSound = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Electronic/ElectronicBurst1.raw");
var deleteSound = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Bubbles/bubbles1.raw");
var changeColorSound = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Electronic/ElectronicBurst6.raw");
var audioOptions = new AudioInjectionOptions(); 

function mousePressEvent(event) {
    var pickRay = Camera.computePickRay(event.x, event.y);
    var intersection = Voxels.findRayIntersection(pickRay);
    audioOptions.volume = 1.0;
    audioOptions.position = { x: intersection.voxel.x, y: intersection.voxel.y, z: intersection.voxel.z };

    if (intersection.intersects) {
         if (key_alt) {
            //  Delete voxel
            Voxels.eraseVoxel(intersection.voxel.x, intersection.voxel.y, intersection.voxel.z, intersection.voxel.s);
            Audio.playSound(deleteSound, audioOptions);

        } else if (key_shift) {
            //  Recolor Voxel
            whichColor++;
            if (whichColor == numColors) whichColor = 0; 
            Voxels.setVoxel(intersection.voxel.x, 
                            intersection.voxel.y, 
                            intersection.voxel.z, 
                            intersection.voxel.s, 
                            colors[whichColor].red, colors[whichColor].green, colors[whichColor].blue);
            Audio.playSound(changeColorSound, audioOptions);
        } else {
            //  Add voxel on face
            var newVoxel = {    
                    x: intersection.voxel.x,
                    y: intersection.voxel.y,
                    z: intersection.voxel.z,
                    s: intersection.voxel.s,
                    red: intersection.voxel.red,
                    green: intersection.voxel.green,
                    blue: intersection.voxel.blue };
                    
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
            dragStart = { x: event.x, y: event.y };
            isAdding = true;
        }       
    }
}

function keyPressEvent(event) {
    key_alt = event.isAlt;
    key_shift = event.isShifted;
}
function keyReleaseEvent(event) {
    key_alt = false;
    key_shift = false; 
}
function mouseMoveEvent(event) {
    if (isAdding) {
        var pickRay = Camera.computePickRay(event.x, event.y);
        var lastVoxelDistance = { x: pickRay.origin.x - lastVoxelPosition.x, 
                                    y: pickRay.origin.y - lastVoxelPosition.y, 
                                    z: pickRay.origin.z - lastVoxelPosition.z };
        var distance = vLength(lastVoxelDistance);
        var mouseSpot = { x: pickRay.direction.x * distance, y: pickRay.direction.y * distance, z: pickRay.direction.z * distance };
        mouseSpot.x += pickRay.origin.x;
        mouseSpot.y += pickRay.origin.y;
        mouseSpot.z += pickRay.origin.z;
        var dx = mouseSpot.x - lastVoxelPosition.x;
        var dy = mouseSpot.y - lastVoxelPosition.y;
        var dz = mouseSpot.z - lastVoxelPosition.z;
        if (dx > lastVoxelScale) {
            lastVoxelPosition.x += lastVoxelScale;
            Voxels.setVoxel(lastVoxelPosition.x, lastVoxelPosition.y, lastVoxelPosition.z, 
                            lastVoxelScale, lastVoxelColor.red, lastVoxelColor.green, lastVoxelColor.blue);
        } else if (dx < -lastVoxelScale) {
            lastVoxelPosition.x -= lastVoxelScale;
            Voxels.setVoxel(lastVoxelPosition.x, lastVoxelPosition.y, lastVoxelPosition.z, 
                            lastVoxelScale, lastVoxelColor.red, lastVoxelColor.green, lastVoxelColor.blue);
        } else if (dy > lastVoxelScale) {
            lastVoxelPosition.y += lastVoxelScale;
            Voxels.setVoxel(lastVoxelPosition.x, lastVoxelPosition.y, lastVoxelPosition.z, 
                            lastVoxelScale, lastVoxelColor.red, lastVoxelColor.green, lastVoxelColor.blue);
        } else if (dy < -lastVoxelScale) {
            lastVoxelPosition.y -= lastVoxelScale;
            Voxels.setVoxel(lastVoxelPosition.x, lastVoxelPosition.y, lastVoxelPosition.z, 
                            lastVoxelScale, lastVoxelColor.red, lastVoxelColor.green, lastVoxelColor.blue);
        } else if (dz > lastVoxelScale) {
            lastVoxelPosition.z += lastVoxelScale;
            Voxels.setVoxel(lastVoxelPosition.x, lastVoxelPosition.y, lastVoxelPosition.z, 
                            lastVoxelScale, lastVoxelColor.red, lastVoxelColor.green, lastVoxelColor.blue);
        } else if (dz < -lastVoxelScale) {
            lastVoxelPosition.z -= lastVoxelScale;
            Voxels.setVoxel(lastVoxelPosition.x, lastVoxelPosition.y, lastVoxelPosition.z, 
                            lastVoxelScale, lastVoxelColor.red, lastVoxelColor.green, lastVoxelColor.blue);
        }
    }
}

function mouseReleaseEvent(event) {
    isAdding = false;
}

Controller.mousePressEvent.connect(mousePressEvent);
Controller.mouseReleaseEvent.connect(mouseReleaseEvent);
Controller.mouseMoveEvent.connect(mouseMoveEvent);
Controller.keyPressEvent.connect(keyPressEvent);
Controller.keyReleaseEvent.connect(keyReleaseEvent);

function scriptEnding() {
}
Script.scriptEnding.connect(scriptEnding);
