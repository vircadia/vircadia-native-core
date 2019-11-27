//
//  Created by Bradley Austin Davis on 2015/10/19
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_controller_Actions_h
#define hifi_controller_Actions_h

#include <QtCore/QObject>
#include <QtCore/QVector>

#include "InputDevice.h"

namespace controller {

// Actions are the output channels of the Mapper, that's what the InputChannel map to
// For now the Actions are hardcoded, this is bad, but we will fix that in the near future
enum class Action {
    TRANSLATE_X = 0,
    TRANSLATE_Y,
    TRANSLATE_Z,
    ROTATE_X, PITCH = ROTATE_X,
    ROTATE_Y, YAW = ROTATE_Y,
    ROTATE_Z, ROLL = ROTATE_Z,

    DELTA_PITCH,
    DELTA_YAW,
    DELTA_ROLL,

    STEP_YAW,
    // FIXME does this have a use case?
    STEP_PITCH,
    // FIXME does this have a use case?
    STEP_ROLL,

    STEP_TRANSLATE_X,
    STEP_TRANSLATE_Y,
    STEP_TRANSLATE_Z,

    TRANSLATE_CAMERA_Z,
    NUM_COMBINED_AXES,

    LEFT_HAND = NUM_COMBINED_AXES,
    RIGHT_HAND,
    LEFT_FOOT,
    RIGHT_FOOT,
    HIPS,
    SPINE2,
    HEAD,

    LEFT_HAND_CLICK,
    RIGHT_HAND_CLICK,

    ACTION1,
    ACTION2,

    CONTEXT_MENU,
    TOGGLE_MUTE,
    TOGGLE_PUSHTOTALK,
    CYCLE_CAMERA,
    TOGGLE_OVERLAY,

    SHIFT,

    UI_NAV_LATERAL,
    UI_NAV_VERTICAL,
    UI_NAV_GROUP,
    UI_NAV_SELECT,
    UI_NAV_BACK,

    // Pointer/Reticle control
    RETICLE_CLICK,
    RETICLE_X,
    RETICLE_Y,

    // Bisected aliases for RETICLE_X/RETICLE_Y
    RETICLE_LEFT,
    RETICLE_RIGHT,
    RETICLE_UP,
    RETICLE_DOWN,

    // Bisected aliases for TRANSLATE_Z
    LONGITUDINAL_BACKWARD,
    LONGITUDINAL_FORWARD,

    // Bisected aliases for TRANSLATE_X
    LATERAL_LEFT,
    LATERAL_RIGHT,

    // Bisected aliases for TRANSLATE_Y
    VERTICAL_DOWN,
    VERTICAL_UP,

    // Bisected aliases for ROTATE_Y
    YAW_LEFT,
    YAW_RIGHT,

    // Bisected aliases for ROTATE_X
    PITCH_DOWN,
    PITCH_UP,

    // Bisected aliases for TRANSLATE_CAMERA_Z
    BOOM_IN,
    BOOM_OUT,

    LEFT_ARM,
    RIGHT_ARM,

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
    RIGHT_SHOULDER,
    LEFT_FORE_ARM,
    RIGHT_FORE_ARM,
    LEFT_LEG,
    RIGHT_LEG,
    LEFT_UP_LEG,
    RIGHT_UP_LEG,
    LEFT_TOE_BASE,
    RIGHT_TOE_BASE,

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
    SPRINT,

    LEFT_EYE,
    RIGHT_EYE,

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

    NUM_ACTIONS
};

template <typename T>
int toInt(T enumValue) { return static_cast<int>(enumValue); }

class ActionsDevice : public QObject, public InputDevice {
    Q_OBJECT
    Q_PROPERTY(QString name READ getName)

public:
    ActionsDevice();

    EndpointPointer createEndpoint(const Input& input) const override;
    Input::NamedVector getAvailableInputs() const override;

};

}

#endif // hifi_StandardController_h
