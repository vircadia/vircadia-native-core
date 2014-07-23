//
//  FacialAnimationData.cpp
//  interface/src/devices
//
//  Created by Ben Arnold on 7/21/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "FacialAnimationData.h"

#ifndef max
inline int max(int a, int b) { return a > b ? a : b; }
#endif

FacialAnimationData::FacialAnimationData() :_leftBlinkIndex(0), // see http://support.faceshift.com/support/articles/35129-export-of-blendshapes
_rightBlinkIndex(1),
_leftEyeOpenIndex(8),
_rightEyeOpenIndex(9),
_browDownLeftIndex(14),
_browDownRightIndex(15),
_browUpCenterIndex(16),
_browUpLeftIndex(17),
_browUpRightIndex(18),
_mouthSmileLeftIndex(28),
_mouthSmileRightIndex(29),
_jawOpenIndex(21) {
}

void FacialAnimationData::setBlendshapeCoefficient(int index, float val) {
    _blendshapeCoefficients.resize(max((int)_blendshapeCoefficients.size(), _jawOpenIndex + 1));
    if (index >= 0 && index < (int)_blendshapeCoefficients.size()) {
        _blendshapeCoefficients[index] = val;
    }
}

float FacialAnimationData::getBlendshapeCoefficient(int index) const {
    return (index >= 0 && index < (int)_blendshapeCoefficients.size()) ? _blendshapeCoefficients[index] : 0.0f;
}

void FacialAnimationData::updateFakeCoefficients(float leftBlink, float rightBlink, float browUp, float jawOpen) {
    _blendshapeCoefficients.resize(max((int)_blendshapeCoefficients.size(), _jawOpenIndex + 1));
    qFill(_blendshapeCoefficients.begin(), _blendshapeCoefficients.end(), 0.0f);
    _blendshapeCoefficients[_leftBlinkIndex] = leftBlink;
    _blendshapeCoefficients[_rightBlinkIndex] = rightBlink;
    _blendshapeCoefficients[_browUpCenterIndex] = browUp;
    _blendshapeCoefficients[_browUpLeftIndex] = browUp;
    _blendshapeCoefficients[_browUpRightIndex] = browUp;
    _blendshapeCoefficients[_jawOpenIndex] = jawOpen;
}