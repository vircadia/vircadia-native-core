//
//  doppelganger.js
//
//  Created by James B. Pollack @imgntn on 12/28/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  This script shows how to hook up a model entity to your avatar to act as a doppelganger.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  To-Do:  mirror joints, rotate avatar fully, automatically get avatar fbx, make sure dimensions for avatar are right when u bring it in

var TEST_MODEL_URL = 'https://s3.amazonaws.com/hifi-public/ozan/avatars/albert/albert/albert.fbx';

var MIRROR_JOINT_DATA = true;
var MIRRORED_ENTITY_SCRIPT_URL = Script.resolvePath('mirroredEntity.js');
var FREEZE_TOGGLER_SCRIPT_URL = Script.resolvePath('freezeToggler.js?' + Math.random(0, 1000))
var THROTTLE = false;
var THROTTLE_RATE = 100;
var doppelgangers = [];

function Doppelganger(avatar) {
    this.initialProperties = {
        name: 'Hifi-Doppelganger',
        type: 'Model',
        modelURL: TEST_MODEL_URL,
        // dimensions: getAvatarDimensions(avatar),
        position: putDoppelgangerAcrossFromAvatar(this, avatar),
        rotation: rotateDoppelgangerTowardAvatar(this, avatar),
        collisionsWillMove: false,
        ignoreForCollisions: false,
        script: FREEZE_TOGGLER_SCRIPT_URL,
        userData: JSON.stringify({
            grabbableKey: {
                grabbable: false,
                wantsTrigger: true
            }
        })
    };

    this.id = createDoppelgangerEntity(this);
    this.avatar = avatar;
    return this;
}
function getJointData(avatar) {
    var allJointData = [];
    var jointNames = MyAvatar.jointNames;
    jointNames.forEach(function(joint, index) {
        var translation = MyAvatar.getJointTranslation(index);
        var rotation = MyAvatar.getJointRotation(index)
        allJointData.push({
            joint: joint,
            index: index,
            translation: translation,
            rotation: rotation
        });
    });

    return allJointData;
}

function setJointData(doppelganger, relativeXforms) {
    var jointRotations = [];
    var i, l = relativeXforms.length;
    for (i = 0; i < l; i++) {
        jointRotations.push(relativeXforms[i].rot);
    }
    Entities.setAbsoluteJointRotationsInObjectFrame(doppelganger.id, jointRotations);

    return true;
}

// maps joint names to their mirrored joint
var JOINT_MIRROR_NAME_MAP = {
    RightUpLeg: "LeftUpLeg",
    RightLeg: "LeftLeg",
    RightFoot: "LeftFoot",
    LeftUpLeg: "RightUpLeg",
    LeftLeg: "RightLeg",
    LeftFoot: "RightFoot",
    RightShoulder: "LeftShoulder",
    RightArm: "LeftArm",
    RightForeArm: "LeftForeArm",
    RightHand: "LeftHand",
    RightHandThumb1: "LeftHandThumb1",
    RightHandThumb2: "LeftHandThumb2",
    RightHandThumb3: "LeftHandThumb3",
    RightHandThumb4: "LeftHandThumb4",
    RightHandIndex1: "LeftHandIndex1",
    RightHandIndex2: "LeftHandIndex2",
    RightHandIndex3: "LeftHandIndex3",
    RightHandIndex4: "LeftHandIndex4",
    RightHandMiddle1: "LeftHandMiddle1",
    RightHandMiddle2: "LeftHandMiddle2",
    RightHandMiddle3: "LeftHandMiddle3",
    RightHandMiddle4: "LeftHandMiddle4",
    RightHandRing1: "LeftHandRing1",
    RightHandRing2: "LeftHandRing2",
    RightHandRing3: "LeftHandRing3",
    RightHandRing4: "LeftHandRing4",
    RightHandPinky1: "LeftHandPinky1",
    RightHandPinky2: "LeftHandPinky2",
    RightHandPinky3: "LeftHandPinky3",
    RightHandPinky4: "LeftHandPinky4",
    LeftShoulder: "RightShoulder",
    LeftArm: "RightArm",
    LeftForeArm: "RightForeArm",
    LeftHand: "RightHand",
    LeftHandThumb1: "RightHandThumb1",
    LeftHandThumb2: "RightHandThumb2",
    LeftHandThumb3: "RightHandThumb3",
    LeftHandThumb4: "RightHandThumb4",
    LeftHandIndex1: "RightHandIndex1",
    LeftHandIndex2: "RightHandIndex2",
    LeftHandIndex3: "RightHandIndex3",
    LeftHandIndex4: "RightHandIndex4",
    LeftHandMiddle1: "RightHandMiddle1",
    LeftHandMiddle2: "RightHandMiddle2",
    LeftHandMiddle3: "RightHandMiddle3",
    LeftHandMiddle4: "RightHandMiddle4",
    LeftHandRing1: "RightHandRing1",
    LeftHandRing2: "RightHandRing2",
    LeftHandRing3: "RightHandRing3",
    LeftHandRing4: "RightHandRing4",
    LeftHandPinky1: "RightHandPinky1",
    LeftHandPinky2: "RightHandPinky2",
    LeftHandPinky3: "RightHandPinky3",
    LeftHandPinky4: "RightHandPinky4",
    LeftHandPinky: "RightHandPinky",
};

