// maps controller joint names to avatar joint names
var JOINT_NAME_MAP = {
    Hips: "Hips",
    RightUpLeg: "RightUpLeg",
    RightLeg: "RightLeg",
    RightFoot: "RightFoot",
    LeftUpLeg: "LeftUpLeg",
    LeftLeg: "LeftLeg",
    LeftFoot: "LeftFoot",
    Spine: "Spine",
    Spine1: "Spine1",
    Spine2: "Spine2",
    Spine3: "Spine3",
    Neck: "Neck",
    Head: "Head",
    RightShoulder: "RightShoulder",
    RightArm: "RightArm",
    RightForeArm: "RightForeArm",
    RightHand: "RightHand",
    RightHandThumb1: "RightHandThumb2",
    RightHandThumb2: "RightHandThumb3",
    RightHandThumb3: "RightHandThumb4",
    RightInHandIndex: "RightHandIndex1",
    RightHandIndex1: "RightHandIndex2",
    RightHandIndex2: "RightHandIndex3",
    RightHandIndex3: "RightHandIndex4",
    RightInHandMiddle: "RightHandMiddle1",
    RightHandMiddle1: "RightHandMiddle2",
    RightHandMiddle2: "RightHandMiddle3",
    RightHandMiddle3: "RightHandMiddle4",
    RightInHandRing: "RightHandRing1",
    RightHandRing1: "RightHandRing2",
    RightHandRing2: "RightHandRing3",
    RightHandRing3: "RightHandRing4",
    RightInHandPinky: "RightHandPinky1",
    RightHandPinky1: "RightHandPinky2",
    RightHandPinky2: "RightHandPinky3",
    RightHandPinky3: "RightHandPinky4",
    LeftShoulder: "LeftShoulder",
    LeftArm: "LeftArm",
    LeftForeArm: "LeftForeArm",
    LeftHand: "LeftHand",
    LeftHandThumb1: "LeftHandThumb2",
    LeftHandThumb2: "LeftHandThumb3",
    LeftHandThumb3: "LeftHandThumb4",
    LeftInHandIndex: "LeftHandIndex1",
    LeftHandIndex1: "LeftHandIndex2",
    LeftHandIndex2: "LeftHandIndex3",
    LeftHandIndex3: "LeftHandIndex4",
    LeftInHandMiddle: "LeftHandMiddle1",
    LeftHandMiddle1: "LeftHandMiddle2",
    LeftHandMiddle2: "LeftHandMiddle3",
    LeftHandMiddle3: "LeftHandMiddle4",
    LeftInHandRing: "LeftHandRing1",
    LeftHandRing1: "LeftHandRing2",
    LeftHandRing2: "LeftHandRing3",
    LeftHandRing3: "LeftHandRing4",
    LeftInHandPinky: "LeftHandPinky1",
    LeftHandPinky1: "LeftHandPinky2",
    LeftHandPinky2: "LeftHandPinky3",
    LeftHandPinky3: "LeftHandPinky4"
};

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
    this._absDefaultRotMap = {};
    var keys = Object.keys(JOINT_NAME_MAP);
    var i, l = keys.length;
    for (i = 0; i < l; i++) {
        var jointName = JOINT_NAME_MAP[keys[i]];
        var j = MyAvatar.getJointIndex(jointName);
        var parentJointName = JOINT_PARENT_MAP[jointName];
        if (parentJointName === "") {
            this._absDefaultRotMap[jointName] = MyAvatar.getDefaultJointRotation(j);
        } else {
            this._absDefaultRotMap[jointName] = Quat.multiply(this._absDefaultRotMap[parentJointName], MyAvatar.getDefaultJointRotation(j));
        }
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
    var keys = Object.keys(JOINT_NAME_MAP);
    var i, l = keys.length;
    var absDefaultRot = {};
    for (i = 0; i < l; i++) {
        var channel = Controller.Hardware.Neuron[keys[i]];
        if (channel) {
            var pose = Controller.getPoseValue(channel);
            var jointName = JOINT_NAME_MAP[keys[i]];
            var parentJointName = JOINT_PARENT_MAP[jointName];
            var j = MyAvatar.getJointIndex(jointName);
            var defaultAbsRot = this._absDefaultRotMap[jointName];
            var parentDefaultAbsRot;
            if (parentJointName === "") {
                 parentDefaultAbsRot = {x: 0, y: 0, z: 0, w: 1};
            } else {
                parentDefaultAbsRot = this._absDefaultRotMap[parentJointName];
            }
            MyAvatar.setJointRotation(j, Quat.multiply(Quat.inverse(parentDefaultAbsRot), Quat.multiply(pose.rotation, defaultAbsRot)));
            //MyAvatar.setJointTranslation(j, Vec3.multiply(100.0, pose.translation));
        }
    }
};

var neuronAvatar = new NeuronAvatar();

function updateCallback(deltaTime) {
    neuronAvatar.update(deltaTime);
}

