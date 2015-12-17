Script.include("../../libraries/utils.js");
var modelURL = "file:///C:/Users/Eric/Desktop/raveStick.fbx?v1" + Math.random();
var scriptURL = Script.resolvePath("raveStickEntityScript.js");
RaveStick = function(spawnPosition) {

    var stick = Entities.addEntity({
        type: "Model",
        modelURL: modelURL,
        position: spawnPosition,
        shapeType: 'box',
        dimensions: {
            x: 0.06,
            y: 0.06,
            z: 0.8
        },
        userData: JSON.stringify({
            grabbableKey: {
                spatialKey: {
                    relativePosition: {
                        x: 0,
                        y: 0,
                        z: -0.4
                    },
                    relativeRotation: Quat.fromPitchYawRollDegrees(90, 90, 0)
                },
                invertSolidWhileHeld: true
            }
        })
    });


    function cleanup() {
        Entities.deleteEntity(stick);
    }

    this.cleanup = cleanup;
}