//
//  FacialAnimationData.h
//  interface/src/devices
//
//  Created by Ben Arnold on 7/21/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_FacialAnimationData_h
#define hifi_FacialAnimationData_h

#include <QObject>
#include <QVector>

/// Stores facial animation data for use by faceshift or scripts
class FacialAnimationData : public QObject {
    Q_OBJECT

public:
    friend class Faceshift;

    FacialAnimationData();

    float getLeftBlink() const { return getBlendshapeCoefficient(_leftBlinkIndex); }
    float getRightBlink() const { return getBlendshapeCoefficient(_rightBlinkIndex); }
    float getLeftEyeOpen() const { return getBlendshapeCoefficient(_leftEyeOpenIndex); }
    float getRightEyeOpen() const { return getBlendshapeCoefficient(_rightEyeOpenIndex); }

    float getBrowDownLeft() const { return getBlendshapeCoefficient(_browDownLeftIndex); }
    float getBrowDownRight() const { return getBlendshapeCoefficient(_browDownRightIndex); }
    float getBrowUpCenter() const { return getBlendshapeCoefficient(_browUpCenterIndex); }
    float getBrowUpLeft() const { return getBlendshapeCoefficient(_browUpLeftIndex); }
    float getBrowUpRight() const { return getBlendshapeCoefficient(_browUpRightIndex); }

    float getMouthSize() const { return getBlendshapeCoefficient(_jawOpenIndex); }
    float getMouthSmileLeft() const { return getBlendshapeCoefficient(_mouthSmileLeftIndex); }
    float getMouthSmileRight() const { return getBlendshapeCoefficient(_mouthSmileRightIndex); }

private:
    float getBlendshapeCoefficient(int index) const;

    int _leftBlinkIndex;
    int _rightBlinkIndex;
    int _leftEyeOpenIndex;
    int _rightEyeOpenIndex;

    int _browDownLeftIndex;
    int _browDownRightIndex;
    int _browUpCenterIndex;
    int _browUpLeftIndex;
    int _browUpRightIndex;

    int _mouthSmileLeftIndex;
    int _mouthSmileRightIndex;

    int _jawOpenIndex;

    //Only used by agents, since FaceTracker has its own _blendshapeCoefficients;
    QVector<float> _blendshapeCoefficients;
};

#endif // hifi_FacialAnimationData_h