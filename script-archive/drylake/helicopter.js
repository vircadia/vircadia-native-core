var modelURL = "https://s3.amazonaws.com/hifi-public/eric/models/helicopter.fbx?v3";
var animationURL = "https://s3.amazonaws.com/hifi-public/eric/models/bladeAnimation.fbx?v7";
var spawnPosition = {
    x: 1031,
    y: 145,
    z: 1041
};

var speed = 0;

var helicopterSound = SoundCache.getSound("https://hifi-public.s3.amazonaws.com/ryan/helicopter.L.wav");
var audioInjector = Audio.playSound(helicopterSound, {
    volume: 0.3,
    loop: true
});

// These constants define the Spotlight position and orientation relative to the model 
var MODEL_LIGHT_POSITION = {
    x: 2,
    y: 0,
    z: -5
};
var MODEL_LIGHT_ROTATION = Quat.angleAxis(-90, {
    x: 1,
    y: 0,
    z: 0
});

// Evaluate the world light entity positions and orientations from the model ones
function evalLightWorldTransform(modelPos, modelRot) {

    return {
        p: Vec3.sum(modelPos, Vec3.multiplyQbyV(modelRot, MODEL_LIGHT_POSITION)),
        q: Quat.multiply(modelRot, MODEL_LIGHT_ROTATION)
    };
}


var helicopter = Entities.addEntity({
    type: "Model",
    name: "Helicopter",
    modelURL: modelURL,
    animation: {
        url: animationURL,
        running: true,  
        fps: 180

    },
    dimensions: {
        x: 12.13,
        y: 3.14,
        z: 9.92
    },
    position: spawnPosition,
});



var spotlight = Entities.addEntity({
    type: "Light",
    name: "helicopter light",
    intensity: 2,
    color: {
        red: 200,
        green: 200,
        blue: 255
    },
    intensity: 1,
    dimensions: {
        x: 2,
        y: 2,
        z: 200
    },
    exponent: 0.01,
    cutoff: 10,
    isSpotlight: true
});

var debugLight = Entities.addEntity({
    type: "Box",
    dimensions: {
        x: .1,
        y: .1,
        z: .3
    },
    color: {
        red: 200,
        green: 200,
        blue: 0
    }
});

function cleanup() {
    Entities.deleteEntity(debugLight);
    Entities.deleteEntity(helicopter);
    Entities.deleteEntity(spotlight);

}

function update() {
    var modelProperties = Entities.getEntityProperties(helicopter, ['position', 'rotation']);
    var lightTransform = evalLightWorldTransform(modelProperties.position, modelProperties.rotation);
    Entities.editEntity(spotlight, {
        position: lightTransform.p,
        rotation: lightTransform.q
    });
    Entities.editEntity(debugLight, {
        position: lightTransform.p,
        rotation: lightTransform.q
    });

    audioInjector.setOptions({
        position: modelProperties.position,
    });

    //Move forward 
    var newRotation = Quat.multiply(modelProperties.rotation, {
        x: 0,
        y: .002,
        z: 0,
        w: 1
    })
    var newPosition = Vec3.sum(modelProperties.position, Vec3.multiply(speed, Quat.getFront(modelProperties.rotation)));
    Entities.editEntity(helicopter, {
        position: newPosition,
        rotation: newRotation
    })
}


Script.update.connect(update);
Script.scriptEnding.connect(cleanup);