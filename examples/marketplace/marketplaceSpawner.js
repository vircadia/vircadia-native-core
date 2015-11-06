var floorPosition = Vec3.sum(MyAvatar.position, Vec3.multiply(3, Quat.getFront(Camera.getOrientation())));;
floorPosition.y = MyAvatar.position.y - 5;

var modelsToLoad = [
    {
        lowURL: "https://s3.amazonaws.com/hifi-public/ozan/3d_marketplace/sets/tuscany/tuscany_low.fbx",
        highURL: "https://s3.amazonaws.com/hifi-public/ozan/3d_marketplace/sets/tuscany/tuscany_hi.fbx"
    }
];

var models = [];

var floor = Entities.addEntity({
    type: "Model",
    modelURL: "https://hifi-public.s3.amazonaws.com/ozan/3d_marketplace/props/floor/3d_mp_floor.fbx",
    position: floorPosition,
    shapeType: 'box',
    dimensions: {x: 1000, y: 9, z: 1000} 
});

//Create grid
var modelParams = {
    type: "Model",
    shapeType: "box",
    dimensions: {x: 53, y: 15.7, z: 44.8},
    velocity: {x: 0, y: -1, z: 0},
    gravity: {x: 0, y: -1, z: 0},
    collisionsWillMove: true    
};

var modelPosition = {x: floorPosition.x + 10, y: floorPosition.y + 15, z: floorPosition.z};
for (var i = 0; i < modelsToLoad.length; i++) {
    modelParams.modelURL = modelsToLoad[i].lowURL;
    modelPosition.z -= 10;
    modelParams.position = modelPosition;
    var model = Entities.addEntity(modelParams);
    models.push(model);
}




function cleanup() {
    Entities.deleteEntity(floor);
    models.forEach(function(model) {
        Entities.deleteEntity(model);
    });
}

Script.scriptEnding.connect(cleanup);