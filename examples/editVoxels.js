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

function vLength(v) {
    return Math.sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

function vMinus(a, b) { 
    var rval = { x: a.x - b.x, y: a.y - b.y, z: a.z - b.z };
    return rval;
}

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

function mousePressEvent(event) {
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
            orbitRadius = vLength(orbitVector); 
            orbitAzimuth = Math.atan2(orbitVector.z, orbitVector.x);
            orbitAltitude = Math.asin(orbitVector.y / Vec3.length(orbitVector));

        } else if (event.isRightButton || event.isControl) {
            //  Delete voxel
            Voxels.eraseVoxel(intersection.voxel.x, intersection.voxel.y, intersection.voxel.z, intersection.voxel.s);
            Audio.playSound(deleteSound, audioOptions);

        } else if (event.isShifted) {
            //  Recolor Voxel
            Voxels.setVoxel(intersection.voxel.x, 
                            intersection.voxel.y, 
                            intersection.voxel.z, 
                            intersection.voxel.s, 
                            colors[whichColor].red, colors[whichColor].green, colors[whichColor].blue);
            Audio.playSound(changeColorSound, audioOptions);
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
            dragStart = { x: event.x, y: event.y };
            isAdding = true;
        }       
    }
}

function keyPressEvent(event) {
    key_alt = event.isAlt;
    key_shift = event.isShifted;
    var nVal = parseInt(event.text);
    if (event.text == "0") {
        print("Color = Copy");
        whichColor = -1;
        Audio.playSound(clickSound, audioOptions);
    } else if ((nVal > 0) && (nVal <= numColors)) {
        whichColor = nVal - 1;
        print("Color = " + (whichColor + 1));
        Audio.playSound(clickSound, audioOptions);
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
    } else if (event.text == " ") {
        //  Reset my orientation!
        var orientation = { x:0, y:0, z:0, w:1 };
        Camera.setOrientation(orientation);
        MyAvatar.orientation = orientation;
    }
}

function keyReleaseEvent(event) {
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
            var distance = vLength(lastVoxelDistance);
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
}

function mouseReleaseEvent(event) {
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

Controller.mousePressEvent.connect(mousePressEvent);
Controller.mouseReleaseEvent.connect(mouseReleaseEvent);
Controller.mouseMoveEvent.connect(mouseMoveEvent);
Controller.keyPressEvent.connect(keyPressEvent);
Controller.keyReleaseEvent.connect(keyReleaseEvent);

function scriptEnding() {
}
Script.scriptEnding.connect(scriptEnding);
