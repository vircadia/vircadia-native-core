
    var DEFAULT_LIFETIME = 120;
    var GRID_WORLD_SIZE = 100.0;
    var GRID_WORLD_MARGIN = 5.0;
    var GRID_WORLD_RESOLUTION = 30.0;
    var GRID_WORLD_DROP_HEIGHT = 4.0;

    var GRID_SIZE = GRID_WORLD_RESOLUTION;
    var GRID_HALFSIZE = GRID_SIZE *0.5;

    var ROOT_Z_OFFSET = -3;
    var ROOT_Y_OFFSET = -0.1;

    var TILE_UNIT = GRID_WORLD_SIZE / GRID_SIZE;
    var TILE_DIM = { x: TILE_UNIT, y: TILE_UNIT, z: TILE_UNIT};
    var GRID_TILE_OFFSET = Vec3.multiply(0.5, TILE_DIM);

    var GRID_DROP_C = GRID_WORLD_DROP_HEIGHT / TILE_UNIT;

    function updateWorldSizeAndResolution(size, res) {
        GRID_WORLD_SIZE = size
        GRID_WORLD_RESOLUTION = res;

        GRID_SIZE = GRID_WORLD_RESOLUTION;
        GRID_HALFSIZE = GRID_SIZE *0.5;
        
        TILE_UNIT = GRID_WORLD_SIZE / GRID_SIZE;
        TILE_DIM = { x: TILE_UNIT, y: TILE_UNIT, z: TILE_UNIT};
        GRID_TILE_OFFSET = Vec3.multiply(0.5, TILE_DIM);

        GRID_DROP_C = GRID_WORLD_DROP_HEIGHT / TILE_UNIT;
        print("TILE_UNIT = " + TILE_UNIT)    
        print("GRID_DROP_C = " + GRID_DROP_C)    
    }

    var OBJECT_DIM = { x: 0.5, y: 0.5, z: 0.5};
    var CONE_DIM = { x: 0.3104, y: 0.3336, z: 0.3104};
    var OBJECT_SPIN = { x: 0.5, y: 0.5, z: 0.5};

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
    var center = getStagePosOriAt(a, b, c).pos;
    var offset = (Math.abs(a) + (Math.abs(b) % 2)) %  2;                                         
    var makePrim = (offset == 0 ? true : false) ;
    if (makePrim == true) {
        return (Entities.addEntity({
            type: "Shape",
            shape: "Sphere",
            name: "object-" + a + b + c,
            color: getTileColor(a, b, c),
            position: center,    
            rotation: stageOrientation,    
            dimensions: OBJECT_DIM,
            lifetime: (lifetime === undefined) ? DEFAULT_LIFETIME : lifetime,
            shapeType:shapeTypes[2],
            dynamic: true,
            gravity:{"x":0,"y":-9.8,"z":0},
            velocity:{"x":0,"y":0.02,"z":0},
            angularVelocity:OBJECT_SPIN,
            restitution:0.70,
            friction:0.01,
            damping:0.001,

        }));
    } else {
        return (Entities.addEntity({
            type: "Model",
            name: "object-" + a + b + c,
            position: center,    
            rotation: stageOrientation,    
            dimensions: OBJECT_DIM,
            lifetime: (lifetime === undefined) ? DEFAULT_LIFETIME : lifetime,
            modelURL: Script.getExternalPath(Script.ExternalPaths.HF_Content, "/jimi/props/cones/trafficCone.fbx"),
            shapeType:shapeTypes[4],
            dynamic: true,
            gravity:{"x":0,"y":-9.8,"z":0},
            velocity:{"x":0,"y":0.02,"z":0},
            angularVelocity:OBJECT_SPIN,
            restitution:0.70,
            friction:0.01,
            damping:0.01,

        }));        
    }
}

function addObjectGrid(backdrop, lifetime) {
    for (i = GRID_HALFSIZE; i > -GRID_HALFSIZE; i--) {
        for (j = -GRID_HALFSIZE; j < GRID_HALFSIZE; j++) {
            backdrop.push(addObject(i, j, GRID_DROP_C, lifetime));
        }
    }

}

function addFloor(lifetime) {
    var floorDim = { x:GRID_WORLD_SIZE + 2 * GRID_WORLD_MARGIN, y: TILE_DIM.y, z:GRID_WORLD_SIZE + 2 * GRID_WORLD_MARGIN};
    var center = getStagePosOriAt(0, 0, -0.5).pos;

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
        friction:0.001,
        damping:0.3,

    })); 
}

function addZone(hasKeyLight, hasAmbient, lifetime) {
    var zoneDim = { x:GRID_WORLD_SIZE + 2 * GRID_WORLD_MARGIN, y:GRID_WORLD_SIZE, z:GRID_WORLD_SIZE + 2 * GRID_WORLD_MARGIN};
    var center = getStagePosOriAt(0, 0, -1).pos;
    
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
   // scene.push(addFloor(lifetime));
  //  scene.push(addZone(true, true,  lifetime));

    addObjectGrid(scene, lifetime);
    
    return scene;
}



// Stage position and orientation initialised at setup
stageOrientation = Quat.fromPitchYawRollDegrees(0.0, 0.0, 0.0);
stageRoot = {"x":0.0,"y":0.0,"z":0.0};
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
    updateWorldSizeAndResolution(GRID_WORLD_SIZE, res);
}

getResolution = function() {
    return GRID_WORLD_RESOLUTION;
}

changeSize = function(size) {
    updateWorldSizeAndResolution(size, GRID_WORLD_RESOLUTION);
}

getSize = function() {
    return GRID_WORLD_SIZE;
}


getNumObjects = function() {
    return GRID_SIZE * GRID_SIZE;
}

bumpUpFloor = function() {
    print("bumpUpFloor")
    if (scene.length > 0) {
        Entities.editEntity(scene[0],{ velocity: {x: 0, y:-2.0,z: 0}})
    }
}