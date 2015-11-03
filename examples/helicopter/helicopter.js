var modelURL = "https://s3.amazonaws.com/hifi-public/eric/models/helicopter.fbx";
var center = Vec3.sum(MyAvatar.position, Vec3.multiply(3, Quat.getFront(Camera.getOrientation())));

// These constants define the Spotlight position and orientation relative to the model 
var MODEL_LIGHT_POSITION = {
    x: -2,
    y: 0,
    z: 0
};
var MODEL_LIGHT_ROTATION = Quat.angleAxis(-90, {
    x: 0,
    y: 1,
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
    position: center
});

var modelProperties = Entities.getEntityProperties(helicopter, ['position', 'rotation']);
var lightTransform = evalLightWorldTransform(modelProperties.position, modelProperties.rotation);


var spotlight = Entities.addEntity({
    type: "Light",
    name: "helicopter light",
    position: lightTransform.p,
    rotation: lightTransform.q,
    intensity: 2,
    color: {red: 200, green: 200, blue: 255},
    intensity: 2,
    exponent: 0.3,
    cutoff: 20
});

var debugLight = Entities.addEntity({
    type: "Box",
    position: lightTransform.p,
    rotation: lightTransform.q,
    dimensions: {x: 4, y: 1, z: 1},
    color: {red: 200, green: 200, blue: 0}
});

function cleanup() {
    Entities.deleteEntity(debugLight);
    Entities.deleteEntity(helicopter);
    Entities.deleteEntity(spotlight);

}

Script.scriptEnding.connect(cleanup);