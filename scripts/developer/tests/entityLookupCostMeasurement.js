// Creates a large number of entities on the cardinal planes of the octree (all
// objects will live in the root octree element).  Measures how long it takes
// to update the properties of the first and last entity.  The difference
// between the two measurements shows how the cost of lookup changes as a
// function of the number of entities.  For best results run this in an
// otherwise empty domain.

var firstId;
var lastId;
var NUM_ENTITIES_ON_SIDE = 25;

// create the objects
createObjects = function () {
    var STRIDE = 0.75;
    var WIDTH = 0.5;
    var DIMENSIONS = { x: WIDTH, y: WIDTH, z: WIDTH };
    var LIFETIME = 20;

    var properties = {
        name: "",
        type : "Box",
        dimensions : DIMENSIONS,
        position : { x: 0, y: 0, z: 0},
        lifetime : LIFETIME,
        color : { red:255, green: 64, blue: 255 }
    };

    // xy
    var planeName = "xy";
    for (var i = 0; i < NUM_ENTITIES_ON_SIDE; ++i) {
        for (var j = 0; j < NUM_ENTITIES_ON_SIDE; ++j) {
            properties.name = "Box-" + planeName + "-" + i + "." + j;
            properties.position = { x: i * STRIDE, y: j * STRIDE, z: 0 };
            var red = i * 255 / NUM_ENTITIES_ON_SIDE;
            var green = j * 255 / NUM_ENTITIES_ON_SIDE;
            var blue = 0;
            properties.color = { red: red, green: green, blue: blue };
            if (i == 0 && j == 0) {
                firstId = Entities.addEntity(properties);
            } else {
                Entities.addEntity(properties);
            }
        }
    }

    // yz
    var planeName = "yz";
    for (var i = 0; i < NUM_ENTITIES_ON_SIDE; ++i) {
        for (var j = 0; j < NUM_ENTITIES_ON_SIDE; ++j) {
            properties.name = "Box-" + planeName + "-" + i + "." + j;
            properties.position = { x: 0, y: i * STRIDE, z: j * STRIDE };
            var red = 0;
            var green = i * 255 / NUM_ENTITIES_ON_SIDE;
            var blue = j * 255 / NUM_ENTITIES_ON_SIDE;
            properties.color = { red: red, green: green, blue: blue };
            Entities.addEntity(properties);
        }
    }

    // zx
    var planeName = "zx";
    for (var i = 0; i < NUM_ENTITIES_ON_SIDE; ++i) {
        for (var j = 0; j < NUM_ENTITIES_ON_SIDE; ++j) {
            properties.name = "Box-" + planeName + "-" + i + "." + j;
            properties.position = { x: j * STRIDE, y: 0, z: i * STRIDE };
            var red = j * 255 / NUM_ENTITIES_ON_SIDE;
            var green = 0;
            var blue = i * 255 / NUM_ENTITIES_ON_SIDE;
            properties.color = { red: red, green: green, blue: blue };
            lastId = Entities.addEntity(properties);
        }
    }
};

createObjects();

// measure the time it takes to edit the first and last entities many times
// (requires a lookup by entityId each time)
changeProperties = function (id) {
    var newProperties = { color : { red: 255, green: 255, blue: 255 } };
    Entities.editEntity(id, newProperties);
}

// first
var NUM_CHANGES = 10000;
var firstStart = Date.now();
for (var k = 0; k < NUM_CHANGES; ++k) {
    changeProperties(firstId);
}
var firstEnd = Date.now();
var firstDt = firstEnd - firstStart;

// last
var lastStart = Date.now();
for (var k = 0; k < NUM_CHANGES; ++k) {
    changeProperties(lastId);
}
var lastEnd = Date.now();
var lastDt = lastEnd - lastStart;

// print the results
var numEntities = 3 * NUM_ENTITIES_ON_SIDE * NUM_ENTITIES_ON_SIDE;
print("numEntities = " + numEntities + "  numEdits = " + NUM_CHANGES + "  firstDt = " + firstDt + "  lastDt = " + lastDt);

