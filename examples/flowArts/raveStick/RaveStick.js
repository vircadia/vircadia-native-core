Script.include("../../libraries/utils.js");
var modelURL = "file:///C:/Users/Eric/Desktop/raveStick.fbx?v1" + Math.random();

RaveStick = function(spawnPosition) {

    var stick = Entities.addEntity({
        type: "Model",
        modelURL: modelURL,
        position: spawnPosition,
        shapeType: 'box',
        userData: JSON.stringify({
            grabbableKey: {
                invertSolidWhileHeld: true
            }
        })
    });


    function cleanup() {
        Entities.deleteEntity(stick);
    }

    this.cleanup = cleanup;
}