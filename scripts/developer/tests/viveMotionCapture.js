/* global Xform */
Script.include("/~/system/libraries/Xform.js");

var TRACKED_OBJECT_POSES = [
    "TrackedObject00", "TrackedObject01", "TrackedObject02", "TrackedObject03",
    "TrackedObject04", "TrackedObject05", "TrackedObject06", "TrackedObject07",
    "TrackedObject08", "TrackedObject09", "TrackedObject10", "TrackedObject11",
    "TrackedObject12", "TrackedObject13", "TrackedObject14", "TrackedObject15"
];

var calibrated = false;
var rightTriggerPressed = false;
var leftTriggerPressed = false;

var MAPPING_NAME = "com.highfidelity.viveMotionCapture";

var mapping = Controller.newMapping(MAPPING_NAME);
mapping.from([Controller.Standard.RTClick]).peek().to(function (value) {
    rightTriggerPressed = (value !== 0) ? true : false;
});
mapping.from([Controller.Standard.LTClick]).peek().to(function (value) {
    leftTriggerPressed = (value !== 0) ? true : false;
});

Controller.enableMapping(MAPPING_NAME);

var leftFoot;
var rightFoot;
var hips;
var spine2;

var FEET_ONLY = 0;
var FEET_AND_HIPS = 1;
var FEET_AND_CHEST = 2;
var FEET_HIPS_AND_CHEST = 3;

var SENSOR_CONFIG_NAMES = [
    "FeetOnly",
    "FeetAndHips",
    "FeetAndChest",
    "FeetHipsAndChest"
];

var ANIM_VARS = [
    "leftFootType",
    "leftFootPosition",
    "leftFootRotation",
    "rightFootType",
    "rightFootPosition",
    "rightFootRotation",
    "hipsType",
    "hipsPosition",
    "hipsRotation",
    "spine2Type",
    "spine2Position",
    "spine2Rotation"
];

var sensorConfig = FEET_HIPS_AND_CHEST;

var Y_180 = {x: 0, y: 1, z: 0, w: 0};
var Y_180_XFORM = new Xform(Y_180, {x: 0, y: 0, z: 0});

function computeOffsetXform(pose, jointIndex) {
    var poseXform = new Xform(pose.rotation, pose.translation);

    // TODO: we can do better here...
    // one idea, hang default pose skeleton from HMD head.
    var referenceXform = new Xform(MyAvatar.getAbsoluteDefaultJointRotationInObjectFrame(jointIndex),
                                   MyAvatar.getAbsoluteDefaultJointTranslationInObjectFrame(jointIndex));

    return Xform.mul(poseXform.inv(), referenceXform);
}

function calibrate() {
    print("AJT: calibrating...");

    leftFoot = undefined;
    rightFoot = undefined;
    hips = undefined;
    spine2 = undefined;

    var poses = [];
    if (Controller.Hardware.Vive) {
        TRACKED_OBJECT_POSES.forEach(function (key) {
            var channel = Controller.Hardware.Vive[key];
            var pose = Controller.getPoseValue(channel);
            if (pose.valid) {
                poses.push({
                    channel: channel,
                    pose: pose
                });
            }
        });
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
        rightFoot.offsetXform = computeOffsetXform(rightFoot.pose, MyAvatar.getJointIndex("RightFoot"));
        leftFoot.offsetXform = computeOffsetXform(leftFoot.pose, MyAvatar.getJointIndex("LeftFoot"));

        print("AJT: rightFoot = " + JSON.stringify(rightFoot));
        print("AJT: leftFoot = " + JSON.stringify(leftFoot));

        if (sensorConfig === FEET_ONLY) {
            // we're done!
        } else if (sensorConfig === FEET_AND_HIPS && poses.length >= 3) {
            hips = poses[2];
        } else if (sensorConfig === FEET_AND_CHEST && poses.length >= 3) {
            spine2 = poses[2];
        } else if (sensorConfig === FEET_HIPS_AND_CHEST && poses.length >= 4) {
            hips = poses[2];
            spine2 = poses[3];
        } else {
            // TODO: better error messages
            print("AJT: could not calibrate for sensor config " + SENSOR_CONFIG_NAMES[sensorConfig] + ", please try again!");
        }

        if (hips) {
            hips.offsetXform = computeOffsetXform(hips.pose, MyAvatar.getJointIndex("Hips"));
            print("AJT: hips = " + JSON.stringify(hips));
        }

        if (spine2) {
            spine2.offsetXform = computeOffsetXform(spine2.pose, MyAvatar.getJointIndex("Spine2"));
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

function computeIKTargetXform(jointInfo) {
    var pose = Controller.getPoseValue(jointInfo.channel);
    var offsetXform = jointInfo.offsetXform;
    return Xform.mul(Y_180_XFORM, Xform.mul(new Xform(pose.rotation, pose.translation), offsetXform));
}

function update(dt) {
    if (rightTriggerPressed && leftTriggerPressed) {
        if (!calibrated) {
            calibrate();
            calibrated = true;

            if (handlerId) {
                MyAvatar.removeAnimationStateHandler(handlerId);
            }

            handlerId = MyAvatar.addAnimationStateHandler(function (props) {

                var result = {}, xform;
                if (rightFoot) {
                    xform = computeIKTargetXform(rightFoot);
                    result.rightFootType = ikTypes.RotationAndPosition;
                    result.rightFootPosition = xform.pos;
                    result.rightFootRotation = xform.rot;
                } else {
                    result.rightFootType = props.rightFootType;
                    result.rightFootPosition = props.rightFootPosition;
                    result.rightFootRotation = props.rightFootRotation;
                }

                if (leftFoot) {
                    xform = computeIKTargetXform(leftFoot);
                    result.leftFootType = ikTypes.RotationAndPosition;
                    result.leftFootPosition = xform.pos;
                    result.leftFootRotation = xform.rot;
                } else {
                    result.leftFootType = props.leftFootType;
                    result.leftFootPosition = props.leftFootPosition;
                    result.leftFootRotation = props.leftFootRotation;
                }

                if (hips) {
                    xform = computeIKTargetXform(hips);
                    result.hipsType = ikTypes.RotationAndPosition;
                    result.hipsPosition = xform.pos;
                    result.hipsRotation = xform.rot;
                } else {
                    result.hipsType = props.hipsType;
                    result.hipsPosition = props.hipsPosition;
                    result.hipsRotation = props.hipsRotation;
                }

                if (spine2) {
                    xform = computeIKTargetXform(spine2);
                    result.spine2Type = ikTypes.RotationAndPosition;
                    result.spine2Position = xform.pos;
                    result.spine2Rotation = xform.rot;
                } else {
                    result.spine2Type = ikTypes.Off;
                }

                return result;
            }, ANIM_VARS);

        }
    } else {
        calibrated = false;
    }

    var drawMarkers = false;
    if (drawMarkers) {
        var RED = {x: 1, y: 0, z: 0, w: 1};
        var GREEN = {x: 0, y: 1, z: 0, w: 1};
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
}

Script.update.connect(update);

Script.scriptEnding.connect(function () {
    Controller.disableMapping(MAPPING_NAME);
    Script.update.disconnect(update);
});
var TRIGGER_OFF_VALUE = 0.1;
