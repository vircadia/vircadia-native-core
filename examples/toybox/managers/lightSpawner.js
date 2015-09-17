var light = Entities.addEntity({
    type: "light",
    position: {x: 544, y: 498.9, z: 506.7},
    intensity: 10,
    dimensions: {x: 10, y: 10, z: 10},
    color: {red: 200, green : 10, blue: 200}
});


function cleanup() {
 Entities.deleteEntity(light);
}

Script.scriptEnding.connect(cleanup);
