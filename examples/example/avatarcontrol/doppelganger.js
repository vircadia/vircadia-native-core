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

var doppelgangers = [];

function Doppelganger(avatar) {
    this.initialProperties = {
        name: 'Hifi-Doppelganger',
        type: 'Model',
        modelURL: TEST_MODEL_URL,
        // dimensions: getAvatarDimensions(avatar),
        position: putDoppelgangerAcrossFromAvatar(this, avatar),
        rotation: rotateDoppelgangerTowardAvatar(this, avatar),
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
        })
    });

    return allJointData;
}

function setJointData(doppelganger, allJointData) {
    var jointRotations = [];
    allJointData.forEach(function(jointData, index) {
        jointRotations.push(jointData.rotation);
    });
    Entities.setAbsoluteJointRotationsInObjectFrame(doppelganger.id, jointRotations);

    return true;
}

function mirrorJointData() {
    return mirroredJointData;
}

function createDoppelganger(avatar) {
    return new Doppelganger(avatar);
}

function createDoppelgangerEntity(doppelganger) {
    return Entities.addEntity(doppelganger.initialProperties);
}

function putDoppelgangerAcrossFromAvatar(doppelganger, avatar) {
    var avatarRot = Quat.fromPitchYawRollDegrees(0, avatar.bodyYaw, 0.0);
    var basePosition = Vec3.sum(avatar.position, Vec3.multiply(1.5, Quat.getFront(avatarRot)));
    return basePosition;
}

function getAvatarDimensions(avatar) {
    return dimensions;
}

function rotateDoppelgangerTowardAvatar(doppelganger, avatar) {
    var avatarRot = Quat.fromPitchYawRollDegrees(0, avatar.bodyYaw, 0.0);
    avatarRot = Vec3.multiply(-1, avatarRot);
    return avatarRot;
}

function connectDoppelgangerUpdates() {
    // Script.update.connect(updateDoppelganger);
    Script.setInterval(updateDoppelganger, 100);
}

function disconnectDoppelgangerUpdates() {
    Script.update.disconnect(updateDoppelganger);
}

function updateDoppelganger() {
    doppelgangers.forEach(function(doppelganger) {
        var joints = getJointData(MyAvatar);
        //var mirroredJoints = mirrorJointData(joints);
        setJointData(doppelganger, joints);
    });
}

function makeDoppelgangerForMyAvatar() {
    var doppelganger = createDoppelganger(MyAvatar);
    doppelgangers.push(doppelganger);
    connectDoppelgangerUpdates();
}

makeDoppelgangerForMyAvatar();

function cleanup() {
    disconnectDoppelgangerUpdates();

    doppelgangers.forEach(function(doppelganger) {
        Entities.deleteEntity(doppelganger.id);
    });
}

Script.scriptEnding.connect(cleanup);
