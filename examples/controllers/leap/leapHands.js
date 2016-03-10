//
//  leapHands.js
//  examples
//
//  Created by David Rowe on 8 Sep 2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  This is an example script that uses the Leap Motion to make the avatar's hands replicate the user's hand actions.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var leftTriggerValue = 0;
var rightTriggerValue = 0;

var LEAP_TRIGGER_START_ANGLE = 15.0;
var LEAP_TRIGGER_END_ANGLE = 40.0;

function getLeapMotionLeftTrigger() {
    //print("left trigger = " + leftTriggerValue);
    return leftTriggerValue;
}
function getLeapMotionRightTrigger() {
    //print("right trigger = " + rightTriggerValue);
    return rightTriggerValue;
}

var leapHands = (function () {

    var isOnHMD,
        LEAP_ON_HMD_MENU_ITEM = "Leap Motion on HMD",
        LEAP_OFFSET = 0.019,  // Thickness of Leap Motion plus HMD clip
        HMD_OFFSET = 0.070,  // Eyeballs to front surface of Oculus DK2  TODO: Confirm and make depend on device and eye relief
        hasHandAndWristJoints,
        handToWristOffset = [],  // For avatars without a wrist joint we control an estimate of a proper hand joint position
        HAND_OFFSET = 0.4,  // Relative distance of wrist to hand versus wrist to index finger knuckle
        handAnimationStateHandlers,
        handAnimationStateFunctions,
        handAnimationStateProperties,
        hands,
        wrists,
        NUM_HANDS = 2,  // 0 = left; 1 = right
        fingers,
        NUM_FINGERS = 5,  // 0 = thumb; ...; 4 = pinky
        THUMB = 0,
        MIDDLE_FINGER = 2,
        NUM_FINGER_JOINTS = 3,  // 0 = metacarpal(hand)-proximal(finger) joint; ...; 2 = intermediate-distal joint
        MAX_HAND_INACTIVE_COUNT = 20,
        calibrationStatus,
        UNCALIBRATED = 0,
        CALIBRATING = 1,
        CALIBRATED = 2,
        CALIBRATION_TIME = 1000,  // milliseconds
        avatarScale,
        avatarFaceModelURL,
        avatarSkeletonModelURL,
        settingsTimer,
        HMD_CAMERA_TO_AVATAR_ROTATION = [
            Quat.angleAxis(180.0, { x: 0, y: 0, z: 1 }),
            Quat.angleAxis(-180.0, { x: 0, y: 0, z: 1 })
        ],
        DESKTOP_CAMERA_TO_AVATAR_ROTATION =
            Quat.multiply(Quat.angleAxis(180.0, { x: 0, y: 1, z: 0 }), Quat.angleAxis(90.0, { x: 0, y: 0, z: 1 })),
        LEAP_THUMB_ROOT_ADJUST = [Quat.fromPitchYawRollDegrees(0, 0, 20), Quat.fromPitchYawRollDegrees(0, 0, -20)];

    function printSkeletonJointNames() {
        var jointNames,
            i;

        print(MyAvatar.skeletonModelURL);

        print("Skeleton joint names ...");
        jointNames = MyAvatar.getJointNames();
        for (i = 0; i < jointNames.length; i += 1) {
            print(i + ": " + jointNames[i]);
        }
        print("... skeleton joint names");
    }

    function animateLeftHand() {
        var ROTATION_AND_POSITION = 0;

        return {
            leftHandType: ROTATION_AND_POSITION,
            leftHandPosition: hands[0].position,
            leftHandRotation: hands[0].rotation
        };
    }

    function animateRightHand() {
        var ROTATION_AND_POSITION = 0;

        return {
            rightHandType: ROTATION_AND_POSITION,
            rightHandPosition: hands[1].position,
            rightHandRotation: hands[1].rotation
        };
    }

    function finishCalibration() {
        var avatarPosition,
            handPosition,
            middleFingerPosition,
            leapHandHeight,
            h;

        if (!isOnHMD) {
            if (hands[0].controller.isActive() && hands[1].controller.isActive()) {
                leapHandHeight = (hands[0].controller.getAbsTranslation().y + hands[1].controller.getAbsTranslation().y) / 2.0;
            } else {
                calibrationStatus = UNCALIBRATED;
                return;
            }
        }

        avatarPosition = MyAvatar.position;

        for (h = 0; h < NUM_HANDS; h += 1) {
            handPosition = MyAvatar.getJointPosition(hands[h].jointName);
            if (!hasHandAndWristJoints) {
                middleFingerPosition = MyAvatar.getJointPosition(fingers[h][MIDDLE_FINGER][0].jointName);
                handToWristOffset[h] = Vec3.multiply(Vec3.subtract(handPosition, middleFingerPosition), 1.0 - HAND_OFFSET);
            }

            if (isOnHMD) {
                // Offset of Leap Motion origin from physical eye position
                hands[h].zeroPosition = { x: 0.0, y: 0.0, z: HMD_OFFSET + LEAP_OFFSET };
            } else {
                hands[h].zeroPosition = {
                    x: handPosition.x - avatarPosition.x,
                    y: handPosition.y - avatarPosition.y,
                    z: avatarPosition.z - handPosition.z
                };
                hands[h].zeroPosition = Vec3.multiplyQbyV(MyAvatar.orientation, hands[h].zeroPosition);
                hands[h].zeroPosition.y = hands[h].zeroPosition.y - leapHandHeight;
            }
        }

        MyAvatar.clearJointData("LeftHand");
        MyAvatar.clearJointData("LeftForeArm");
        MyAvatar.clearJointData("RightHand");
        MyAvatar.clearJointData("RightForeArm");

        calibrationStatus = CALIBRATED;
        print("Leap Motion: Calibrated");
    }

    function calibrate() {
        var jointNames,
            i;

        calibrationStatus = CALIBRATING;

        avatarScale = MyAvatar.scale;
        avatarFaceModelURL = MyAvatar.faceModelURL;
        avatarSkeletonModelURL = MyAvatar.skeletonModelURL;

        // Does this skeleton have both wrist and hand joints?
        hasHandAndWristJoints = false;
        jointNames = MyAvatar.getJointNames();
        for (i = 0; i < jointNames.length; i += 1) {
            hasHandAndWristJoints = hasHandAndWristJoints || jointNames[i].toLowerCase() === "leftwrist";
        }

        // Set avatar arms vertical, forearms horizontal, as "zero" position for calibration
        MyAvatar.setJointRotation("LeftForeArm", Quat.fromPitchYawRollDegrees(0.0, 0.0, 90.0));
        MyAvatar.setJointRotation("LeftHand", Quat.fromPitchYawRollDegrees(0.0, 90.0, 0.0));
        MyAvatar.setJointRotation("RightForeArm", Quat.fromPitchYawRollDegrees(0.0, 0.0, -90.0));
        MyAvatar.setJointRotation("RightHand", Quat.fromPitchYawRollDegrees(0.0, -90.0, 0.0));

        // Wait for arms to assume their positions before calculating
        Script.setTimeout(finishCalibration, CALIBRATION_TIME);
    }

    function checkCalibration() {

        if (calibrationStatus === CALIBRATED) {
            return true;
        }

        if (calibrationStatus !== CALIBRATING) {
            calibrate();
        }

        return false;
    }

    function setIsOnHMD() {
        isOnHMD = Menu.isOptionChecked(LEAP_ON_HMD_MENU_ITEM);
        print("Leap Motion: " + (isOnHMD ? "Is on HMD" : "Is on desk"));
    }

    function checkSettings() {
        if (calibrationStatus > UNCALIBRATED && (MyAvatar.scale !== avatarScale
            || MyAvatar.faceModelURL !== avatarFaceModelURL
            || MyAvatar.skeletonModelURL !== avatarSkeletonModelURL
            || Menu.isOptionChecked(LEAP_ON_HMD_MENU_ITEM) !== isOnHMD)) {
            print("Leap Motion: Recalibrate...");
            calibrationStatus = UNCALIBRATED;

            setIsOnHMD();
        }
    }

    function setUp() {

        wrists = [
            {
                jointName: "LeftWrist",
                controller: Controller.createInputController("Spatial", "joint_L_wrist")
            },
            {
                jointName: "RightWrist",
                controller: Controller.createInputController("Spatial", "joint_R_wrist")
            }
        ];

        hands = [
            {
                jointName: "LeftHand",
                controller: Controller.createInputController("Spatial", "joint_L_hand"),
                inactiveCount: 0
            },
            {
                jointName: "RightHand",
                controller: Controller.createInputController("Spatial", "joint_R_hand"),
                inactiveCount: 0
            }
        ];

        // The Leap controller's first joint is the hand-metacarpal joint but this joint's data is not used because it's too
        // dependent on the model skeleton exactly matching the Leap skeleton; using just the second and subsequent joints 
        // seems to work better over all.
        fingers = [{}, {}];
        fingers[0] = [
            [
                { jointName: "LeftHandThumb1", controller: Controller.createInputController("Spatial", "joint_L_thumb2") },
                { jointName: "LeftHandThumb2", controller: Controller.createInputController("Spatial", "joint_L_thumb3") },
                { jointName: "LeftHandThumb3", controller: Controller.createInputController("Spatial", "joint_L_thumb4") }
            ],
            [
                { jointName: "LeftHandIndex1", controller: Controller.createInputController("Spatial", "joint_L_index2") },
                { jointName: "LeftHandIndex2", controller: Controller.createInputController("Spatial", "joint_L_index3") },
                { jointName: "LeftHandIndex3", controller: Controller.createInputController("Spatial", "joint_L_index4") }
            ],
            [
                { jointName: "LeftHandMiddle1", controller: Controller.createInputController("Spatial", "joint_L_middle2") },
                { jointName: "LeftHandMiddle2", controller: Controller.createInputController("Spatial", "joint_L_middle3") },
                { jointName: "LeftHandMiddle3", controller: Controller.createInputController("Spatial", "joint_L_middle4") }
            ],
            [
                { jointName: "LeftHandRing1", controller: Controller.createInputController("Spatial", "joint_L_ring2") },
                { jointName: "LeftHandRing2", controller: Controller.createInputController("Spatial", "joint_L_ring3") },
                { jointName: "LeftHandRing3", controller: Controller.createInputController("Spatial", "joint_L_ring4") }
            ],
            [
                { jointName: "LeftHandPinky1", controller: Controller.createInputController("Spatial", "joint_L_pinky2") },
                { jointName: "LeftHandPinky2", controller: Controller.createInputController("Spatial", "joint_L_pinky3") },
                { jointName: "LeftHandPinky3", controller: Controller.createInputController("Spatial", "joint_L_pinky4") }
            ]
        ];
        fingers[1] = [
            [
                { jointName: "RightHandThumb1", controller: Controller.createInputController("Spatial", "joint_R_thumb2") },
                { jointName: "RightHandThumb2", controller: Controller.createInputController("Spatial", "joint_R_thumb3") },
                { jointName: "RightHandThumb3", controller: Controller.createInputController("Spatial", "joint_R_thumb4") }
            ],
            [
                { jointName: "RightHandIndex1", controller: Controller.createInputController("Spatial", "joint_R_index2") },
                { jointName: "RightHandIndex2", controller: Controller.createInputController("Spatial", "joint_R_index3") },
                { jointName: "RightHandIndex3", controller: Controller.createInputController("Spatial", "joint_R_index4") }
            ],
            [
                { jointName: "RightHandMiddle1", controller: Controller.createInputController("Spatial", "joint_R_middle2") },
                { jointName: "RightHandMiddle2", controller: Controller.createInputController("Spatial", "joint_R_middle3") },
                { jointName: "RightHandMiddle3", controller: Controller.createInputController("Spatial", "joint_R_middle4") }
            ],
            [
                { jointName: "RightHandRing1", controller: Controller.createInputController("Spatial", "joint_R_ring2") },
                { jointName: "RightHandRing2", controller: Controller.createInputController("Spatial", "joint_R_ring3") },
                { jointName: "RightHandRing3", controller: Controller.createInputController("Spatial", "joint_R_ring4") }
            ],
            [
                { jointName: "RightHandPinky1", controller: Controller.createInputController("Spatial", "joint_R_pinky2") },
                { jointName: "RightHandPinky2", controller: Controller.createInputController("Spatial", "joint_R_pinky3") },
                { jointName: "RightHandPinky3", controller: Controller.createInputController("Spatial", "joint_R_pinky4") }
            ]
        ];

        handAnimationStateHandlers = [null, null];
        handAnimationStateFunctions = [animateLeftHand, animateRightHand];
        handAnimationStateProperties = [
            ["leftHandType", "leftHandPosition", "leftHandRotation"],
            ["rightHandType", "rightHandPosition", "rightHandPosition"]
        ];

        setIsOnHMD();

        settingsTimer = Script.setInterval(checkSettings, 2000);

        calibrationStatus = UNCALIBRATED;

        {
            var mapping = Controller.newMapping("LeapmotionTrigger");
            mapping.from(getLeapMotionLeftTrigger).to(Controller.Standard.LT);
            mapping.from(getLeapMotionRightTrigger).to(Controller.Standard.RT);
            mapping.enable();
        }
    }

    function moveHands() {
        var h,
            i,
            j,
            side,
            handOffset,
            wristOffset,
            handRotation,
            locRotation,
            cameraOrientation,
            inverseAvatarOrientation;

        for (h = 0; h < NUM_HANDS; h += 1) {
            side = h === 0 ? -1.0 : 1.0;

            if (hands[h].controller.isActive()) {

                // Calibrate if necessary.
                if (!checkCalibration()) {
                    return;
                }

                // Hand animation handlers ...
                if (handAnimationStateHandlers[h] === null) {
                    handAnimationStateHandlers[h] = MyAvatar.addAnimationStateHandler(handAnimationStateFunctions[h],
                        handAnimationStateProperties[h]);
                }

                // Hand position ...
                handOffset = hands[h].controller.getAbsTranslation();
                handRotation = hands[h].controller.getAbsRotation();

                if (isOnHMD) {

                    // Adjust to control wrist position if "hand" joint is at wrist ...
                    if (!hasHandAndWristJoints) {
                        wristOffset = Vec3.multiplyQbyV(handRotation, handToWristOffset[h]);
                        handOffset = Vec3.sum(handOffset, wristOffset);
                    }

                    // Hand offset in camera coordinates ...
                    handOffset = {
                        x: -handOffset.x,
                        y: -handOffset.z,
                        z: -handOffset.y - hands[h].zeroPosition.z
                    };

                    // Hand offset in world coordinates ...
                    cameraOrientation = Camera.getOrientation();
                    handOffset = Vec3.sum(Camera.getPosition(), Vec3.multiplyQbyV(cameraOrientation, handOffset));

                    // Hand offset in avatar coordinates ...
                    inverseAvatarOrientation = Quat.inverse(MyAvatar.orientation);
                    handOffset = Vec3.subtract(handOffset, MyAvatar.position);
                    handOffset = Vec3.multiplyQbyV(inverseAvatarOrientation, handOffset);
                    handOffset.z = -handOffset.z;
                    handOffset.x = -handOffset.x;


                    // Hand rotation in camera coordinates ...
                    handRotation = {
                        x: -handRotation.y,
                        y: -handRotation.z,
                        z: -handRotation.x,
                        w: handRotation.w
                    };

                    // Hand rotation in avatar coordinates ...
                    handRotation = Quat.multiply(HMD_CAMERA_TO_AVATAR_ROTATION[h], handRotation);
                    cameraOrientation = {
                        x: cameraOrientation.z,
                        y: cameraOrientation.y,
                        z: cameraOrientation.x,
                        w: cameraOrientation.w
                    };
                    cameraOrientation = Quat.multiply(cameraOrientation, Quat.inverse(MyAvatar.orientation));
                    handRotation = Quat.multiply(handRotation, cameraOrientation);  // Works!!!

                } else {

                    // Adjust to control wrist position if "hand" joint is at wrist ...
                    if (!hasHandAndWristJoints) {
                        wristOffset = Vec3.multiplyQbyV(handRotation, handToWristOffset[h]);
                        handOffset = Vec3.sum(handOffset, wristOffset);
                    }

                    // Hand offset in camera coordinates ...
                    handOffset = {
                        x: -handOffset.x,
                        y: hands[h].zeroPosition.y + handOffset.y,
                        z: hands[h].zeroPosition.z - handOffset.z
                    };

                    // Hand rotation in camera coordinates ...
                    handRotation = {
                        x: handRotation.z,
                        y: handRotation.y,
                        z: handRotation.x,
                        w: handRotation.w
                    };

                    // Hand rotation in avatar coordinates ...
                    handRotation = Quat.multiply(DESKTOP_CAMERA_TO_AVATAR_ROTATION, handRotation);
                }

                // Set hand position and orientation for animation state handler ...
                hands[h].position = handOffset;
                hands[h].rotation = handRotation;

                // Set finger joints ...
                var summed = 0;
                var closeAngle = 0;
                for (i = 0; i < NUM_FINGERS; i += 1) {
                    for (j = 0; j < NUM_FINGER_JOINTS; j += 1) {
                        if (fingers[h][i][j].controller !== null) {
                            locRotation = fingers[h][i][j].controller.getLocRotation();
                            var eulers = Quat.safeEulerAngles(locRotation);
                            closeAngle += eulers.x;

                            summed++;

                            if (i === THUMB) {
                                locRotation = {
                                    x: side * locRotation.y,
                                    y: side * -locRotation.z,
                                    z: side * -locRotation.x,
                                    w: locRotation.w
                                };
                                if (j === 0) {
                                    // Adjust avatar thumb root joint rotation to make avatar hands look better
                                    locRotation = Quat.multiply(LEAP_THUMB_ROOT_ADJUST[h], locRotation);
                                }
                            } else {
                                locRotation = {
                                    x: -locRotation.x,
                                    y: -locRotation.z,
                                    z: -locRotation.y,
                                    w: locRotation.w
                                };
                            }
                            MyAvatar.setJointRotation(fingers[h][i][j].jointName, locRotation);
                        }
                    }
                }
 
                hands[h].inactiveCount = 0;
                if (summed > 0) {
                    closeAngle /= summed;
                }

                var triggerValue = (-closeAngle - LEAP_TRIGGER_START_ANGLE) /  (LEAP_TRIGGER_END_ANGLE - LEAP_TRIGGER_START_ANGLE);
                triggerValue = Math.max(0.0, Math.min(triggerValue, 1.0));

                if (h == 0) {
                    leftTriggerValue = triggerValue; 
                } else {
                    rightTriggerValue = triggerValue;

                }

            } else {

                if (hands[h].inactiveCount < MAX_HAND_INACTIVE_COUNT) {

                    hands[h].inactiveCount += 1;

                    if (hands[h].inactiveCount === MAX_HAND_INACTIVE_COUNT) {
                        if (handAnimationStateHandlers[h] !== null) {
                            MyAvatar.removeAnimationStateHandler(handAnimationStateHandlers[h]);
                            handAnimationStateHandlers[h] = null;
                            leftTriggerValue = 0.0;
                            rightTriggerValue = 0.0;
                        }
                    }
                }
            }
        }
    }

    function tearDown() {
        var h,
            i,
            j;

        Script.clearInterval(settingsTimer);

        for (h = 0; h < NUM_HANDS; h += 1) {
            Controller.releaseInputController(hands[h].controller);
            Controller.releaseInputController(wrists[h].controller);
            if (handAnimationStateHandlers[h] !== null) {
                MyAvatar.removeAnimationStateHandler(handAnimationStateHandlers[h]);
            }
            for (i = 0; i < NUM_FINGERS; i += 1) {
                for (j = 0; j < NUM_FINGER_JOINTS; j += 1) {
                    if (fingers[h][i][j].controller !== null) {
                        Controller.releaseInputController(fingers[h][i][j].controller);
                    }
                }
            }
        }
    }

    return {
        printSkeletonJointNames: printSkeletonJointNames,
        setUp : setUp,
        moveHands : moveHands,
        tearDown : tearDown
    };
}());


//leapHands.printSkeletonJointNames();

leapHands.setUp();
Script.update.connect(leapHands.moveHands);
Script.scriptEnding.connect(leapHands.tearDown);
