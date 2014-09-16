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

var leapHands = (function () {

    var hands,
        wrists,
        NUM_HANDS = 2,  // 0 = left; 1 = right
        fingers,
        NUM_FINGERS = 5,  // 0 = thumb; ...; 4 = pinky
        THUMB = 0,
        NUM_FINGER_JOINTS = 3,  // 0 = metacarpal(hand)-proximal(finger) joint; ...; 2 = intermediate-distal(tip) joint
        MAX_HAND_INACTIVE_COUNT = 20,
        calibrationStatus,
        UNCALIBRATED = 0,
        CALIBRATING = 1,
        CALIBRATED = 2,
        CALIBRATION_TIME = 1000,  // milliseconds
        PI = 3.141593;

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

        /*
        http://public.highfidelity.io/models/skeletons/ron_standing.fst
        Skeleton joint names ...
        0: Hips
        1: RightUpLeg
        2: RightLeg
        3: RightFoot
        4: RightToeBase
        5: RightToe_End
        6: LeftUpLeg
        7: LeftLeg
        8: LeftFoot
        9: LeftToeBase
        10: LeftToe_End
        11: Spine
        12: Spine1
        13: Spine2
        14: RightShoulder
        15: RightArm
        16: RightForeArm
        17: RightHand
        18: RightHandPinky1
        19: RightHandPinky2
        20: RightHandPinky3
        21: RightHandPinky4
        22: RightHandRing1
        23: RightHandRing2
        24: RightHandRing3
        25: RightHandRing4
        26: RightHandMiddle1
        27: RightHandMiddle2
        28: RightHandMiddle3
        29: RightHandMiddle4
        30: RightHandIndex1
        31: RightHandIndex2
        32: RightHandIndex3
        33: RightHandIndex4
        34: RightHandThumb1
        35: RightHandThumb2
        36: RightHandThumb3
        37: RightHandThumb4
        38: LeftShoulder
        39: LeftArm
        40: LeftForeArm
        41: LeftHand
        42: LeftHandPinky1
        43: LeftHandPinky2
        44: LeftHandPinky3
        45: LeftHandPinky4
        46: LeftHandRing1
        47: LeftHandRing2
        48: LeftHandRing3
        49: LeftHandRing4
        50: LeftHandMiddle1
        51: LeftHandMiddle2
        52: LeftHandMiddle3
        53: LeftHandMiddle4
        54: LeftHandIndex1
        55: LeftHandIndex2
        56: LeftHandIndex3
        57: LeftHandIndex4
        58: LeftHandThumb1
        59: LeftHandThumb2
        60: LeftHandThumb3
        61: LeftHandThumb4
        62: Neck
        63: Head
        64: HeadTop_End
        65: body
        ... skeleton joint names
        */
    }

    function finishCalibration() {
        var avatarPosition,
            avatarOrientation,
            avatarHandPosition,
            leapHandHeight,
            h;

        if (hands[0].controller.isActive() && hands[1].controller.isActive()) {
            leapHandHeight = (hands[0].controller.getAbsTranslation().y + hands[1].controller.getAbsTranslation().y) / 2.0;
        } else {
            calibrationStatus = UNCALIBRATED;
            return;
        }

        avatarPosition = MyAvatar.position;
        avatarOrientation = MyAvatar.orientation;

        for (h = 0; h < NUM_HANDS; h += 1) {
            avatarHandPosition = MyAvatar.getJointPosition(hands[h].jointName);
            hands[h].zeroPosition = {
                x: avatarHandPosition.x - avatarPosition.x,
                y: avatarHandPosition.y - avatarPosition.y,
                z: avatarPosition.z - avatarHandPosition.z
            };
            hands[h].zeroPosition = Vec3.multiplyQbyV(MyAvatar.orientation, hands[h].zeroPosition);
            hands[h].zeroPosition.y = hands[h].zeroPosition.y - leapHandHeight;
        }

        MyAvatar.clearJointData("LeftHand");
        MyAvatar.clearJointData("LeftForeArm");
        MyAvatar.clearJointData("LeftArm");
        MyAvatar.clearJointData("RightHand");
        MyAvatar.clearJointData("RightForeArm");
        MyAvatar.clearJointData("RightArm");

        calibrationStatus = CALIBRATED;
        print("Leap Motion: Calibrated");
    }

    function calibrate() {

        calibrationStatus = CALIBRATING;

        // Set avatar arms vertical, forearms horizontal, as "zero" position for calibration
        MyAvatar.setJointData("LeftArm", Quat.fromPitchYawRollDegrees(90.0, 0.0, -90.0));
        MyAvatar.setJointData("LeftForeArm", Quat.fromPitchYawRollDegrees(90.0, 0.0, 180.0));
        MyAvatar.setJointData("LeftHand", Quat.fromPitchYawRollRadians(0.0, 0.0, 0.0));
        MyAvatar.setJointData("RightArm", Quat.fromPitchYawRollDegrees(90.0, 0.0, 90.0));
        MyAvatar.setJointData("RightForeArm", Quat.fromPitchYawRollDegrees(90.0, 0.0, 180.0));
        MyAvatar.setJointData("RightHand", Quat.fromPitchYawRollRadians(0.0, 0.0, 0.0));

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

    function setUp() {

        calibrationStatus = UNCALIBRATED;

        // TODO: Leap Motion controller joint naming doesn't match up with skeleton joint naming; numbers are out by 1.

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

        wrists = [
            { controller: Controller.createInputController("Spatial", "joint_L_wrist") },
            { controller: Controller.createInputController("Spatial", "joint_R_wrist") }
        ];

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
    }

    function moveHands() {
        var h,
            i,
            j,
            side,
            handOffset,
            handRoll,
            handPitch,
            handYaw,
            handRotation,
            wristAbsRotation,
            locRotation;

        for (h = 0; h < NUM_HANDS; h += 1) {
            side = h === 0 ? -1.0 : 1.0;

            if (hands[h].controller.isActive()) {

                // Calibrate when and if a controller is first active.
                if (!checkCalibration()) {
                    return;
                }

                // Hand position ...
                handOffset = hands[h].controller.getAbsTranslation();
                handOffset = {
                    x: -handOffset.x,
                    y: hands[h].zeroPosition.y + handOffset.y,
                    z: hands[h].zeroPosition.z - handOffset.z
                };

                // TODO: 2.0* scale factor should not be necessary; Leap Motion controller code needs investigating.
                handRoll = 2.0 * -hands[h].controller.getAbsRotation().z;
                wristAbsRotation = wrists[h].controller.getAbsRotation();
                handPitch = 2.0 * -wristAbsRotation.x;
                handYaw = 2.0 * wristAbsRotation.y;

                // TODO: Leap Motion controller's right-hand roll calculation only works if physical hand is upside down.
                // Approximate fix is to add a fudge factor.
                if (h === 1) {
                    handRoll = handRoll + 0.6 * PI;
                }

                // Hand position and orientation ...
                if (h === 0) {
                    handRotation = Quat.multiply(Quat.angleAxis(-90.0, { x: 0, y: 1, z: 0 }),
                        Quat.fromVec3Radians({ x: handRoll, y: handYaw, z: -handPitch }));


                } else {
                    handRotation = Quat.multiply(Quat.angleAxis(90.0, { x: 0, y: 1, z: 0 }),
                        Quat.fromVec3Radians({ x: -handRoll, y: handYaw, z: handPitch }));
                }
                MyAvatar.setJointModelPositionAndOrientation(hands[h].jointName, handOffset, handRotation, true);

                // Finger joints ...
                // TODO: 2.0 * scale factors should not be necessary; Leap Motion controller code needs investigating.
                for (i = 0; i < NUM_FINGERS; i += 1) {
                    for (j = 0; j < NUM_FINGER_JOINTS; j += 1) {
                        if (fingers[h][i][j].controller !== null) {
                            locRotation = fingers[h][i][j].controller.getLocRotation();
                            if (i === THUMB) {
                                MyAvatar.setJointData(fingers[h][i][j].jointName,
                                    Quat.fromPitchYawRollRadians(2.0 * side * locRotation.y, 2.0 * -locRotation.z,
                                        2.0 * side * -locRotation.x));
                            } else {
                                MyAvatar.setJointData(fingers[h][i][j].jointName,
                                    Quat.fromPitchYawRollRadians(2.0 * -locRotation.x, 0.0, 2.0 * -locRotation.y));
                            }
                        }
                    }
                }

                hands[h].inactiveCount = 0;

            } else {

                hands[h].inactiveCount += 1;

                if (hands[h].inactiveCount === MAX_HAND_INACTIVE_COUNT) {
                    if (h === 0) {
                        MyAvatar.clearJointData("LeftHand");
                        MyAvatar.clearJointData("LeftForeArm");
                        MyAvatar.clearJointData("LeftArm");
                    } else {
                        MyAvatar.clearJointData("RightHand");
                        MyAvatar.clearJointData("RightForeArm");
                        MyAvatar.clearJointData("RightArm");
                    }
                }
            }
        }
    }

    function tearDown() {
        var h,
            i,
            j;

        for (h = 0; h < NUM_HANDS; h += 1) {
            Controller.releaseInputController(hands[h].controller);
            Controller.releaseInputController(wrists[h].controller);
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
