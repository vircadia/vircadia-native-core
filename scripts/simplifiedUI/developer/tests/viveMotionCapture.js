/* global Xform */
Script.include("/~/system/libraries/Xform.js");

var TRACKED_OBJECT_POSES = [
    "TrackedObject00", "TrackedObject01", "TrackedObject02", "TrackedObject03",
    "TrackedObject04", "TrackedObject05", "TrackedObject06", "TrackedObject07",
    "TrackedObject08", "TrackedObject09", "TrackedObject10", "TrackedObject11",
    "TrackedObject12", "TrackedObject13", "TrackedObject14", "TrackedObject15"
];

var triggerPressHandled = false;
var rightTriggerPressed = false;
var leftTriggerPressed = false;
var calibrationCount = 0;

var TRIGGER_MAPPING_NAME = "com.highfidelity.viveMotionCapture.triggers";
var triggerMapping = Controller.newMapping(TRIGGER_MAPPING_NAME);
triggerMapping.from([Controller.Standard.RTClick]).peek().to(function (value) {
    rightTriggerPressed = (value !== 0) ? true : false;
});
triggerMapping.from([Controller.Standard.LTClick]).peek().to(function (value) {
    leftTriggerPressed = (value !== 0) ? true : false;
});
Controller.enableMapping(TRIGGER_MAPPING_NAME);

var CONTROLLER_MAPPING_NAME = "com.highfidelity.viveMotionCapture.controller";
var controllerMapping;

var head;
var leftFoot;
var rightFoot;
var hips;
var spine2;

var FEET_ONLY = 0;
var FEET_AND_HIPS = 1;
var FEET_AND_CHEST = 2;
var FEET_HIPS_AND_CHEST = 3;
var AUTO = 4;

var SENSOR_CONFIG_NAMES = [
    "FeetOnly",
    "FeetAndHips",
    "FeetAndChest",
    "FeetHipsAndChest",
    "Auto"
];

var sensorConfig = AUTO;

var Y_180 = {x: 0, y: 1, z: 0, w: 0};
var Y_180_XFORM = new Xform(Y_180, {x: 0, y: 0, z: 0});

function computeOffsetXform(defaultToReferenceXform, pose, jointIndex) {
    var poseXform = new Xform(pose.rotation, pose.translation);

    var defaultJointXform = new Xform(MyAvatar.getAbsoluteDefaultJointRotationInObjectFrame(jointIndex),
                                      MyAvatar.getAbsoluteDefaultJointTranslationInObjectFrame(jointIndex));

    var referenceJointXform = Xform.mul(defaultToReferenceXform, defaultJointXform);

    return Xform.mul(poseXform.inv(), referenceJointXform);
}

function computeDefaultToReferenceXform() {
    var headIndex = MyAvatar.getJointIndex("Head");
    if (headIndex >= 0) {
        var defaultHeadXform = new Xform(MyAvatar.getAbsoluteDefaultJointRotationInObjectFrame(headIndex),
                                         MyAvatar.getAbsoluteDefaultJointTranslationInObjectFrame(headIndex));
        var currentHeadXform = new Xform(Quat.cancelOutRollAndPitch(MyAvatar.getAbsoluteJointRotationInObjectFrame(headIndex)),
                                         MyAvatar.getAbsoluteJointTranslationInObjectFrame(headIndex));

        var defaultToReferenceXform = Xform.mul(currentHeadXform, defaultHeadXform.inv());

        return defaultToReferenceXform;
    } else {
        return Xform.ident();
    }
}

function computeHeadOffsetXform() {
    var leftEyeIndex = MyAvatar.getJointIndex("LeftEye");
    var rightEyeIndex = MyAvatar.getJointIndex("RightEye");
    var headIndex = MyAvatar.getJointIndex("Head");
    if (leftEyeIndex > 0 && rightEyeIndex > 0 && headIndex > 0) {
        var defaultHeadXform = new Xform(MyAvatar.getAbsoluteDefaultJointRotationInObjectFrame(headIndex),
                                         MyAvatar.getAbsoluteDefaultJointTranslationInObjectFrame(headIndex));
        var defaultLeftEyeXform = new Xform(MyAvatar.getAbsoluteDefaultJointRotationInObjectFrame(leftEyeIndex),
                                            MyAvatar.getAbsoluteDefaultJointTranslationInObjectFrame(leftEyeIndex));
        var defaultRightEyeXform = new Xform(MyAvatar.getAbsoluteDefaultJointRotationInObjectFrame(rightEyeIndex),
                                             MyAvatar.getAbsoluteDefaultJointTranslationInObjectFrame(rightEyeIndex));
        var defaultCenterEyePos = Vec3.multiply(0.5, Vec3.sum(defaultLeftEyeXform.pos, defaultRightEyeXform.pos));
        var defaultCenterEyeXform = new Xform(defaultLeftEyeXform.rot, defaultCenterEyePos);

        return Xform.mul(defaultCenterEyeXform.inv(), defaultHeadXform);
    } else {
        return undefined;
    }
}

