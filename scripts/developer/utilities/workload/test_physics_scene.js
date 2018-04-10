
var DEFAULT_LIFETIME = 120;

var GRID_WORLD_SIZE = 100.0;
var GRID_WORLD_RESOLUTION = 2.0;

var BACKDROP_SIZE = GRID_WORLD_SIZE / GRID_WORLD_RESOLUTION;
var BACKDROP_HALFSIZE = BACKDROP_SIZE *0.5;
var BACKDROP_MIN_C = -2;

var ROOT_Z_OFFSET = -3;
var ROOT_Y_OFFSET = -0.1;

var TILE_UNIT = GRID_WORLD_RESOLUTION;
var TILE_DIM = { x: TILE_UNIT, y: TILE_UNIT, z: TILE_UNIT};
var GRID_TILE_OFFSET = Vec3.multiply(0.5, TILE_DIM);

var OBJECT_DIM = { x: 0.5, y: 0.5, z: 0.5};


var shapeTypes = [
    "none",
    "box",
    "sphere",
    "compound",
    "simple-hull",
    "simple-compound",
    "static-mesh"
];

function getTileColor(a, b, c) {
    var offset = (Math.abs(a) + ((Math.abs(b) + (Math.abs(c) % 2)) %  2)) % 2;
    var intensity = (1 - offset) * 128 + offset * 255;
    return { red: intensity * (a % 4), green: intensity, blue: intensity * (b % 4) };
}

function addObject(a, b, c, lifetime) {
    var center = Vec3.sum(stageTileRoot, Vec3.multiply(a, stageAxisA));
    center = Vec3.sum(center, Vec3.multiply(b, stageAxisB));
    center = Vec3.sum(center, Vec3.multiply(c, stageAxisC));                                           

    return (Entities.addEntity({
        type: "Shape",
        shape: "Sphere",
        name: "Backdrop",
        color: getTileColor(a, b, c),
        position: center,    
        rotation: stageOrientation,    
        dimensions: OBJECT_DIM,
        lifetime: (lifetime === undefined) ? DEFAULT_LIFETIME : lifetime,
        shapeType:shapeTypes[2],
        dynamic: true,
        gravity:{"x":0,"y":-9.8,"z":0},
        velocity:{"x":0,"y":0.02,"z":0},
        restitution:0.70,
        friction:0.001,
        damping:0.001,

    }));
}

function addObjectGrid(backdrop, lifetime) {
    for (i = BACKDROP_HALFSIZE; i > -BACKDROP_HALFSIZE; i--) {
        for (j = -BACKDROP_HALFSIZE; j < BACKDROP_HALFSIZE; j++) {
            backdrop.push(addObject(i, j, BACKDROP_MIN_C + 2, lifetime));
        }
    }

}

function addFloor(lifetime) {
    var floorDim = { x:BACKDROP_SIZE * TILE_DIM.x, y:TILE_DIM.y, z:BACKDROP_SIZE *TILE_DIM.x};
    var center = getStagePosOriAt(0, 0, -1).pos;

    return (Entities.addEntity({
        type: "Shape",
        shape: "Cube",
        name: "Floor",
        color: { red: 20, green: 20, blue: 40 },
        position: center,
        rotation: stageOrientation,    
        dimensions: floorDim,
        lifetime: (lifetime === undefined) ? DEFAULT_LIFETIME : lifetime,

        shapeType:shapeTypes[1],
       // dynamic: true,
      //  gravity:{"x":0,"y":-9.8,"z":0},
       // velocity:{"x":0,"y":0.01,"z":0},
        restitution:0.999,
        friction:0.000,
        damping:0.0,

    })); 
}

