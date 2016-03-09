var SHOULD_CLEANUP = true;

var WHITE = {
    red: 255,
    green: 255,
    blue: 255
};

var RED = {
    red: 255,
    green: 255,
    blue: 255
};

var GREEN = {
    red: 255,
    green: 255,
    blue: 255
};

var BLUE = {
    red: 255,
    green: 255,
    blue: 255
};

var center = Vec3.sum(Vec3.sum(MyAvatar.position, {
    x: 0,
    y: 0.5,
    z: 0
}), Vec3.multiply(1, Quat.getFront(Camera.getOrientation())));

var BASE_DIMENSIONS = {
    x: 0.5,
    y: 0.5,
    z: 0.5
};

var BASE_POSITION = center;

var LID_OFFSET = {
    x: 0,
    y: BASE_DIMENSIONS.y / 2,
    z: 0
};

var LID_DIMENSIONS = {
    x: BASE_DIMENSIONS.x,
    y: BASE_DIMENSIONS.y / 8,
    z: BASE_DIMENSIONS.z
};

var base, lid;

function createBase() {
    var baseProperties = {
        name: 'hifi-home-music-box-base',
        type: 'Box',
        color: WHITE,
        position: BASE_POSITION
    }

    base = Entities.addEntity(base);
};

function createLid() {

    var lidPosition = Vec3.sum(lidPosition, LID_OFFSET);
    var lidProperties = {
        name: 'hifi-home-music-box-lid',
        type: 'Box',
        color: BLUE,
        dimensions: LID_DIMENSIONS,
        position: lidPosition,
        userData:JSON.stringify({
            grabConstraintsKey:{
                positionLocked:true,
                rotationLocked:false,
                positionMod:false,
                rotationMod:{
                    pitch:{
                        min:0,
                        max:100
                    },
                    yaw:false,
                    roll:false
                }
            }
        })
    }

    lid = Entities.addEntity(lidProperties);
};

if (SHOULD_CLEANUP === true) {
    Script.scriptEnding.connect(cleanup);
}

function cleanup() {
    Entities.deleteEntity(base);
    Entities.deleteEntity(lid);
}

createBase();
createLid();