function calibrate() {

    head = undefined;
    leftFoot = undefined;
    rightFoot = undefined;
    hips = undefined;
    spine2 = undefined;

    var defaultToReferenceXform = computeDefaultToReferenceXform();

    var headOffsetXform = computeHeadOffsetXform();
    print("AJT: computed headOffsetXform " + (headOffsetXform ? JSON.stringify(headOffsetXform) : "undefined"));

    if (headOffsetXform) {
        head = { offsetXform: headOffsetXform };
    }

    var poses = [];
    if (Controller.Hardware.Vive) {
        TRACKED_OBJECT_POSES.forEach(function (key) {
            var channel = Controller.Hardware.Vive[key];
            var pose = Controller.getPoseValue(channel);
            if (pose.valid) {
                poses.push({
                    channel: channel,
                    pose: pose,
                    lastestPose: pose
                });
            }
        });
    }

    print("AJT: calibrating, num tracked poses = " + poses.length + ", sensorConfig = " + SENSOR_CONFIG_NAMES[sensorConfig]);

    var config = sensorConfig;

    if (config === AUTO) {
        if (poses.length === 2) {
            config = FEET_ONLY;
        } else if (poses.length === 3) {
            config = FEET_AND_HIPS;
        } else if (poses.length >= 4) {
            config = FEET_HIPS_AND_CHEST;
        } else {
            print("AJT: auto config failed: poses.length = " + poses.length);
            config = FEET_ONLY;
        }
    }

    if (poses.length >= 2) {
        // sort by y
        poses.sort(function(a, b) {
            var ay = a.pose.translation.y;
            var by = b.pose.translation.y;
            return ay - by;
        });

        if (poses[0].pose.translation.x > poses[1].pose.translation.x) {
            rightFoot = poses[0];
            leftFoot = poses[1];
        } else {
            rightFoot = poses[1];
            leftFoot = poses[0];
        }

        // compute offsets
        rightFoot.offsetXform = computeOffsetXform(defaultToReferenceXform, rightFoot.pose, MyAvatar.getJointIndex("RightFoot"));
        leftFoot.offsetXform = computeOffsetXform(defaultToReferenceXform, leftFoot.pose, MyAvatar.getJointIndex("LeftFoot"));

        print("AJT: rightFoot = " + JSON.stringify(rightFoot));
        print("AJT: leftFoot = " + JSON.stringify(leftFoot));

        if (config === FEET_ONLY) {
            // we're done!
        } else if (config === FEET_AND_HIPS && poses.length >= 3) {
            hips = poses[2];
        } else if (config === FEET_AND_CHEST && poses.length >= 3) {
            spine2 = poses[2];
        } else if (config === FEET_HIPS_AND_CHEST && poses.length >= 4) {
            hips = poses[2];
            spine2 = poses[3];
        } else {
            // TODO: better error messages
            print("AJT: could not calibrate for sensor config " + SENSOR_CONFIG_NAMES[config] + ", please try again!");
        }

        if (hips) {
            hips.offsetXform = computeOffsetXform(defaultToReferenceXform, hips.pose, MyAvatar.getJointIndex("Hips"));
            print("AJT: hips = " + JSON.stringify(hips));
        }

        if (spine2) {
            spine2.offsetXform = computeOffsetXform(defaultToReferenceXform, spine2.pose, MyAvatar.getJointIndex("Spine2"));
            print("AJT: spine2 = " + JSON.stringify(spine2));
        }

    } else {
        print("AJT: could not find two trackers, try again!");
    }
}

var ikTypes = {
    RotationAndPosition: 0,
    RotationOnly: 1,
    HmdHead: 2,
    HipsRelativeRotationAndPosition: 3,
    Off: 4
};

var handlerId;

function convertJointInfoToPose(jointInfo) {
    var latestPose = jointInfo.latestPose;
    var offsetXform = jointInfo.offsetXform;
    var xform = Xform.mul(new Xform(latestPose.rotation, latestPose.translation), offsetXform);
    return {
        valid: true,
        translation: xform.pos,
        rotation: xform.rot,
        velocity: Vec3.sum(latestPose.velocity, Vec3.cross(latestPose.angularVelocity, Vec3.subtract(xform.pos, latestPose.translation))),
        angularVelocity: latestPose.angularVelocity
    };
}

