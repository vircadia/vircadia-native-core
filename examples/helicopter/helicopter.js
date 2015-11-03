var modelURL = "https://s3.amazonaws.com/hifi-public/eric/models/helicopter.fbx?v3";
var center = Vec3.sum(MyAvatar.position, Vec3.multiply(2, Quat.getFront(Camera.getOrientation())));

// These constants define the Spotlight position and orientation relative to the model 
var MODEL_LIGHT_POSITION = {
    x: 2,
    y: 0,
    z: -5
};
var MODEL_LIGHT_ROTATION = Quat.angleAxis(0,  {
    x: 0,
    y: 0,
    z: 1
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
    dimensions: {x: 12.13, y: 3.14, z: 9.92},
    // rotation: Quat.fromPitchYawRollDegrees(0, -90, 0),
    position: center,
    collisionsWillMove: true
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
    intensity: 10,
    dimensions: {
        x: 2,
        y: 2,
        z: 200
    },
    exponent: .1,
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
         // rotation: lightTransform.q
    });
      Entities.editEntity(debugLight, {
         position: lightTransform.p,
         rotation: lightTransform.q
    });
}


Script.update.connect(update);
Script.scriptEnding.connect(cleanup);