// maps joint names to parent joint names.
var JOINT_PARENT_NAME_MAP = {
    Hips: "",
    RightUpLeg: "Hips",
    RightLeg: "RightUpLeg",
    RightFoot: "RightLeg",
    LeftUpLeg: "Hips",
    LeftLeg: "LeftUpLeg",
    LeftFoot: "LeftLeg",
    Spine: "Hips",
    Spine1: "Spine",
    Spine2: "Spine1",
    Spine3: "Spine2",
    Neck: "Spine3",
    Head: "Neck",
    RightShoulder: "Spine3",
    RightArm: "RightShoulder",
    RightForeArm: "RightArm",
    RightHand: "RightForeArm",
    RightHandThumb1: "RightHand",
    RightHandThumb2: "RightHandThumb1",
    RightHandThumb3: "RightHandThumb2",
    RightHandThumb4: "RightHandThumb3",
    RightHandIndex1: "RightHand",
    RightHandIndex2: "RightHandIndex1",
    RightHandIndex3: "RightHandIndex2",
    RightHandIndex4: "RightHandIndex3",
    RightHandMiddle1: "RightHand",
    RightHandMiddle2: "RightHandMiddle1",
    RightHandMiddle3: "RightHandMiddle2",
    RightHandMiddle4: "RightHandMiddle3",
    RightHandRing1: "RightHand",
    RightHandRing2: "RightHandRing1",
    RightHandRing3: "RightHandRing2",
    RightHandRing4: "RightHandRing3",
    RightHandPinky1: "RightHand",
    RightHandPinky2: "RightHandPinky1",
    RightHandPinky3: "RightHandPinky2",
    RightHandPinky4: "RightHandPinky3",
    LeftShoulder: "Spine3",
    LeftArm: "LeftShoulder",
    LeftForeArm: "LeftArm",
    LeftHand: "LeftForeArm",
    LeftHandThumb1: "LeftHand",
    LeftHandThumb2: "LeftHandThumb1",
    LeftHandThumb3: "LeftHandThumb2",
    LeftHandThumb4: "LeftHandThumb3",
    LeftHandIndex1: "LeftHand",
    LeftHandIndex2: "LeftHandIndex1",
    LeftHandIndex3: "LeftHandIndex2",
    LeftHandIndex4: "LeftHandIndex3",
    LeftHandMiddle1: "LeftHand",
    LeftHandMiddle2: "LeftHandMiddle1",
    LeftHandMiddle3: "LeftHandMiddle2",
    LeftHandMiddle4: "LeftHandMiddle3",
    LeftHandRing1: "LeftHand",
    LeftHandRing2: "LeftHandRing1",
    LeftHandRing3: "LeftHandRing2",
    LeftHandRing4: "LeftHandRing3",
    LeftHandPinky1: "LeftHand",
    LeftHandPinky2: "LeftHandPinky1",
    LeftHandPinky3: "LeftHandPinky2",
    LeftHandPinky: "LeftHandPinky3",
};

// maps joint indices to parent joint indices.
var JOINT_PARENT_INDEX_MAP;
var JOINT_MIRROR_INDEX_MAP;

// ctor
function Xform(rot, pos) {
    this.rot = rot;
    this.pos = pos;
};
Xform.ident = function () {
    return new Xform({x: 0, y: 0, z: 0, w: 1}, {x: 0, y: 0, z: 0});
}
Xform.mul = function (lhs, rhs) {
    var rot = Quat.multiply(lhs.rot, rhs.rot);
    var pos = Vec3.sum(lhs.pos, Vec3.multiplyQbyV(lhs.rot, rhs.pos));
    return new Xform(rot, pos);
};
Xform.prototype.inv = function () {
    var invRot = Quat.inverse(this.rot);
    var invPos = Vec3.multiply(-1, this.pos);
    return new Xform(invRot, Vec3.multiplyQbyV(invRot, invPos));
};
Xform.prototype.mirrorX = function () {
    return new Xform({x: this.rot.x, y: -this.rot.y, z: -this.rot.z, w: this.rot.w},
                     {x: -this.pos.x, y: this.pos.y, z: this.pos.z});
}
Xform.prototype.toString = function () {
    var rot = this.rot;
    var pos = this.pos;
    return "Xform rot = (" + rot.x + ", " + rot.y + ", " + rot.z + ", " + rot.w + "), pos = (" + pos.x + ", " + pos.y + ", " + pos.z + ")";
};

