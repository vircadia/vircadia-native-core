var TEST_MODEL_URL = '';

var doppelgangers = [];

function Doppelganger(avatar) {
    this.initialProperties = {
        name: 'Hifi-Doppelganger',
        type: 'Model',
        modelURL: TEST_MODEL_URL,
        dimensions: getAvatarDimensions(avatar),
        position: putDoppelgangerAcrossFromAvatar(this, avatar),
        rotation: rotateDoppelgangerTowardAvatar(this, avatar),
    };

    this.id = createDoppelgangerEntity(this);
    this.avatar = avatar;
    return this;
}

function getJointData() {
    return jointData;
}

function setJointData(doppelganger, jointData) {
    return true;
}

function mirrorJointData() {
    return mirroredJointData;
}

function createDoppelganger(avatar) {
    return new Doppelganger(avatar);
}

function createDoppelgangerEntity(doppelganger) {
    return Entities.addEntitiy(doppelganger.initialProperties);
}

function putDoppelgangerAcrossFromAvatar(doppelganger, avatar) {
    return position;
}

function getAvatarDimensions(avatar) {
    return dimensions;
}

function rotateDoppelgangerTowardAvatar(doppelganger, avatar) {
    return rotation;
}

function connectDoppelgangerUpdates() {
    Script.update.connect(updateDoppelganger);
}

function disconnectDoppelgangerUpdates() {
    Script.update.disconnect(updateDoppelganger);
}

function updateDoppelganger() {
    doppelgangers.forEach(function(doppelganger) {
        var joints = getJointData(doppelganger.avatar.id);
        var mirroredJoints = mirrorJointData(joints);
        setJointData(doppelganger, mirroredJoints);
    });

}

function makeDoppelgangerForMyAvatar() {
    var doppelganger = createDoppelganger(MyAvatar);
    doppelgangers.push(doppelganger);
    connectDoppelgangerUpdates();
}

function cleanup() {
    disconnectDoppelgangerUpdates();

    doppelgangers.forEach(function(doppelganger) {
        Entities.deleteEntity(doppelganger);
    });

}

Script.scriptEnding.connect(cleanup);