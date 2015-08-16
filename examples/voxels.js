var controlHeld = false;
var shiftHeld = false;

function floorVector(v) {
    return {x: Math.floor(v.x), y: Math.floor(v.y), z: Math.floor(v.z)};
}

function backUpByOneVoxel(pickRayDirInVoxelSpace, originalVoxelPosition, low, high) {
    // try to back up along pickRay an amount that changes only one coordinate.  this does
    // a binary search for a pickRay multiplier that only causes one of the 3 coordinates to change.
    var originalVoxelCoords = floorVector(originalVoxelPosition)
    if (high - low < 0.001) {
        print("voxel backup-by-1 failed");
        // give up.
        return oCoords;
    }
    var middle = (low + high) / 2.0;

    var backupVector = Vec3.multiply(pickRayDirInVoxelSpace, -middle);
    print("backupVector = " + "{" + backupVector.x + ", " + backupVector.y + ", " + backupVector.z + "}");
    var nPosition = Vec3.sum(originalVoxelPosition, backupVector); // possible neighbor coordinates
    var nCoords = floorVector(nPosition);

    print("middle = " + middle +
          " -- {" + originalVoxelCoords.x + ", " + originalVoxelCoords.y + ", " + originalVoxelCoords.z + "}" +
          " -- {" + nCoords.x + ", " + nCoords.y + ", " + nCoords.z + "}");


    if (nCoords.x == originalVoxelCoords.x &&
        nCoords.y == originalVoxelCoords.y &&
        nCoords.z == originalVoxelCoords.z) {
        // still in the original voxel.  back up more...
        return backUpByOneVoxel(pickRayDirInVoxelSpace, originalVoxelPosition, middle, high);
    }

    if (nCoords.x != originalVoxelCoords.x &&
        nCoords.y == originalVoxelCoords.y &&
        nCoords.z == originalVoxelCoords.z) {
        return nCoords;
    }
    if (nCoords.x == originalVoxelCoords.x &&
        nCoords.y != originalVoxelCoords.y &&
        nCoords.z == originalVoxelCoords.z) {
        return nCoords;
    }
    if (nCoords.x == originalVoxelCoords.x &&
        nCoords.y == originalVoxelCoords.y &&
        nCoords.z != originalVoxelCoords.z) {
        return nCoords;
    }

    // more than one coordinate changed, but no less...
    return backUpByOneVoxel(pickRayDirInVoxelSpace, originalVoxelPosition, low, middle);
}


function attemptVoxelChange(pickRayDir, intersection) {
    var voxelPosition = Entities.worldCoordsToVoxelCoords(intersection.entityID, intersection.intersection);
    var pickRayDirInVoxelSpace = Entities.localCoordsToVoxelCoords(intersection.entityID, pickRayDir);

    print("voxelPosition = " + voxelPosition.x + ", " + voxelPosition.y + ", " + voxelPosition.z);
    print("pickRay = " + pickRayDir.x + ", " + pickRayDir.y + ", " + pickRayDir.z);
    print("pickRayInVoxelSpace = " +
          pickRayDirInVoxelSpace.x + ", " + pickRayDirInVoxelSpace.y + ", " + pickRayDirInVoxelSpace.z);

    if (controlHeld) {
        // hold control to erase a voxel
        return Entities.setVoxel(intersection.entityID, floorVector(voxelPosition), 0);
    } else if (shiftHeld) {
        // hold shift to set all voxels to 255
        return Entities.setAllVoxels(intersection.entityID, 255);
    } else {
        // no modifier key to add a voxel
        var backOneVoxel = backUpByOneVoxel(pickRayDirInVoxelSpace, voxelPosition, 0.0, 1.0);
        return Entities.setVoxel(intersection.entityID, backOneVoxel, 255);
    }

    // Entities.setVoxelSphere(id, intersection.intersection, radius, 0)
}


function mousePressEvent(event) {
    if (!event.isLeftButton) {
        return;
    }

    var pickRay = Camera.computePickRay(event.x, event.y);
    var intersection = Entities.findRayIntersection(pickRay, true); // accurate picking
    var properties;

    if (intersection.intersects) {
        properties = Entities.getEntityProperties(intersection.entityID);
        print("got accurate pick");
        if (properties.type == "PolyVox") {
            if (attemptVoxelChange(pickRay.direction, intersection)) {
                return;
            }
        } else {
            print("not a polyvox: " + properties.type);
        }
    }

    print("trying bounding-box pick");

    // if the PolyVox entity is empty, we can't pick against its "on" voxels.  try picking against its
    // bounding box, instead.
    intersection = Entities.findRayIntersection(pickRay, false); // bounding box picking
    if (intersection.intersects) {
        attemptVoxelChange(pickRay.direction, intersection);
    }
}


function keyPressEvent(event) {
    if (event.text == "CONTROL") {
        controlHeld = true;
    }
    if (event.text == "SHIFT") {
        shiftHeld = true;
    }
}


function keyReleaseEvent(event) {
    if (event.text == "CONTROL") {
        controlHeld = false;
    }
    if (event.text == "SHIFT") {
        shiftHeld = false;
    }
}


Controller.mousePressEvent.connect(mousePressEvent);
Controller.keyPressEvent.connect(keyPressEvent);
Controller.keyReleaseEvent.connect(keyReleaseEvent);
