//
//  FaceshiftScriptingInterface.h
//  interface/src/scripting
//
//  Created by Ben Arnold on 7/38/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_FaceshiftScriptingInterface_h
#define hifi_FaceshiftScriptingInterface_h

#include <QDebug>
#include <QObject>
#include <QString>

#include "Application.h"

class FaceshiftScriptingInterface : public QObject {
    Q_OBJECT
    FaceshiftScriptingInterface() { };
public:
    static FaceshiftScriptingInterface* getInstance();

public slots:
    bool isConnectedOrConnecting() const;

    bool isActive() const;

    const glm::vec3& getHeadAngularVelocity() const;

    // these pitch/yaw angles are in degrees
    float getEyeGazeLeftPitch() const;
    float getEyeGazeLeftYaw() const;

    float getEyeGazeRightPitch() const;
    float getEyeGazeRightYaw() const;

    float getLeftBlink() const;
    float getRightBlink() const;
    float getLeftEyeOpen() const;
    float getRightEyeOpen() const;

    float getBrowDownLeft() const;
    float getBrowDownRight() const;
    float getBrowUpCenter() const;
    float getBrowUpLeft() const;
    float getBrowUpRight() const;

    float getMouthSize() const;
    float getMouthSmileLeft() const;
    float getMouthSmileRight() const;

    void update();
    void reset();

    void updateFakeCoefficients(float leftBlink, float rightBlink, float browUp,
                                float jawOpen, QVector<float>& coefficients) const;
};

#endif // hifi_FaceshiftScriptingInterface_h
