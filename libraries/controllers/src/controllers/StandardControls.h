//
//  Created by Bradley Austin Davis 2015/10/09
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

namespace controller {

    // Needs to match order and values of SDL_GameControllerButton
    enum StandardButtonChannel {
        // Button quad
        A = 0,
        B,
        X,
        Y,

        // Center buttons
        BACK,
        GUIDE,
        START,

        // Stick press
        LS,
        RS,

        // Bumper press
        LB,
        RB,

        // DPad
        DU,
        DD,
        DL,
        DR,

        // These don't map to SDL types
        LEFT_PRIMARY_THUMB,
        LEFT_SECONDARY_THUMB,
        LEFT_PRIMARY_THUMB_TOUCH,
        LEFT_SECONDARY_THUMB_TOUCH,
        LS_TOUCH,
        LEFT_THUMB_UP,
        LS_CENTER,
        LS_X,
        LS_Y,
        LT_CLICK,

        RIGHT_PRIMARY_THUMB,
        RIGHT_SECONDARY_THUMB,
        RIGHT_PRIMARY_THUMB_TOUCH,
        RIGHT_SECONDARY_THUMB_TOUCH,
        RS_TOUCH,
        RIGHT_THUMB_UP,
        RS_CENTER,
        RS_X,
        RS_Y,
        RT_CLICK,

        LEFT_PRIMARY_INDEX,
        LEFT_SECONDARY_INDEX,
        LEFT_PRIMARY_INDEX_TOUCH,
        LEFT_SECONDARY_INDEX_TOUCH,
        LEFT_INDEX_POINT,
        RIGHT_PRIMARY_INDEX,
        RIGHT_SECONDARY_INDEX,
        RIGHT_PRIMARY_INDEX_TOUCH,
        RIGHT_SECONDARY_INDEX_TOUCH,
        RIGHT_INDEX_POINT,

        LEFT_GRIP_TOUCH,
        RIGHT_GRIP_TOUCH,

        NUM_STANDARD_BUTTONS
    };

    // Needs to match order and values of SDL_GameControllerAxis
    enum StandardAxisChannel {
        // Left Analog stick
        LX = 0,
        LY,
        // Right Analog stick
        RX,
        RY,
        // Triggers
        LT,
        RT,
        // Grips
        LEFT_GRIP,
        RIGHT_GRIP,

        // blendshapes
        EYEBLINK_L,
        EYEBLINK_R,
        EYESQUINT_L,
        EYESQUINT_R,
        EYEDOWN_L,
        EYEDOWN_R,
        EYEIN_L,
        EYEIN_R,
        EYEOPEN_L,
        EYEOPEN_R,
        EYEOUT_L,
        EYEOUT_R,
        EYEUP_L,
        EYEUP_R,
        BROWSD_L,
        BROWSD_R,
        BROWSU_C,
        BROWSU_L,
        BROWSU_R,
        JAWFWD,
        JAWLEFT,
        JAWOPEN,
        JAWRIGHT,
        MOUTHLEFT,
        MOUTHRIGHT,
        MOUTHFROWN_L,
        MOUTHFROWN_R,
        MOUTHSMILE_L,
        MOUTHSMILE_R,
        MOUTHDIMPLE_L,
        MOUTHDIMPLE_R,
        LIPSSTRETCH_L,
        LIPSSTRETCH_R,
        LIPSUPPERCLOSE,
        LIPSLOWERCLOSE,
        LIPSFUNNEL,
        LIPSPUCKER,
        PUFF,
        CHEEKSQUINT_L,
        CHEEKSQUINT_R,
        MOUTHCLOSE,
        MOUTHUPPERUP_L,
        MOUTHUPPERUP_R,
        MOUTHLOWERDOWN_L,
        MOUTHLOWERDOWN_R,
        MOUTHPRESS_L,
        MOUTHPRESS_R,
        MOUTHSHRUGLOWER,
        MOUTHSHRUGUPPER,
        NOSESNEER_L,
        NOSESNEER_R,
        TONGUEOUT,
        USERBLENDSHAPE0,
        USERBLENDSHAPE1,
        USERBLENDSHAPE2,
        USERBLENDSHAPE3,
        USERBLENDSHAPE4,
        USERBLENDSHAPE5,
        USERBLENDSHAPE6,
        USERBLENDSHAPE7,
        USERBLENDSHAPE8,
        USERBLENDSHAPE9,

