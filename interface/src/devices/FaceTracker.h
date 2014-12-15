//
//  FaceTracker.h
//  interface/src/devices
//
//  Created by Andrzej Kapolka on 4/9/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_FaceTracker_h
#define hifi_FaceTracker_h

#include <QObject>
#include <QVector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

/// Base class for face trackers (Faceshift, Visage, Faceplus).
class FaceTracker : public QObject {
    Q_OBJECT
    
public:
    FaceTracker();
    virtual ~FaceTracker() {}
    
    const glm::vec3& getHeadTranslation() const { return _headTranslation; }
    const glm::quat& getHeadRotation() const { return _headRotation; }
    
    float getEstimatedEyePitch() const { return _estimatedEyePitch; }
    float getEstimatedEyeYaw() const { return _estimatedEyeYaw; } 
    
    const QVector<float>& getBlendshapeCoefficients() const { return _blendshapeCoefficients; }
    
protected:
    
    glm::vec3 _headTranslation;
    glm::quat _headRotation;
    float _estimatedEyePitch;
    float _estimatedEyeYaw;
    QVector<float> _blendshapeCoefficients;
};

#endif // hifi_FaceTracker_h
