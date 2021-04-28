//
//  BlendshapeConstants.h
//
//
//  Created by Clement on 1/23/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_BlendshapeConstants_h
#define hifi_BlendshapeConstants_h

#include <QHash>
#include <QString>

#include <glm/glm.hpp>

/// The names of the supported blendshapes, terminated with an empty string.
extern const char* BLENDSHAPE_NAMES[];
extern const QHash<QString, int> BLENDSHAPE_LOOKUP_MAP;
extern const QHash<QString, QPair<QString, float>> READYPLAYERME_BLENDSHAPES_MAP;

enum class Blendshapes : int {
    EyeBlink_L = 0,
    EyeBlink_R,
    EyeSquint_L,
    EyeSquint_R,
    EyeDown_L,
    EyeDown_R,
    EyeIn_L,
    EyeIn_R,
    EyeOpen_L,
    EyeOpen_R,
    EyeOut_L,
    EyeOut_R,
    EyeUp_L,
    EyeUp_R,
    BrowsD_L,
    BrowsD_R,
    BrowsU_C,
    BrowsU_L,
    BrowsU_R,
    JawFwd,
    JawLeft,
    JawOpen,
    JawRight,
    MouthLeft,
    MouthRight,
    MouthFrown_L,
    MouthFrown_R,
    MouthSmile_L,
    MouthSmile_R,
    MouthDimple_L,
    MouthDimple_R,
    LipsStretch_L,
    LipsStretch_R,
    LipsUpperClose,
    LipsLowerClose,
    LipsFunnel,
    LipsPucker,
    Puff,
    CheekSquint_L,
    CheekSquint_R,
    MouthClose,
    MouthUpperUp_L,
    MouthUpperUp_R,
    MouthLowerDown_L,
    MouthLowerDown_R,
    MouthPress_L,
    MouthPress_R,
    MouthShrugLower,
    MouthShrugUpper,
    NoseSneer_L,
    NoseSneer_R,
    TongueOut,
    UserBlendshape0,
    UserBlendshape1,
    UserBlendshape2,
    UserBlendshape3,
    UserBlendshape4,
    UserBlendshape5,
    UserBlendshape6,
    UserBlendshape7,
    UserBlendshape8,
    UserBlendshape9,
    BlendshapeCount
};

// Original blendshapes were per Faceshift.

// NEW in ARKit
// * MouthClose
// * MouthUpperUp_L
// * MouthUpperUp_R
// * MouthLowerDown_L
// * MouthLowerDown_R
// * MouthPress_L
// * MouthPress_R
// * MouthShrugLower
// * MouthShrugUpper
// * NoseSneer_L
// * NoseSneer_R
// * TongueOut

// Legacy shapes
// * JawChew (not in ARKit)
// * LipsUpperUp (split in ARKit)
// * LipsLowerDown (split in ARKit)
// * Sneer (split in ARKit)
// * ChinLowerRaise (not in ARKit)
// * ChinUpperRaise (not in ARKit)
// * LipsUpperOpen (not in ARKit)
// * LipsLowerOpen (not in ARKit)

struct BlendshapeOffsetPacked {
    glm::uvec4 packedPosNorTan;
};

struct BlendshapeOffsetUnpacked {
    glm::vec3 positionOffset;
    glm::vec3 normalOffset;
    glm::vec3 tangentOffset;
};

using BlendshapeOffset = BlendshapeOffsetPacked;

#endif // hifi_BlendshapeConstants_h
