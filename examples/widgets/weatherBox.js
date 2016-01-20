Script.include("../libraries/utils.js");
var weatherBox, boxDimensions;
var orientation = Camera.getOrientation();
orientation = Quat.safeEulerAngles(orientation);
orientation.x = 0;
orientation = Quat.fromVec3Degrees(orientation);
var center = Vec3.sum(MyAvatar.position, Vec3.multiply(3, Quat.getFront(orientation)));

var emitters = [];
var spheres = [];

var bolt;
var raveRoomModelURL = "https://s3-us-west-1.amazonaws.com/hifi-content/eric/models/RaveRoom.fbx";
var roomDimensions = {
    x: 30.58,
    y: 15.29,
    z: 30.58
};

var raveRoom = Entities.addEntity({
    type: "Model",
    name: "Rave Hut Room",
    modelURL: raveRoomModelURL,
    position: center,
    dimensions: roomDimensions,
    visible: true
});


var weatherZone = Entities.addEntity({
    type: "Zone",
    name: "Weather Zone",
    shapeType: 'box',
    keyLightIntensity: 1,
    keyLightColor: {
        red: 50,
        green: 0,
        blue: 50
    },
    keyLightAmbientIntensity: 1,
    position: MyAvatar.position,
    dimensions: {
        x: 100,
        y: 100,
        z: 100
    }
});
createWeatherBox(center);
center.y += boxDimensions.y / 2;
createCloud(center);
createLightningStrike(center)
createLightningStrike(center)
createLightningStrike(center)
createLightningStrike(center)
createLightningStrike(center)
createLightningStrike(center)
createLightningStrike(center)
createLightningStrike(center)
createLightningStrike(center)


function createLightningStrike(basePosition) {
    var normal = Vec3.subtract(position, MyAvatar.position);
    normal.y = 0;
    var textureURL = "http://localhost:8080/lightning.png"
    var linePoints = [];
    var normals = [];
    var strokeWidths = [];
    var strokeWidth = 0.02
    var currentPointPosition = {x: 0, y: 0, z: 0};
    for (var i = 0; i < 8; i++) {
        linePoints.push(currentPointPosition);
        currentPointPosition = Vec3.sum(currentPointPosition, {x: randFloat(-0.07, 0.07), y: randFloat(-0.1, -0.05), z: randFloat(-0.07, 0.07)});
        linePoints.push(currentPointPosition);
        normals.push(normal);
        normals.push(normal);
        strokeWidth -= 0.002;
        strokeWidths.push(strokeWidth);
        strokeWidths.push(strokeWidth);
    }
    var position = Vec3.sum(basePosition, {x: randFloat(-boxDimensions.x/8, boxDimensions.x/8), y: 0, z: randFloat(-boxDimensions.x/8, boxDimensions.x/8)});
    bolt = Entities.addEntity({
        type: "PolyLine",
        textures: textureURL,
        position: position,
        dimensions: {
            x: 10,
            y: 10,
            z: 10
        },
        linePoints: linePoints,
        normals: normals,
        strokeWidths: strokeWidths,
        lifetime: randFloat(0.01, 0.2)
    });

    Script.setTimeout(function() {
        createLightningStrike(position)
    }, randInt(500, 5000));

}


function createWeatherBox(position) {
    var naturalDimensions = {
        x: 1.11,
        y: 1.3,
        z: 1.11
    };
    boxDimensions = Vec3.multiply(naturalDimensions, 0.7);
    var modelURL = "http://localhost:8080/weatherBox.fbx?v1" + Math.random();
    // var modelURL = "https://s3-us-west-1.amazonaws.com/hifi-content/eric/models/raveStick.fbx";
    weatherBox = Entities.addEntity({
        type: "Model",
        name: "Weather Box",
        modelURL: modelURL,
        position: position,
        dimensions: boxDimensions
    });
}


function createCloud(position) {
    var props = {
        "type": "ParticleEffect",
        "position": position,
        "isEmitting": true,
        "maxParticles": 10000,
        "lifespan": 3,
        "emitRate": 2000,
        "emitSpeed": 0.025,
        "speedSpread": 0,
        "emitDimensions": {
            x: boxDimensions.x,
            y: boxDimensions.x,
            z: 0.1
        },
        "emitRadiusStart": 1,
        "polarStart": 0,
        "polarFinish": 1.570796012878418,
        "azimuthStart": -3.1415927410125732,
        "azimuthFinish": 3.1415927410125732,
        "emitAcceleration": {
            "x": 0,
            "y": 0,
            "z": 0
        },
        "particleRadius": 0.04,
        "radiusSpread": 0.02,
        "radiusStart": 0.04,
        radiusFinish: 0.04,
        "colorStart": {
            "red": 40,
            "green": 40,
            "blue": 100
        },
        color: {
            red: 40,
            green: 40,
            blue: 100
        },
        "colorFinish": {
            "red": 40,
            "green": 40,
            "blue": 100
        },
        "alpha": 1,
        "alphaSpread": 0,
        "alphaStart": 0.3,
        "alphaFinish": 0,
        "emitterShouldTrail": true,
        "textures": "https://hifi-public.s3.amazonaws.com/alan/Particles/Particle-Sprite-Smoke-1.png"
    }

    var oceanEmitter = Entities.addEntity(props);
    emitters.push(oceanEmitter);
}



function cleanup() {
    emitters.forEach(function(emitter) {
        Entities.deleteEntity(emitter);
    });
    spheres.forEach(function(sphere) {
        Entities.deleteEntity(sphere);
    });
    Entities.deleteEntity(weatherBox);
    Entities.deleteEntity(weatherZone);

    Entities.deleteEntity(raveRoom);
    Entities.deleteEntity(bolt);
}

Script.scriptEnding.connect(cleanup);