function update(dt) {
    if (rightTriggerPressed && leftTriggerPressed) {
        if (!triggerPressHandled) {
            triggerPressHandled = true;
            if (controllerMapping) {

                // go back to normal, vive pucks will be ignored.
                print("AJT: UN-CALIBRATE!");

                head = undefined;
                leftFoot = undefined;
                rightFoot = undefined;
                hips = undefined;
                spine2 = undefined;

                Controller.disableMapping(CONTROLLER_MAPPING_NAME + calibrationCount);
                controllerMapping = undefined;

            } else {
                print("AJT: CALIBRATE!");
                calibrate();
                calibrationCount++;

                controllerMapping = Controller.newMapping(CONTROLLER_MAPPING_NAME + calibrationCount);

                if (head) {
                    controllerMapping.from(function () {
                        var worldToAvatarXform = (new Xform(MyAvatar.orientation, MyAvatar.position)).inv();
                        head.latestPose = {
                            valid: true,
                            translation: worldToAvatarXform.xformPoint(HMD.position),
                            rotation: Quat.multiply(worldToAvatarXform.rot, Quat.multiply(HMD.orientation, Y_180)), // postMult 180 rot flips head direction
                            velocity: {x: 0, y: 0, z: 0},  // TODO: currently this is unused anyway...
                            angularVelocity: {x: 0, y: 0, z: 0}
                        };
                        return convertJointInfoToPose(head);
                    }).to(Controller.Standard.Head);
                }

                if (leftFoot) {
                    controllerMapping.from(leftFoot.channel).to(function (pose) {
                        leftFoot.latestPose = pose;
                    });
                    controllerMapping.from(function () {
                        return convertJointInfoToPose(leftFoot);
                    }).to(Controller.Standard.LeftFoot);
                }
                if (rightFoot) {
                    controllerMapping.from(rightFoot.channel).to(function (pose) {
                        rightFoot.latestPose = pose;
                    });
                    controllerMapping.from(function () {
                        return convertJointInfoToPose(rightFoot);
                    }).to(Controller.Standard.RightFoot);
                }
                if (hips) {
                    controllerMapping.from(hips.channel).to(function (pose) {
                        hips.latestPose = pose;
                    });
                    controllerMapping.from(function () {
                        return convertJointInfoToPose(hips);
                    }).to(Controller.Standard.Hips);
                }
                if (spine2) {
                    controllerMapping.from(spine2.channel).to(function (pose) {
                        spine2.latestPose = pose;
                    });
                    controllerMapping.from(function () {
                        return convertJointInfoToPose(spine2);
                    }).to(Controller.Standard.Spine2);
                }
                Controller.enableMapping(CONTROLLER_MAPPING_NAME + calibrationCount);
            }
        }
    } else {
        triggerPressHandled = false;
    }

    var drawMarkers = false;
    if (drawMarkers) {
        var RED = {x: 1, y: 0, z: 0, w: 1};
        var BLUE = {x: 0, y: 0, z: 1, w: 1};

        if (leftFoot) {
            var leftFootPose = Controller.getPoseValue(leftFoot.channel);
            DebugDraw.addMyAvatarMarker("leftFootTracker", leftFootPose.rotation, leftFootPose.translation, BLUE);
        }

        if (rightFoot) {
            var rightFootPose = Controller.getPoseValue(rightFoot.channel);
            DebugDraw.addMyAvatarMarker("rightFootTracker", rightFootPose.rotation, rightFootPose.translation, RED);
        }

        if (hips) {
            var hipsPose = Controller.getPoseValue(hips.channel);
            DebugDraw.addMyAvatarMarker("hipsTracker", hipsPose.rotation, hipsPose.translation, GREEN);
        }
    }

    var drawReferencePose = false;
    if (drawReferencePose) {
        var GREEN = {x: 0, y: 1, z: 0, w: 1};
        var defaultToReferenceXform = computeDefaultToReferenceXform();
        var leftFootIndex = MyAvatar.getJointIndex("LeftFoot");
        if (leftFootIndex > 0) {
            var defaultLeftFootXform = new Xform(MyAvatar.getAbsoluteDefaultJointRotationInObjectFrame(leftFootIndex),
                                                 MyAvatar.getAbsoluteDefaultJointTranslationInObjectFrame(leftFootIndex));
            var referenceLeftFootXform = Xform.mul(defaultToReferenceXform, defaultLeftFootXform);
            DebugDraw.addMyAvatarMarker("leftFootTracker", referenceLeftFootXform.rot, referenceLeftFootXform.pos, GREEN);
        }
    }

}

Script.update.connect(update);

Script.scriptEnding.connect(function () {
    Controller.disableMapping(TRIGGER_MAPPING_NAME);
    if (controllerMapping) {
        Controller.disableMapping(CONTROLLER_MAPPING_NAME + calibrationCount);
    }
    Script.update.disconnect(update);
});

