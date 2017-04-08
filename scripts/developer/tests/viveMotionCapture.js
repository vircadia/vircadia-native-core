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

var Y_180 = {x: 0, y: 1, z: 0, w: 0};

function computeOffsetXform(pose, jointIndex) {
    var poseXform = new Xform(pose.rotation, pose.translation);
    var referenceXform = new Xform(MyAvatar.getAbsoluteDefaultJointRotationInObjectFrame(jointIndex),
                                   MyAvatar.getAbsoluteDefaultJointTranslationInObjectFrame(jointIndex));
    return Xform.mul(poseXform.inv(), referenceXform);
}

function calibrate() {
    print("AJT: calibrating");
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

        if (poses.length >= 3) {
            hips = poses[2];
            hips.offsetXform = computeOffsetXform(hips.pose, MyAvatar.getJointIndex("Hips"));

            print("AJT: hips = " + JSON.stringify(hips));
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

function update(dt) {
    if (rightTriggerPressed && leftTriggerPressed) {
        if (!calibrated) {
            calibrate();
            calibrated = true;

            if (handlerId) {
                MyAvatar.removeAnimationStateHandler(handlerId);
            }

            // hook up anim state callback
            var propList = [
                "leftFootType", "leftFootPosition", "leftFootRotation",
                "rightFootType", "rightFootPosition", "rightFootRotation",
                "hipsType", "hipsPosition", "hipsRotation"
            ];

            handlerId = MyAvatar.addAnimationStateHandler(function (props) {

                var result = {}, pose, offsetXform, xform;
                if (rightFoot) {
                    pose = Controller.getPoseValue(rightFoot.channel);
                    offsetXform = rightFoot.offsetXform;

                    xform = Xform.mul(new Xform(pose.rotation, pose.translation), offsetXform);
                    result.rightFootType = ikTypes.RotationAndPosition;
                    result.rightFootPosition = Vec3.multiplyQbyV(Y_180, xform.pos);
                    result.rightFootRotation = Quat.multiply(Y_180, xform.rot);

                } else {
                    result.rightFootType = props.rightFootType;
                    result.rightFootPositon = props.rightFootPosition;
                    result.rightFootRotation = props.rightFootRotation;
                }

                if (leftFoot) {
                    pose = Controller.getPoseValue(leftFoot.channel);
                    offsetXform = leftFoot.offsetXform;
                    xform = Xform.mul(new Xform(pose.rotation, pose.translation), offsetXform);
                    result.leftFootType = ikTypes.RotationAndPosition;
                    result.leftFootPosition = Vec3.multiplyQbyV(Y_180, xform.pos);
                    result.leftFootRotation = Quat.multiply(Y_180, xform.rot);
                } else {
                    result.leftFootType = props.leftFootType;
                    result.leftFootPositon = props.leftFootPosition;
                    result.leftFootRotation = props.leftFootRotation;
                }

                if (hips) {
                    pose = Controller.getPoseValue(hips.channel);
                    offsetXform = hips.offsetXform;
                    xform = Xform.mul(new Xform(pose.rotation, pose.translation), offsetXform);
                    result.hipsType = ikTypes.RotationAndPosition;
                    result.hipsPosition = Vec3.multiplyQbyV(Y_180, xform.pos);
                    result.hipsRotation = Quat.multiply(Y_180, xform.rot);
                } else {
                    result.hipsType = props.hipsType;
                    result.hipsPositon = props.hipsPosition;
                    result.hipsRotation = props.hipsRotation;
                }

                return result;
            }, propList);

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
