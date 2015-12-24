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
    for (i = 0; i < l; i++) {
        var channel = Controller.Hardware.Neuron[keys[i]];
        if (channel) {
            var pose = Controller.getPoseValue(channel);
            var j = MyAvatar.getJointIndex(JOINT_NAME_MAP[keys[i]]);
            var defaultRot = MyAvatar.getDefaultJointRotation(j);
            var rot = Quat.multiply(Quat.inverse(defaultRot), Quat.multiply(pose.rotation, defaultRot));
            MyAvatar.setJointRotation(j, Quat.multiply(defaultRot, rot));
            //MyAvatar.setJointTranslation(j, Vec3.multiply(100.0, pose.translation));
        }
    }
};

var neuronAvatar = new NeuronAvatar();

function updateCallback(deltaTime) {
    neuronAvatar.update(deltaTime);
}

