var JOINT_PARENT_MAP = {
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

var USE_TRANSLATIONS = false;

// ctor
function Xform(rot, pos) {
    this.rot = rot;
    this.pos = pos;
};
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
Xform.prototype.toString = function () {
    var rot = this.rot;
    var pos = this.pos;
    return "Xform rot = (" + rot.x + ", " + rot.y + ", " + rot.z + ", " + rot.w + "), pos = (" + pos.x + ", " + pos.y + ", " + pos.z + ")";
};

function dumpHardwareMapping() {
    Object.keys(Controller.Hardware).forEach(function (deviceName) {
        Object.keys(Controller.Hardware[deviceName]).forEach(function (input) {
            print("Controller.Hardware." + deviceName + "." + input + ":" + Controller.Hardware[deviceName][input]);
        });
    });
}

// ctor
function NeuronAvatar() {
    var self = this;
    Script.scriptEnding.connect(function () {
        self.shutdown();
    });
    Controller.hardwareChanged.connect(function () {
        self.hardwareChanged();
    });

    if (Controller.Hardware.Neuron) {
        this.activate();
    } else {
        this.deactivate();
    }
}

NeuronAvatar.prototype.shutdown = function () {
    this.deactivate();
};

NeuronAvatar.prototype.hardwareChanged = function () {
    if (Controller.Hardware.Neuron) {
        this.activate();
    } else {
        this.deactivate();
    }
};

NeuronAvatar.prototype.activate = function () {
    if (!this._active) {
        Script.update.connect(updateCallback);
    }
    this._active = true;

    // build absDefaultPoseMap
    this._defaultAbsRotMap = {};
    this._defaultAbsPosMap = {};
    this._defaultAbsRotMap[""] = {x: 0, y: 0, z: 0, w: 1};
    this._defaultAbsPosMap[""] = {x: 0, y: 0, z: 0};
    var keys = Object.keys(JOINT_PARENT_MAP);
    var i, l = keys.length;
    for (i = 0; i < l; i++) {
        var jointName = keys[i];
        var j = MyAvatar.getJointIndex(jointName);
        var parentJointName = JOINT_PARENT_MAP[jointName];
        this._defaultAbsRotMap[jointName] = Quat.multiply(this._defaultAbsRotMap[parentJointName], MyAvatar.getDefaultJointRotation(j));
        this._defaultAbsPosMap[jointName] = Vec3.sum(this._defaultAbsPosMap[parentJointName],
                                                     Quat.multiply(this._defaultAbsRotMap[parentJointName], MyAvatar.getDefaultJointTranslation(j)));
    }
};

NeuronAvatar.prototype.deactivate = function () {
    if (this._active) {
        var self = this;
        Script.update.disconnect(updateCallback);
    }
    this._active = false;
    MyAvatar.clearJointsData();
};

NeuronAvatar.prototype.update = function (deltaTime) {

    var hmdActive = HMD.active;
    var keys = Object.keys(JOINT_PARENT_MAP);
    var i, l = keys.length;
    var absDefaultRot = {};
    var jointName, channel, pose, parentJointName, j, parentDefaultAbsRot;
    var localRotations = {};
    var localTranslations = {};
    for (i = 0; i < l; i++) {
        var jointName = keys[i];
        var channel = Controller.Hardware.Neuron[jointName];
        if (channel) {
            pose = Controller.getPoseValue(channel);
            parentJointName = JOINT_PARENT_MAP[jointName];
            j = MyAvatar.getJointIndex(jointName);
            defaultAbsRot = this._defaultAbsRotMap[jointName];
            parentDefaultAbsRot = this._defaultAbsRotMap[parentJointName];

            // Rotations from the neuron controller are in world orientation but are delta's from the default pose.
            // So first we build the absolute rotation of the default pose (local into world).
            // Then apply the rotation from the controller, in world space.
            // Then we transform back into joint local by multiplying by the inverse of the parents absolute rotation.
            var localRotation = Quat.multiply(Quat.inverse(parentDefaultAbsRot), Quat.multiply(pose.rotation, defaultAbsRot));
            if (!hmdActive || jointName !== "Hips") {
                MyAvatar.setJointRotation(j, localRotation);
            }
            localRotations[jointName] = localRotation;

            // translation proportions might be different from the neuron avatar and the user avatar skeleton.
            // so this is disabled by default
            if (USE_TRANSLATIONS) {
                var localTranslation = Vec3.multiplyQbyV(Quat.inverse(parentDefaultAbsRot), pose.translation);
                MyAvatar.setJointTranslation(j, localTranslation);
                localTranslations[jointName] = localTranslation;
            } else {
                localTranslations[jointName] = MyAvatar.getDefaultJointTranslation(j);
            }
        }
    }

    // it attempts to adjust the hips so that the avatar's head is at the same location & oreintation as the HMD.
    // however it's fighting with the internal c++ code that also attempts to adjust the hips.
    if (hmdActive) {
        var UNIT_SCALE = 1 / 100;
        var hmdXform = new Xform(HMD.orientation, Vec3.multiply(1 / UNIT_SCALE, HMD.position)); // convert to cm
        var y180Xform = new Xform({x: 0, y: 1, z: 0, w: 0}, {x: 0, y: 0, z: 0});
        var avatarXform = new Xform(MyAvatar.orientation, Vec3.multiply(1 / UNIT_SCALE, MyAvatar.position)); // convert to cm
        var hipsJointIndex = MyAvatar.getJointIndex("Hips");
        var modelOffsetInvXform = new Xform({x: 0, y: 0, z: 0, w: 1}, MyAvatar.getDefaultJointTranslation(hipsJointIndex));
        var defaultHipsXform = new Xform(MyAvatar.getDefaultJointRotation(hipsJointIndex), MyAvatar.getDefaultJointTranslation(hipsJointIndex));

        var headXform = new Xform(localRotations["Head"], localTranslations["Head"]);

        // transform eyes down the heirarchy chain into avatar space.
        var hierarchy = ["Neck", "Spine3", "Spine2", "Spine1", "Spine"];
        var i, l = hierarchy.length;
        for (i = 0; i < l; i++) {
            var xform = new Xform(localRotations[hierarchy[i]], localTranslations[hierarchy[i]]);
            headXform = Xform.mul(xform, headXform);
        }
        headXform = Xform.mul(defaultHipsXform, headXform);

        var preXform = Xform.mul(headXform, y180Xform);
        var postXform = Xform.mul(avatarXform, Xform.mul(y180Xform, modelOffsetInvXform.inv()));

        // solve for the offset that will put the eyes at the hmd position & orientation.
        var hipsOffsetXform = Xform.mul(postXform.inv(), Xform.mul(hmdXform, preXform.inv()));

        // now combine it with the default hips transform
        var hipsXform = Xform.mul(hipsOffsetXform, defaultHipsXform);

        MyAvatar.setJointRotation("Hips", hipsXform.rot);
        MyAvatar.setJointTranslation("Hips", hipsXform.pos);
    }
};

var neuronAvatar = new NeuronAvatar();

function updateCallback(deltaTime) {
    neuronAvatar.update(deltaTime);
}