function buildAbsoluteXformsFromMyAvatar() {
    var jointNames = MyAvatar.getJointNames();

    // lazy init of JOINT_PARENT_INDEX_MAP
    if (jointNames.length > 0 && !JOINT_PARENT_INDEX_MAP) {
        JOINT_PARENT_INDEX_MAP = {};
        var keys = Object.keys(JOINT_PARENT_NAME_MAP);
        var i, l = keys.length;
        var keyIndex, valueName, valueIndex;
        for (i = 0; i < l; i++) {
            keyIndex = MyAvatar.getJointIndex(keys[i]);
            valueName = JOINT_PARENT_NAME_MAP[keys[i]];
            if (valueName) {
                valueIndex = MyAvatar.getJointIndex(valueName);
            } else {
                valueIndex = -1;
            }
            JOINT_PARENT_INDEX_MAP[keyIndex] = valueIndex;
        }
    }

    // lazy init of JOINT_MIRROR_INDEX_MAP
    if (jointNames.length > 0 && !JOINT_MIRROR_INDEX_MAP) {
        JOINT_MIRROR_INDEX_MAP = {};
        var keys = Object.keys(JOINT_MIRROR_NAME_MAP);
        var i, l = keys.length;
        var keyIndex, valueName, valueIndex;
        for (i = 0; i < l; i++) {
            keyIndex = MyAvatar.getJointIndex(keys[i]);
            valueIndex = MyAvatar.getJointIndex(JOINT_MIRROR_NAME_MAP[keys[i]]);
            if (valueIndex > 0) {
                JOINT_MIRROR_INDEX_MAP[keyIndex] = valueIndex;
            }
        }
    }

    // build absolute xforms by multiplying by parent Xforms
    var absoluteXforms = [];
    var i, l = jointNames.length;
    var parentXform;
    for (i = 0; i < l; i++) {
        var parentIndex = JOINT_PARENT_INDEX_MAP[i];
        if (parentIndex >= 0) {
            parentXform = absoluteXforms[parentIndex];
        } else {
            parentXform = Xform.ident();
        }
        var localXform = new Xform(MyAvatar.getJointRotation(i), MyAvatar.getJointTranslation(i));
        absoluteXforms.push(Xform.mul(parentXform, localXform));
    }
    return absoluteXforms;
}

function buildRelativeXformsFromAbsoluteXforms(absoluteXforms) {

    // build relative xforms by multiplying by the inverse of the parent Xforms
    var relativeXforms = [];
    var i, l = absoluteXforms.length;
    var parentXform;
    for (i = 0; i < l; i++) {
        var parentIndex = JOINT_PARENT_INDEX_MAP[i];
        if (parentIndex >= 0) {
            parentXform = absoluteXforms[parentIndex];
        } else {
            parentXform = Xform.ident();
        }
        relativeXforms.push(Xform.mul(parentXform.inv(), absoluteXforms[i]));
    }
    return relativeXforms;
}

function createDoppelganger(avatar) {
    return new Doppelganger(avatar);
}

function createDoppelgangerEntity(doppelganger) {
    return Entities.addEntity(doppelganger.initialProperties);
}

function putDoppelgangerAcrossFromAvatar(doppelganger, avatar) {
    var avatarRot = Quat.fromPitchYawRollDegrees(0, avatar.bodyYaw, 0.0);
    var position;

    var ids = Entities.findEntities(MyAvatar.position, 20);
    var hasBase = false;
    for (var i = 0; i < ids.length; i++) {
        var entityID = ids[i];
        var props = Entities.getEntityProperties(entityID, "name");
        var name = props.name;
        if (name === "Hifi-Dressing-Room-Base") {
            var details = Entities.getEntityProperties(entityID, ["position", "dimensions"]);
            details.position.y += getAvatarFootOffset();
            details.position.y += details.dimensions.y / 2;
            position = details.position;
            hasBase = true;
        }
    }

    if (hasBase === false) {
        position = Vec3.sum(avatar.position, Vec3.multiply(1.5, Quat.getFront(avatarRot)));

    }

    return position;
}

