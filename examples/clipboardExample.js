//
//  clipboardExample.js
//  hifi
//
//  Created by Brad Hefta-Gaub on 1/28/14.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//
//  This is an example script that demonstrates use of the Clipboard class
//
//

var selectedVoxel = { x: 0, y: 0, z: 0, s: 0 };
var selectedSize = 4;

function printKeyEvent(eventName, event) {
    print(eventName);
    print("    event.key=" + event.key);
    print("    event.text=" + event.text);
    print("    event.isShifted=" + event.isShifted);
    print("    event.isControl=" + event.isControl);
    print("    event.isMeta=" + event.isMeta);
    print("    event.isAlt=" + event.isAlt);
    print("    event.isKeypad=" + event.isKeypad);
}


function keyPressEvent(event) {
    var debug = true;
    if (debug) {
        printKeyEvent("keyPressEvent", event);
    }
}

function keyReleaseEvent(event) {
    var debug = true;
    if (debug) {
        printKeyEvent("keyReleaseEvent", event);
    }
    
    // Note: this sample uses Alt+ as the key codes for these clipboard items
    if ((event.key == 199 || event.key == 67 || event.text == "C" || event.text == "c") && event.isAlt) {
        print("the Alt+C key was pressed");
        Clipboard.copyVoxels(selectedVoxel.x, selectedVoxel.y, selectedVoxel.z, selectedVoxel.s);
    }
    if ((event.key == 8776 || event.key == 88 || event.text == "X" || event.text == "x") && event.isAlt) {
        print("the Alt+X key was pressed");
        Clipboard.cutVoxels(selectedVoxel.x, selectedVoxel.y, selectedVoxel.z, selectedVoxel.s);
    }
    if ((event.key == 8730 || event.key == 86 || event.text == "V" || event.text == "v") && event.isAlt) {
        print("the Alt+V key was pressed");
        Clipboard.pasteVoxels(selectedVoxel.x, selectedVoxel.y, selectedVoxel.z, selectedVoxel.s);
    }
    if (event.text == "DELETE" || event.text == "BACKSPACE") {
        print("the DELETE/BACKSPACE key was pressed");
        Clipboard.deleteVoxels(selectedVoxel.x, selectedVoxel.y, selectedVoxel.z, selectedVoxel.s);
    }
    
    if ((event.text == "E" || event.text == "e") && event.isMeta) {
        print("the Ctl+E key was pressed");
        Clipboard.exportVoxels(selectedVoxel.x, selectedVoxel.y, selectedVoxel.z, selectedVoxel.s);
    }
    if ((event.text == "I" || event.text == "i") && event.isMeta) {
        print("the Ctl+I key was pressed");
        Clipboard.importVoxels();
    }
}

// Map keyPress and mouse move events to our callbacks
Controller.keyPressEvent.connect(keyPressEvent);
Controller.keyReleaseEvent.connect(keyReleaseEvent);

var selectCube = Overlays.addOverlay("cube", {
                    position: { x: 0, y: 0, z: 0},
                    size: selectedSize,
                    color: { red: 255, green: 255, blue: 0},
                    alpha: 1,
                    solid: false,
                    visible: false,
                    lineWidth: 4
                });


function mouseMoveEvent(event) {

    var pickRay = Camera.computePickRay(event.x, event.y);

    var debug = false;
    if (debug) {
        print("mouseMoveEvent event.x,y=" + event.x + ", " + event.y);
        print("called Camera.computePickRay()");
        print("computePickRay origin=" + pickRay.origin.x + ", " + pickRay.origin.y + ", " + pickRay.origin.z);
        print("computePickRay direction=" + pickRay.direction.x + ", " + pickRay.direction.y + ", " + pickRay.direction.z);
    }

    var intersection = Voxels.findRayIntersection(pickRay);

    if (intersection.intersects) {
        if (debug) {
            print("intersection voxel.red/green/blue=" + intersection.voxel.red + ", " 
                        + intersection.voxel.green + ", " + intersection.voxel.blue);
            print("intersection voxel.x/y/z/s=" + intersection.voxel.x + ", " 
                        + intersection.voxel.y + ", " + intersection.voxel.z+ ": " + intersection.voxel.s);
            print("intersection face=" + intersection.face);
            print("intersection distance=" + intersection.distance);
            print("intersection intersection.x/y/z=" + intersection.intersection.x + ", " 
                        + intersection.intersection.y + ", " + intersection.intersection.z);
        }                    
                    
        
        
        var x = Math.floor(intersection.voxel.x / selectedSize) * selectedSize;
        var y = Math.floor(intersection.voxel.y / selectedSize) * selectedSize;
        var z = Math.floor(intersection.voxel.z / selectedSize) * selectedSize;
        selectedVoxel = { x: x, y: y, z: z, s: selectedSize };
        Overlays.editOverlay(selectCube, { position: selectedVoxel, size: selectedSize, visible: true } );
    } else {
        Overlays.editOverlay(selectCube, { visible: false } );
        selectedVoxel = { x: 0, y: 0, z: 0, s: 0 };
    }
}

Controller.mouseMoveEvent.connect(mouseMoveEvent);

function wheelEvent(event) {
    var debug = false;
    if (debug) {
        print("wheelEvent");
        print("    event.x,y=" + event.x + ", " + event.y);
        print("    event.delta=" + event.delta);
        print("    event.orientation=" + event.orientation);
        print("    event.isLeftButton=" + event.isLeftButton);
        print("    event.isRightButton=" + event.isRightButton);
        print("    event.isMiddleButton=" + event.isMiddleButton);
        print("    event.isShifted=" + event.isShifted);
        print("    event.isControl=" + event.isControl);
        print("    event.isMeta=" + event.isMeta);
        print("    event.isAlt=" + event.isAlt);
    }
}

Controller.wheelEvent.connect(wheelEvent);

function scriptEnding() {
    Overlays.deleteOverlay(selectCube);
}

Script.scriptEnding.connect(scriptEnding);
