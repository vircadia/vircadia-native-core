// compute a position to create the object relative to avatar
var forwardOffset = Vec3.multiply(2.0, Quat.getFront(MyAvatar.orientation));
var objectPosition = Vec3.sum(MyAvatar.position, forwardOffset);

var LIFETIME = 1800; //seconds
var DIM_HEIGHT = 1, DIM_WIDTH = 1, DIM_DEPTH = 1;
var COLOR_R = 100, COLOR_G = 10, COLOR_B = 200;

var properties = {
        name: "ShapeSpawnTest",
        type: "Shape",
        shape: "Cylinder",
        dimensions: {x: DIM_WIDTH, y: DIM_HEIGHT, z: DIM_DEPTH},
        color: {red: COLOR_R, green: COLOR_G, blue: COLOR_B},
        position: objectPosition,
        lifetime: LIFETIME,
};

// create the object
var entityId = Entities.addEntity(properties);

function cleanup() {
    Entities.deleteEntity(entityId);
}

// delete the object when this script is stopped
Script.scriptEnding.connect(cleanup);