function getAvatarFootOffset() {
    var data = getJointData();
    var upperLeg, lowerLeg, foot, toe, toeTop;
    data.forEach(function(d) {

        var jointName = d.joint;
        if (jointName === "RightUpLeg") {
            upperLeg = d.translation.y;
        }
        if (jointName === "RightLeg") {
            lowerLeg = d.translation.y;
        }
        if (jointName === "RightFoot") {
            foot = d.translation.y;
        }
        if (jointName === "RightToeBase") {
            toe = d.translation.y;
        }
        if (jointName === "RightToe_End") {
            toeTop = d.translation.y
        }
    })

    var myPosition = MyAvatar.position;
    var offset = upperLeg + lowerLeg + foot + toe + toeTop;
    offset = offset / 100;
    return offset
}


function getAvatarDimensions(avatar) {
    return dimensions;
}

function rotateDoppelgangerTowardAvatar(doppelganger, avatar) {
    var avatarRot = Quat.fromPitchYawRollDegrees(0, avatar.bodyYaw, 0.0);

    var ids = Entities.findEntities(MyAvatar.position, 20);
    var hasBase = false;
    for (var i = 0; i < ids.length; i++) {
        var entityID = ids[i];
        var props = Entities.getEntityProperties(entityID, "name");
        var name = props.name;
        if (name === "Hifi-Dressing-Room-Base") {
            var details = Entities.getEntityProperties(entityID, "rotation");
            avatarRot = details.rotation;
        }
    }
    if (hasBase === false) {
        avatarRot = Vec3.multiply(-1, avatarRot);
    }
    return avatarRot;
}

var isConnected = false;

function connectDoppelgangerUpdates() {
    Script.update.connect(updateDoppelganger);
    isConnected = true;
}

function disconnectDoppelgangerUpdates() {
    print('SHOULD DISCONNECT')
    if (isConnected === true) {
        Script.update.disconnect(updateDoppelganger);
    }
    isConnected = false;
}

var sinceLastUpdate = 0;

function updateDoppelganger(deltaTime) {
       if (THROTTLE === true) {
        sinceLastUpdate = sinceLastUpdate + deltaTime * 100;
        if (sinceLastUpdate > THROTTLE_RATE) {
            sinceLastUpdate = 0;
        } else {
            return;
        }
    }


    var absoluteXforms = buildAbsoluteXformsFromMyAvatar();
    if (MIRROR_JOINT_DATA) {
        var mirroredAbsoluteXforms = [];
        var i, l = absoluteXforms.length;
        for (i = 0; i < l; i++) {
            var mirroredIndex = JOINT_MIRROR_INDEX_MAP[i];
            if (mirroredIndex === undefined) {
                mirroredIndex = i;
            }
            mirroredAbsoluteXforms[mirroredIndex] = absoluteXforms[i].mirrorX();
        }
        absoluteXforms = mirroredAbsoluteXforms;
    }
    var relativeXforms = buildRelativeXformsFromAbsoluteXforms(absoluteXforms);
    doppelgangers.forEach(function(doppelganger) {
        setJointData(doppelganger, relativeXforms);
    });
}

function makeDoppelgangerForMyAvatar() {
    var doppelganger = createDoppelganger(MyAvatar);
    doppelgangers.push(doppelganger);
    connectDoppelgangerUpdates();
}

function subscribeToWearableMessages() {
    Messages.subscribe('Hifi-Doppelganger-Wearable');
    Messages.messageReceived.connect(handleWearableMessages);
}

function subscribeToFreezeMessages() {
    Messages.subscribe('Hifi-Doppelganger-Freeze');
    Messages.messageReceived.connect(handleFreezeMessages);
}

function handleFreezeMessages(channel, message, sender) {
    if (channel !== 'Hifi-Doppelganger-Freeze') {
        return;
    }
    if (sender !== MyAvatar.sessionUUID) {
        return;
    }

    var parsedMessage = null;

    try {
        parsedMessage = JSON.parse(message);
    } catch (e) {
        print('error parsing wearable message');
    }
    print('MESSAGE ACTION::' + parsedMessage.action)
    if (parsedMessage.action === 'freeze') {
        print('ACTUAL FREEZE')
        disconnectDoppelgangerUpdates();
    }
    if (parsedMessage.action === 'unfreeze') {
        print('ACTUAL UNFREEZE')

        connectDoppelgangerUpdates();
    }

}

var wearablePairs = [];

