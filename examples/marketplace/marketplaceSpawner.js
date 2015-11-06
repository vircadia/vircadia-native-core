var floorPosition = Vec3.sum(MyAvatar.position, Vec3.multiply(3, Quat.getFront(Camera.getOrientation())));;
floorPosition.y = MyAvatar.position.y - 5;

Script.include('../libraries/utils.js');

var entityScriptURL = Script.resolvePath("modelSwap.js");

var modelsToLoad = [{
    lowURL: "https://s3.amazonaws.com/hifi-public/ozan/3d_marketplace/sets/dojo/dojo_low.fbx",
    highURL: "https://s3.amazonaws.com/hifi-public/ozan/3d_marketplace/sets/dojo/dojo_hi.fbx"
}, {
    lowURL: "https://s3.amazonaws.com/hifi-public/ozan/3d_marketplace/sets/tuscany/tuscany_low.fbx",
    highURL: "https://s3.amazonaws.com/hifi-public/ozan/3d_marketplace/sets/tuscany/tuscany_hi.fbx"
}];

var models = [];

var floor = Entities.addEntity({
    type: "Model",
    modelURL: "https://hifi-public.s3.amazonaws.com/ozan/3d_marketplace/props/floor/3d_mp_floor.fbx",
    position: floorPosition,
    shapeType: 'box',
    dimensions: {
        x: 1000,
        y: 9,
        z: 1000
    }
});

//Create grid
var modelParams = {
    type: "Model",
    dimensions: {
        x: 31.85,
        y: 7.75,
        z: 54.51
    },
    script: entityScriptURL,
    userData: JSON.stringify({
        grabbableKey: {
            wantsTrigger: true
        }
    })

};

var modelPosition = {
    x: floorPosition.x + 10,
    y: floorPosition.y + 8.5,
    z: floorPosition.z - 30
};
for (var i = 0; i < modelsToLoad.length; i++) {
    modelParams.modelURL = modelsToLoad[i].lowURL;
    modelParams.position = modelPosition;
    var lowModel = Entities.addEntity(modelParams);

    modelParams.modelURL = modelsToLoad[i].highURL;
    modelParams.visible = false;
    modelParams.dimensions = Vec3.multiply(modelParams.dimensions, 0.5);
    var highModel = Entities.addEntity(modelParams);
    models.push({
        low: lowModel,
        high: highModel
    });
    // customKey, id, data
    setEntityCustomData('modelCounterpart', lowModel, {modelCounterpartId: highModel});
    setEntityCustomData('modelCounterpart', highModel, {modelCounterpartId: lowModel});

    modelPosition.z -= 60;
}



function cleanup() {
    Entities.deleteEntity(floor);
    models.forEach(function(model) {
        Entities.deleteEntity(model.low);
        Entities.deleteEntity(model.high);
    });
}

Script.scriptEnding.connect(cleanup);