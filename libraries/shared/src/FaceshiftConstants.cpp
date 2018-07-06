//
//  FaceshiftConstants.cpp
//
//
//  Created by Clement on 1/23/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "FaceshiftConstants.h"

const char* FACESHIFT_BLENDSHAPES[] = {
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
    "JawChew",
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
    "LipsUpperUp",
    "LipsLowerDown",
    "LipsUpperOpen",
    "LipsLowerOpen",
    "LipsFunnel",
    "LipsPucker",
    "ChinLowerRaise",
    "ChinUpperRaise",
    "Sneer",
    "Puff",
    "CheekSquint_L",
    "CheekSquint_R",
    ""
};

const int NUM_FACESHIFT_BLENDSHAPES = sizeof(FACESHIFT_BLENDSHAPES) / sizeof(char*);

const int EYE_BLINK_L_INDEX = 0;
const int EYE_BLINK_R_INDEX = 1;
const int EYE_SQUINT_L_INDEX = 2;
const int EYE_SQUINT_R_INDEX = 3;
const int EYE_OPEN_L_INDEX = 8;
const int EYE_OPEN_R_INDEX = 9;
const int BROWS_U_L_INDEX = 17;
const int BROWS_U_R_INDEX = 18;


const int EYE_BLINK_INDICES[] = { EYE_BLINK_L_INDEX, EYE_BLINK_R_INDEX };
const int EYE_SQUINT_INDICES[] = { EYE_SQUINT_L_INDEX, EYE_SQUINT_R_INDEX };
const int EYE_OPEN_INDICES[] = { EYE_OPEN_L_INDEX, EYE_OPEN_R_INDEX };
const int BROWS_U_INDICES[] = { BROWS_U_L_INDEX, BROWS_U_R_INDEX };