        NUM_STANDARD_AXES,
        LZ = LT,
        RZ = RT
    };

    // No correlation to SDL
    enum StandardPoseChannel {
        HIPS = 0,
        RIGHT_UP_LEG,
        RIGHT_LEG,
        RIGHT_FOOT,
        LEFT_UP_LEG,
        LEFT_LEG,
        LEFT_FOOT,
        SPINE,
        SPINE1,
        SPINE2,
        SPINE3,
        NECK,
        HEAD,
        RIGHT_SHOULDER,
        RIGHT_ARM,
        RIGHT_FORE_ARM,
        RIGHT_HAND,
        RIGHT_HAND_THUMB1,
        RIGHT_HAND_THUMB2,
        RIGHT_HAND_THUMB3,
        RIGHT_HAND_THUMB4,
        RIGHT_HAND_INDEX1,
        RIGHT_HAND_INDEX2,
        RIGHT_HAND_INDEX3,
        RIGHT_HAND_INDEX4,
        RIGHT_HAND_MIDDLE1,
        RIGHT_HAND_MIDDLE2,
        RIGHT_HAND_MIDDLE3,
        RIGHT_HAND_MIDDLE4,
        RIGHT_HAND_RING1,
        RIGHT_HAND_RING2,
        RIGHT_HAND_RING3,
        RIGHT_HAND_RING4,
        RIGHT_HAND_PINKY1,
        RIGHT_HAND_PINKY2,
        RIGHT_HAND_PINKY3,
        RIGHT_HAND_PINKY4,
        LEFT_SHOULDER,
        LEFT_ARM,
        LEFT_FORE_ARM,
        LEFT_HAND,
        LEFT_HAND_THUMB1,
        LEFT_HAND_THUMB2,
        LEFT_HAND_THUMB3,
        LEFT_HAND_THUMB4,
        LEFT_HAND_INDEX1,
        LEFT_HAND_INDEX2,
        LEFT_HAND_INDEX3,
        LEFT_HAND_INDEX4,
        LEFT_HAND_MIDDLE1,
        LEFT_HAND_MIDDLE2,
        LEFT_HAND_MIDDLE3,
        LEFT_HAND_MIDDLE4,
        LEFT_HAND_RING1,
        LEFT_HAND_RING2,
        LEFT_HAND_RING3,
        LEFT_HAND_RING4,
        LEFT_HAND_PINKY1,
        LEFT_HAND_PINKY2,
        LEFT_HAND_PINKY3,
        LEFT_HAND_PINKY4,
        TRACKED_OBJECT_00,
        TRACKED_OBJECT_01,
        TRACKED_OBJECT_02,
        TRACKED_OBJECT_03,
        TRACKED_OBJECT_04,
        TRACKED_OBJECT_05,
        TRACKED_OBJECT_06,
        TRACKED_OBJECT_07,
        TRACKED_OBJECT_08,
        TRACKED_OBJECT_09,
        TRACKED_OBJECT_10,
        TRACKED_OBJECT_11,
        TRACKED_OBJECT_12,
        TRACKED_OBJECT_13,
        TRACKED_OBJECT_14,
        TRACKED_OBJECT_15,
        LEFT_EYE,
        RIGHT_EYE,
        NUM_STANDARD_POSES
    };

    enum StandardCounts {
        TRIGGERS = 2,
        ANALOG_STICKS = 2,
        POSES = NUM_STANDARD_POSES
    };
}
