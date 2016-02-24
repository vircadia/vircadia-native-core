var fishtank;

var TANK_DIMENSIONS = {
    x: 1.3393,
    y: 1.3515,
    z: 3.5914
};

var TANK_WIDTH = TANK_DIMENSIONS.z;
var TANK_HEIGHT = TANK_DIMENSIONS.y;

var DEBUG_COLOR = {
    red: 255,
    green: 0,
    blue: 255
}

var center = Vec3.sum(MyAvatar.position, Vec3.multiply(Quat.getFront(MyAvatar.orientation), 2 * TANK_WIDTH));

var TANK_POSITION = center;

var TANK_SCRIPT = Script.resolvePath('tank.js?' + Math.random())

var TANK_MODEL_URL = "http://hifi-content.s3.amazonaws.com/DomainContent/Home/fishTank/aquarium-6.fbx";

function createFishTank() {
    var tankProperties = {
        name: 'hifi-home-fishtank',
        type: 'Model',
        modelURL:TANK_MODEL_URL,
        dimensions: TANK_DIMENSIONS,
        position: TANK_POSITION,
        color: DEBUG_COLOR,
        collisionless: true,
        script: TANK_SCRIPT,
        userData: JSON.stringify({
            'hifi-home-fishtank': {
                fishLoaded: false
            },
            grabbableKey: {
                grabbable: false
            }
        }),
        visible:true
    }

    fishTank = Entities.addEntity(tankProperties);
}

function cleanup() {
    Entities.deleteEntity(fishTank);
}

createFishTank();

Script.scriptEnding.connect(cleanup);