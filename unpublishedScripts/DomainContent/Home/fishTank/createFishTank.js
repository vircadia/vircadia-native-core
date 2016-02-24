var fishtank;

var TANK_DIMENSIONS = {
    x: 1,
    y: 1,
    z: 2
};

var TANK_WIDTH = 3.0;
var TANK_HEIGHT = 1.0;

var DEBUG_COLOR = {
    red: 255,
    green: 0,
    blue: 255
}

var center = Vec3.sum(MyAvatar.position, Vec3.multiply(Quat.getFront(MyAvatar.orientation), 2 * TANK_WIDTH));

var TANK_POSITION = center;

var TANK_SCRIPT = Script.resolvePath('tank.js?' + Math.random())

function createFishTank() {
    var tankProperties = {
        name: 'hifi-home-fishtank',
        type: 'Box',
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
        visible:false
    }

    fishTank = Entities.addEntity(tankProperties);
}

function cleanup() {
    Entities.deleteEntity(fishTank);
}

createFishTank();

Script.scriptEnding.connect(cleanup);