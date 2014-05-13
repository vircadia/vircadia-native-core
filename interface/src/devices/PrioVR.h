//
//  PrioVR.h
//  interface/src/devices
//
//  Created by Andrzej Kapolka on 5/12/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PrioVR_h
#define hifi_PrioVR_h

#include <QDateTime>
#include <QObject>
#include <QVector>

#include <glm/gtc/quaternion.hpp>

#ifdef HAVE_PRIOVR
extern "C" {
#include <yei_skeletal_api.h>
}
#endif

/// Handles interaction with the PrioVR skeleton tracking suit.
class PrioVR : public QObject {
    Q_OBJECT

public:
    
    PrioVR();
    virtual ~PrioVR();

    bool isActive() const { return !_jointRotations.isEmpty(); }

    glm::quat getHeadRotation() const;
    glm::quat getTorsoRotation() const;

    const QVector<int>& getHumanIKJointIndices() const { return _humanIKJointIndices; }
    const QVector<glm::quat>& getJointRotations() const { return _jointRotations; }
    
    void update();
    void reset();

private slots:

    void renderCalibrationCountdown();

private:
#ifdef HAVE_PRIOVR
    YEI_Device_Id _skeletalDevice;
#endif

    QVector<int> _humanIKJointIndices;
    QVector<glm::quat> _jointRotations;
    
    QDateTime _calibrationCountdownStarted;
};

#endif // hifi_PrioVR_h
