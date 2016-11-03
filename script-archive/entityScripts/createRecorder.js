var PARAMS_SCRIPT_URL = Script.resolvePath('recordingEntityScript.js');
var rotation = Quat.safeEulerAngles(Camera.getOrientation());
rotation = Quat.fromPitchYawRollDegrees(0, rotation.y, 0);
var center = Vec3.sum(MyAvatar.position, Vec3.multiply(6, Quat.getFront(rotation)));

var recordAreaEntity = Entities.addEntity({
    name: 'recorderEntity',
    dimensions: {
        x: 10,
        y: 10,
        z: 10
    },
    type: 'Box',
    position: center,
    color: {
        red: 255,
        green: 255,
        blue: 255
    },
    visible: true,
    script: PARAMS_SCRIPT_URL,
    collisionless: true,
});
