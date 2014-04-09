//
//  FaceTracker.h
//  interface
//
//  Created by Andrzej Kapolka on 4/8/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__FaceTracker__
#define __interface__FaceTracker__

#include <QObject>
#include <QVector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

/// Base class for face trackers (Faceshift, Visage, Faceplus).
class FaceTracker : public QObject {
    Q_OBJECT
    
public:
    
    FaceTracker();
    
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

#endif /* defined(__interface__FaceTracker__) */
