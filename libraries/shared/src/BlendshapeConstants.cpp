//
//  BlendshapeConstants.cpp
//
//
//  Created by Clement on 1/23/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "BlendshapeConstants.h"

const char* BLENDSHAPE_NAMES[] = {
    "EyeBlink_L",
    "EyeBlink_R",
    "EyeSquint_L",
    "EyeSquint_R",
    "EyeDown_L",
    "EyeDown_R",
    "EyeIn_L",
    "EyeIn_R",
    "EyeOpen_L",
    "EyeOpen_R",
    "EyeOut_L",
    "EyeOut_R",
    "EyeUp_L",
    "EyeUp_R",
    "BrowsD_L",
    "BrowsD_R",
    "BrowsU_C",
    "BrowsU_L",
    "BrowsU_R",
    "JawFwd",
    "JawLeft",
    "JawOpen",
    "JawRight",
    "MouthLeft",
    "MouthRight",
    "MouthFrown_L",
    "MouthFrown_R",
    "MouthSmile_L",
    "MouthSmile_R",
    "MouthDimple_L",
    "MouthDimple_R",
    "LipsStretch_L",
    "LipsStretch_R",
    "LipsUpperClose",
    "LipsLowerClose",
    "LipsFunnel",
    "LipsPucker",
    "Puff",
    "CheekSquint_L",
    "CheekSquint_R",
    "MouthClose",
    "MouthUpperUp_L",
    "MouthUpperUp_R",
    "MouthLowerDown_L",
    "MouthLowerDown_R",
    "MouthPress_L",
    "MouthPress_R",
    "MouthShrugLower",
    "MouthShrugUpper",
    "NoseSneer_L",
    "NoseSneer_R",
    "TongueOut",
    "UserBlendshape0",
    "UserBlendshape1",
    "UserBlendshape2",
    "UserBlendshape3",
    "UserBlendshape4",
    "UserBlendshape5",
    "UserBlendshape6",
    "UserBlendshape7",
    "UserBlendshape8",
    "UserBlendshape9",
    ""
};

const QHash<QString, int> BLENDSHAPE_LOOKUP_MAP = [] {
    QHash<QString, int> toReturn;
    for (int i = 0; i < (int)Blendshapes::BlendshapeCount; i++) {
        toReturn[BLENDSHAPE_NAMES[i]] = i;
    }
    return toReturn;
}();

const QHash<QString, QPair<QString, float>> READYPLAYERME_BLENDSHAPES_MAP = {
    // ReadyPlayerMe blendshape default mapping.
    { "mouthOpen", { "JawOpen", 1.0f } },
    { "eyeBlinkLeft", { "EyeBlink_L", 1.0f } },
    { "eyeBlinkRight", { "EyeBlink_R", 1.0f } },
    { "eyeSquintLeft", { "EyeSquint_L", 1.0f } },
    { "eyeSquintRight", { "EyeSquint_R", 1.0f } },
    { "eyeWideLeft", { "EyeOpen_L", 1.0f } },
    { "eyeWideRight", { "EyeOpen_R", 1.0f } },
    { "browDownLeft", { "BrowsD_L", 1.0f } },
    { "browDownRight", { "BrowsD_R", 1.0f } },
    { "browInnerUp", { "BrowsU_C", 1.0f } },
    { "browOuterUpLeft", { "BrowsU_L", 1.0f } },
    { "browOuterUpRight", { "BrowsU_R", 1.0f } },
    { "mouthFrownLeft", { "MouthFrown_L", 1.0f } },
    { "mouthFrownRight", { "MouthFrown_R", 1.0f } },
    { "mouthPucker", { "LipsPucker", 1.0f } },
    { "jawForward", { "JawFwd", 1.0f } },
    { "jawLeft", { "JawLeft", 1.0f } },
    { "jawRight", { "JawRight", 1.0f } },
    { "mouthLeft", { "MouthLeft", 1.0f } },
    { "mouthRight", { "MouthRight", 1.0f } },
    { "noseSneerLeft", { "NoseSneer_L", 1.0f } },
    { "noseSneerRight", { "NoseSneer_R", 1.0f } },
    { "mouthLowerDownLeft", { "MouthLowerDown_L", 1.0f } },
    { "mouthLowerDownRight", { "MouthLowerDown_R", 1.0f } },
    { "mouthShrugLower", { "MouthShrugLower", 1.0f } },
    { "mouthShrugUpper", { "MouthShrugUpper", 1.0f } },
    { "viseme_sil", { "MouthClose", 1.0f } },
    { "mouthSmile", { "MouthSmile_L", 1.0f } },
    { "mouthSmile", { "MouthSmile_R", 1.0f } },
    { "viseme_CH", { "LipsFunnel", 1.0f } },
    { "viseme_PP", { "LipsUpperClose", 1.0f } },
    { "mouthShrugLower", { "LipsLowerClose", 1.0f } },
    { "viseme_FF", { "Puff", 1.0f } }
};
