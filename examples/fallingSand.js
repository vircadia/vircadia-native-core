var SAND_LIFETIME = 100.0;
var UPDATE_RATE = 1.0;
var SAND_SIZE = 0.125;

var sandArray = [];
var numSand = 0;


var red = 255;
var green = 0;
var blue = 0;

var SOURCE_POSITION = MyAvatar.position;

function update() {

    for (var i = 0; i < numSand; i++) {
        updateSand(i);
    }
    
    var voxel = Voxels.getVoxelAt(SOURCE_POSITION.x, SOURCE_POSITION.y, SOURCE_POSITION.z, SAND_SIZE);
    if (voxel.s == 0) {
        sandArray[numSand] = {x: SOURCE_POSITION.x, y: SOURCE_POSITION.y, z: SOURCE_POSITION.z, s: SAND_SIZE, r: red, g: green, b: blue};
        numSand++;
        Voxels.setVoxel(SOURCE_POSITION.x, SOURCE_POSITION.y, SOURCE_POSITION.z, SAND_SIZE, red, green, blue);
    }
}

function isVoxelEmpty(x, y, z) {
    var adjacent = Voxels.getVoxelAt(x, y, z, SAND_SIZE);
    return (adjacent.s == 0);
}

function tryMoveVoxel(voxel, x, y, z) {
    //If the adjacent voxel is empty, we will move to it.
    if (isVoxelEmpty(x, y, z)) {
        moveVoxel(voxel, x, y, z);
        return true;
    }
    return false;
}

function moveVoxel(voxel, x, y, z) {
    Voxels.eraseVoxel(voxel.x, voxel.y, voxel.z, SAND_SIZE);
    Voxels.setVoxel(x, y, z, SAND_SIZE, red, green, blue);
    voxel.x = x;
    voxel.y = y;
    voxel.z = z;
}

function updateSand(i) {
    var voxel = sandArray[i];
    
    //Down
    if (tryMoveVoxel(voxel, voxel.x, voxel.y - SAND_SIZE, voxel.z)) return true;
    //Left
    if (isVoxelEmpty(voxel.x - SAND_SIZE, voxel.y, voxel.z) &&
        isVoxelEmpty(voxel.x - SAND_SIZE, voxel.y - SAND_SIZE, voxel.z)) {
        moveVoxel(voxel, voxel.x - SAND_SIZE, voxel.y, voxel.z);
        return true;
    }
    //Back
    if (isVoxelEmpty(voxel.x, voxel.y, voxel.z - SAND_SIZE) &&
        isVoxelEmpty(voxel.x, voxel.y - SAND_SIZE, voxel.z - SAND_SIZE)) {
        moveVoxel(voxel, voxel.x, voxel.y, voxel.z - SAND_SIZE);
        return true;
    }
    //Right
    if (isVoxelEmpty(voxel.x + SAND_SIZE, voxel.y, voxel.z) &&
        isVoxelEmpty(voxel.x + SAND_SIZE, voxel.y - SAND_SIZE, voxel.z)) {
        moveVoxel(voxel, voxel.x + SAND_SIZE, voxel.y, voxel.z);
        return true;
    }
    //Front
    if (isVoxelEmpty(voxel.x, voxel.y, voxel.z + SAND_SIZE) &&
        isVoxelEmpty(voxel.x, voxel.y - SAND_SIZE, voxel.z + SAND_SIZE)) {
        moveVoxel(voxel, voxel.x, voxel.y, voxel.z + SAND_SIZE);
        return true;
    }
    return false;
}

function scriptEnding() {
    for (var i = 0; i < numSand; i++) {
        var voxel = sandArray[i];
        Voxels.eraseVoxel(voxel.x, voxel.y, voxel.z, SAND_SIZE);
    }
}

Script.update.connect(update);
Script.scriptEnding.connect(scriptEnding);
