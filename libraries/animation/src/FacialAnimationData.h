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
    friend class HeadData;
    friend class AvatarData;

    FacialAnimationData();

    // Setters
    void setLeftBlink(float val) { setBlendshapeCoefficient(_leftBlinkIndex, val); }
    void setRightBlink(float val) { setBlendshapeCoefficient(_rightBlinkIndex, val); }
    void setLeftEyeOpen(float val) { setBlendshapeCoefficient(_leftBlinkIndex, val); }
    void setRightEyeOpen(float val) { setBlendshapeCoefficient(_rightBlinkIndex, val); }

    void setBrowDownLeft(float val) { setBlendshapeCoefficient(_browDownLeftIndex, val); }
    void setBrowDownRight(float val) { setBlendshapeCoefficient(_browDownRightIndex, val); }
    void setBrowUpCenter(float val) { setBlendshapeCoefficient(_browUpCenterIndex, val); }
    void setBrowUpLeft(float val) { setBlendshapeCoefficient(_browUpLeftIndex, val); }
    void setBrowUpRight(float val) { setBlendshapeCoefficient(_browUpRightIndex, val); }

    void setMouthSize(float val) { setBlendshapeCoefficient(_jawOpenIndex, val); }
    void setMouthSmileLeft(float val) { setBlendshapeCoefficient(_mouthSmileLeftIndex, val); }
    void setMouthSmileRight(float val) { setBlendshapeCoefficient(_mouthSmileRightIndex, val); }

    // Getters
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

    void updateFakeCoefficients(float leftBlink, float rightBlink, float browUp, float jawOpen);

    void setBlendshapeCoefficients(const QVector<float>& coefficients) { _blendshapeCoefficients = coefficients; }
    const QVector<float>& getBlendshapeCoefficients() const { return _blendshapeCoefficients; }

private:
    void setBlendshapeCoefficient(int index, float val);
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

    QVector<float> _blendshapeCoefficients;
};

#endif // hifi_FacialAnimationData_h