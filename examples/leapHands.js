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

    var leftHand,
        rightHand,
        NUM_FINGERS = 5,  // 0 = thumb; ...; 4 = pinky
        THUMB = 0,
        leftHandFingers,
        rightHandFingers,
        NUM_FINGER_JOINTS = 3,  // 0 = hand-finger joint; ...; 2 = fingertip joint
        leftHandInactiveCount,
        rightHandInactiveCount,
        MAX_HAND_INACTIVE_COUNT = 20;

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

    function setUp() {

        leftHand = Controller.createInputController("Spatial", "joint_L_hand");
        leftHandFingers = [
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
        leftHandInactiveCount = MAX_HAND_INACTIVE_COUNT;

        rightHand = Controller.createInputController("Spatial", "joint_R_hand");
        rightHandFingers = [
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
        rightHandInactiveCount = MAX_HAND_INACTIVE_COUNT;
    }

    function moveHands() {
        var i,
            j,
            locRotation;

        if (leftHand.isActive()) {
            leftHandInactiveCount = 0;

            // Fixed hand location for starters ...
            MyAvatar.setJointData("LeftArm", Quat.fromPitchYawRollDegrees(90.0, 0.0, -90.0));
            MyAvatar.setJointData("LeftForeArm", Quat.fromPitchYawRollDegrees(90.0, 0.0, 180.0));
            MyAvatar.setJointData("LeftHand", Quat.fromPitchYawRollDegrees(0.0, 0.0, 0.0));

            // Finger joints ...
            for (i = 0; i < NUM_FINGERS; i += 1) {
                for (j = 0; j < NUM_FINGER_JOINTS; j += 1) {
                    if (leftHandFingers[i][j].controller !== null) {
                        locRotation = leftHandFingers[i][j].controller.getLocRotation();
                        if (i === THUMB) {
                            MyAvatar.setJointData(leftHandFingers[i][j].jointName,
                                Quat.fromPitchYawRollRadians(-locRotation.y, -locRotation.z, locRotation.x));

                        } else {
                            MyAvatar.setJointData(leftHandFingers[i][j].jointName,
                                Quat.fromPitchYawRollRadians(-locRotation.x, 0.0, -locRotation.y));
                        }
                    }
                }
            }

            leftHandInactiveCount = 0;
        } else {
            leftHandInactiveCount += 1;
        }

        if (rightHand.isActive()) {

            // Fixed hand location for starters ...
            MyAvatar.setJointData("RightArm", Quat.fromPitchYawRollDegrees(90.0, 0.0, 90.0));
            MyAvatar.setJointData("RightForeArm", Quat.fromPitchYawRollDegrees(90.0, 0.0, 180.0));
            MyAvatar.setJointData("RightHand", Quat.fromPitchYawRollDegrees(0.0, 0.0, 0.0));

            // Finger joints ...
            for (i = 0; i < NUM_FINGERS; i += 1) {
                for (j = 0; j < NUM_FINGER_JOINTS; j += 1) {
                    if (rightHandFingers[i][j].controller !== null) {
                        locRotation = rightHandFingers[i][j].controller.getLocRotation();
                        if (i === THUMB) {
                            MyAvatar.setJointData(rightHandFingers[i][j].jointName,
                                Quat.fromPitchYawRollRadians(locRotation.y, -locRotation.z, -locRotation.x));
                        } else {
                            MyAvatar.setJointData(rightHandFingers[i][j].jointName,
                                Quat.fromPitchYawRollRadians(-locRotation.x, 0.0, -locRotation.y));
                        }
                    }
                }
            }

            rightHandInactiveCount = 0;
        } else {
            rightHandInactiveCount += 1;
        }

        if (leftHandInactiveCount >= MAX_HAND_INACTIVE_COUNT && rightHandInactiveCount >= MAX_HAND_INACTIVE_COUNT) {
            MyAvatar.clearJointsData();
        }
    }

    function tearDown() {
        var i,
            j;

        Controller.releaseInputController(leftHand);
        for (i = 0; i < NUM_FINGERS; i += 1) {
            for (j = 0; j < NUM_FINGER_JOINTS; j += 1) {
                if (leftHandFingers[i][j].controller !== null) {
                    Controller.releaseInputController(leftHandFingers[i][j].controller);
                }
            }
        }

        Controller.releaseInputController(rightHand);
        for (i = 0; i < NUM_FINGERS; i += 1) {
            for (j = 0; j < NUM_FINGER_JOINTS; j += 1) {
                if (rightHandFingers[i][j].controller !== null) {
                    Controller.releaseInputController(rightHandFingers[i][j].controller);
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