function addZone(hasKeyLight, hasAmbient, lifetime) {
    var zoneDim = Vec3.multiply(BACKDROP_SIZE, TILE_DIM);
    var center = getStagePosOriAt(0, 0, 0).pos;
    
    var lightDir = Vec3.normalize(Vec3.sum(Vec3.multiply(-1, Quat.getUp(stageOrientation)), Vec3.multiply(-1, Quat.getRight(stageOrientation))))

    return (Entities.addEntity({
        type: "Zone",
        name: "Backdrop zone",
  
        position: center,    
        rotation: stageOrientation,    
        dimensions: zoneDim,
        lifetime: (lifetime === undefined) ? DEFAULT_LIFETIME : lifetime,

        keyLightMode: "enabled",
        skyboxMode: "enabled",
        ambientLightMode: "enabled",

        keyLight:{
            intensity: 0.8 * hasKeyLight,
            direction: {
                "x": 0.037007175385951996,
                "y": -0.7071067690849304,
                "z": -0.7061376571655273
            },
            castShadows: true,
        },
        ambientLight: {
            ambientIntensity: 1.0 * hasAmbient,
            ambientURL: "https://github.com/highfidelity/hifi_tests/blob/master/assets/skymaps/Sky_Day-Sun-Mid-photo.ktx?raw=true",
        },

        hazeMode:"disabled",
        backgroundMode:"skybox",
        skybox:{
            color: {"red":2,"green":2,"blue":2}, // Dark grey background
            skyboxURL: "https://github.com/highfidelity/hifi_tests/blob/master/assets/skymaps/Sky_Day-Sun-Mid-photo.ktx?raw=true",
        }
    }));
}

function addTestScene(name, lifetime) {
    var scene = [];
    scene.push(addFloor(lifetime));
    scene.push(addZone(true, true,  lifetime));

    addObjectGrid(scene, lifetime);
    
    return scene;
}



// Stage position and orientation initialised at setup
stageOrientation = Quat.fromPitchYawRollDegrees(0.0, 0.0, 0.0);
stageRoot = {"x":0.0,"y":0.0,"z":0.0};
stageTileRoot = {"x":0.0,"y":0.0,"z":0.0};
stageAxisA = Vec3.multiply(TILE_UNIT, Quat.getForward(stageOrientation));
stageAxisB = Vec3.multiply(TILE_UNIT, Quat.getRight(stageOrientation));
stageAxisC = Vec3.multiply(TILE_UNIT, Quat.getUp(stageOrientation));

setupScene = function (lifetime) {
    MyAvatar.orientation = Quat.fromPitchYawRollDegrees(0.0, 0.0, 0.0);
    var orientation = MyAvatar.orientation;
    orientation = Quat.safeEulerAngles(orientation);
    orientation.x = 0;
    orientation = Quat.fromVec3Degrees(orientation);

    stageOrientation = orientation;
    stageAxisA = Vec3.multiply(TILE_UNIT, Quat.getForward(stageOrientation));
    stageAxisB = Vec3.multiply(TILE_UNIT, Quat.getRight(stageOrientation));
    stageAxisC = Vec3.multiply(TILE_UNIT, Quat.getUp(stageOrientation));   

    stageRoot = Vec3.sum(MyAvatar.position, Vec3.multiply(-ROOT_Z_OFFSET, Quat.getForward(orientation)));
    stageRoot = Vec3.sum(stageRoot, Vec3.multiply(ROOT_Y_OFFSET, Quat.getUp(orientation)));
    stageTileRoot = Vec3.sum(stageRoot, GRID_TILE_OFFSET);

    return addTestScene("Physics_stage_backdrop", lifetime);
}

getStagePosOriAt = function (a, b, c) {    
    var center = Vec3.sum(stageRoot, Vec3.multiply(a, stageAxisA));
    center = Vec3.sum(center, Vec3.multiply(b, stageAxisB));
    center = Vec3.sum(center, Vec3.multiply(c, stageAxisC));                                           

    return { "pos": center, "ori": stageOrientation};
}


var scene = []

createScene = function() {
    clearScene();
    scene = setupScene();
}

clearScene = function() {
    for (var i = 0; i < scene.length; i++) {
        Entities.deleteEntity(scene[i]);
    }
}

changeResolution = function(res) {
    GRID_WORLD_RESOLUTION = res;

    BACKDROP_SIZE = GRID_WORLD_SIZE / GRID_WORLD_RESOLUTION;
    BACKDROP_HALFSIZE = BACKDROP_SIZE *0.5;
   
    TILE_UNIT = GRID_WORLD_RESOLUTION;  
}
// clean up after test
Script.scriptEnding.connect(function () {
    clearScene()
});