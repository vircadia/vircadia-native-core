var controlHeld = false;
var shiftHeld = false;

function floorVector(v) {
    return {x: Math.floor(v.x), y: Math.floor(v.y), z: Math.floor(v.z)};
}

function attemptVoxelChange(pickRayDir, intersection) {

    var properties = Entities.getEntityProperties(intersection.entityID);
    if (properties.type != "PolyVox") {
        return false;
    }

    var voxelPosition = Entities.worldCoordsToVoxelCoords(intersection.entityID, intersection.intersection);
    voxelPosition = Vec3.subtract(voxelPosition, {x: 0.5, y: 0.5, z: 0.5});
    var pickRayDirInVoxelSpace = Entities.localCoordsToVoxelCoords(intersection.entityID, pickRayDir);
    pickRayDirInVoxelSpace = Vec3.normalize(pickRayDirInVoxelSpace);

    if (controlHeld) {
        // hold control to erase a voxel
        var toErasePosition = Vec3.sum(voxelPosition, Vec3.multiply(pickRayDirInVoxelSpace, 0.1));
        return Entities.setVoxel(intersection.entityID, floorVector(toErasePosition), 0);
    } else if (shiftHeld) {
        // hold shift to set all voxels to 255
        return Entities.setAllVoxels(intersection.entityID, 255);
    } else {
        // no modifier key to add a voxel
        var toDrawPosition = Vec3.subtract(voxelPosition, Vec3.multiply(pickRayDirInVoxelSpace, 0.1));
        return Entities.setVoxel(intersection.entityID, floorVector(toDrawPosition), 255);
    }

    // Entities.setVoxelSphere(id, intersection.intersection, radius, 0)
}

function mousePressEvent(event) {
    if (!event.isLeftButton) {
        return;
    }

    var pickRay = Camera.computePickRay(event.x, event.y);
    var intersection = Entities.findRayIntersection(pickRay, true); // accurate picking

    if (intersection.intersects) {
        if (attemptVoxelChange(pickRay.direction, intersection)) {
            return;
        }
    }

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