function handleWearableMessages(channel, message, sender) {
    if (channel !== 'Hifi-Doppelganger-Wearable' || 'Hifi-Doppelganger-Wearable-Avatar') {
        return;
    }

    if (sender !== MyAvatar.sessionUUID) {
        return;
    }

    var parsedMessage = null;

    try {
        parsedMessage = JSON.parse(message);
    } catch (e) {
        print('error parsing wearable message');
    }
    print('parsed message!!!')

    if (channel === 'Hifi-Doppelganger-Wearable') {
        mirrorEntitiesForDoppelganger(doppelgangers[0], parsedMessage);
    }
    if (channel === 'Hifi-Doppelganger-Wearable') {
        mirrorEntitiesForAvatar(parsedMessge);
    }

}

function mirrorEntitiesForDoppelganger(doppelganger, parsedMessage) {
    var doppelgangerProps = Entities.getEntityProperties(doppelganger.id);

    var action = parsedMessage.action;
    print('IN MIRROR ENTITIES CALL' + action)

    var baseEntity = parsedMessage.baseEntity;

    var wearableProps = Entities.getEntityProperties(baseEntity);

    print('WEARABLE PROPS::')
    delete wearableProps.id;
    delete wearableProps.created;
    delete wearableProps.age;
    delete wearableProps.ageAsText;
    //delete wearableProps.position;
    // add to dg
    // add to avatar
    // moved item on dg
    // moved item on avatar
    // remove item from dg
    // remove item from avatar

    var joint = wearableProps.parentJointIndex;
    if (action === 'add') {
        print('IN DOPPELGANGER ADD');

        wearableProps.parentID = doppelganger.id;
        wearableProps.parentJointIndex = joint;

        //create a new one
        wearableProps.script = MIRRORED_ENTITY_SCRIPT_URL;
        wearableProps.name = 'Hifi-Doppelganger-Mirrored-Entity';
        wearableProps.userData = JSON.stringify({
            doppelgangerKey: {
                baseEntity: baseEntity,
                doppelganger: doppelganger.id
            }
        })
        var mirrorEntity = Entities.addEntity(wearableProps);

        var mirrorEntityProps = Entities.getEntityProperties(mirrorEntity)
        print('MIRROR PROPS::' + JSON.stringify(mirrorEntityProps))
        var wearablePair = {
            baseEntity: baseEntity,
            mirrorEntity: mirrorEntity
        }

        wearablePairs.push(wearablePair);
    }

    if (action === 'update') {
        wearableProps.parentID = doppelganger.id;

        var mirrorEntity = getMirrorEntityForBaseEntity(baseEntity);
        //   print('MIRROR ENTITY, newPosition' + mirrorEntity + ":::" + JSON.stringify(newPosition))
        Entities.editEntity(mirrorEntity, wearableProps)
    }

    if (action === 'remove') {
        Entities.deleteEntity(getMirrorEntityForBaseEntity(baseEntity))
        wearablePairs = wearablePairs.filter(function(obj) {
            return obj.baseEntity !== baseEntity;
        });
    }

    if (action === 'updateBase') {
        //this gets called when the mirrored entity gets grabbed.  now we move the 
        var mirrorEntityProperties = Entities.getEntityProperties(message.mirrorEntity)
        var doppelgangerToMirrorEntity = Vec3.subtract(doppelgangerProps.position, mirrorEntityProperties.position);
        var newPosition = Vec3.sum(MyAvatar.position, doppelgangerToMirrorEntity);

        delete mirrorEntityProperties.id;
        delete mirrorEntityProperties.created;
        delete mirrorEntityProperties.age;
        delete mirrorEntityProperties.ageAsText;
        mirrorEntityProperties.position = newPosition;
        mirrorEntityProperties.parentID = MyAvatar.sessionUUID;
        Entities.editEntity(message.baseEntity, mirrorEntityProperties);
    }
}

function getMirrorEntityForBaseEntity(baseEntity) {
    var result = wearablePairs.filter(function(obj) {
        return obj.baseEntity === baseEntity;
    });
    if (result.length === 0) {
        return false;
    } else {
        return result[0].mirrorEntity
    }
}

function getBaseEntityForMirrorEntity(mirrorEntity) {
    var result = wearablePairs.filter(function(obj) {
        return obj.mirrorEntity === mirrorEntity;
    });
    if (result.length === 0) {
        return false;
    } else {
        return result[0].baseEntity
    }
}

makeDoppelgangerForMyAvatar();
subscribeToWearableMessages();
subscribeToWearableMessagesForAvatar();
subscribeToFreezeMessages();

function cleanup() {
    if (isConnected === true) {
        disconnectDoppelgangerUpdates();
    }

    doppelgangers.forEach(function(doppelganger) {
        print('DOPPELGANGER' + doppelganger.id)
        Entities.deleteEntity(doppelganger.id);
    });
}
Script.scriptEnding.connect(cleanup);