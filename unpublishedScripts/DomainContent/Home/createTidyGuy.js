//v1.0
var center = Vec3.sum(Vec3.sum(MyAvatar.position, {
    x: 0,
    y: 0.5,
    z: 0
}), Vec3.multiply(2, Quat.getFront(Camera.getOrientation())));

var SCRIPT_URL = Script.resolvePath('reset.js?' + Math.random());

function createTidyGuy() {
    var properties = {
        type: 'Model',
        modelURL: 'http://hifi-content.s3.amazonaws.com/DomainContent/Home/tidyGuy/Tidyguy-6.fbx',
        dimensions: {
            x: 0.32,
            y: 0.96,
            z: 0.6844
        },
        position: center,
        script: SCRIPT_URL,
        dynamic:false,
        userData:JSON.stringify({
            grabbableKey:{
                wantsTrigger:true
            }
        })
    }

    return Entities.addEntity(properties);
}

var tidyGuy = createTidyGuy();

Script.scriptEnding.connect(function() {
    Entities.deleteEntity(tidyGuy);
})