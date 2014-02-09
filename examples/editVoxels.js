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

var key_alt = false;
var key_shift = false;

//  Create a table of the different colors you can choose
var colors = new Array();
colors[0] = { red: 255, green: 0, blue: 0 };
colors[1] = { red: 0, green: 255, blue: 0 };
colors[2] = { red: 0, green: 0, blue: 255 };
colors[3] = { red: 255, green: 255, blue: 0 };
colors[4] = { red: 0, green: 255, blue: 255 };
colors[5] = { red: 255, green: 0, blue: 255 };
var numColors = 6;
var whichColor = 0;

//  Create sounds for adding, deleting, recoloring voxels 
var addSound = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Electronic/ElectronicBurst1.raw");
var deleteSound = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Bubbles/bubbles1.raw");
var changeColorSound = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Electronic/ElectronicBurst6.raw");
var audioOptions = new AudioInjectionOptions(); 

function mousePressEvent(event) {
    print("mousePressEvent event.x,y=" + event.x + ", " + event.y);
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
            Audio.playSound(addSound, audioOptions);
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

Controller.mousePressEvent.connect(mousePressEvent);
Controller.keyPressEvent.connect(keyPressEvent);
Controller.keyReleaseEvent.connect(keyReleaseEvent);

function scriptEnding() {
}
Script.scriptEnding.connect(scriptEnding);
