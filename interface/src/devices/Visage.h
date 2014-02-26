//
//  Visage.h
//  interface
//
//  Created by Andrzej Kapolka on 2/11/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Visage__
#define __interface__Visage__

#include <vector>

#include <QMultiHash>
#include <QPair>
#include <QVector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace VisageSDK {
    class VisageTracker2;
    struct FaceData;
}

/// Handles input from the Visage webcam feature tracking software.
class Visage {
public:
    
    Visage();
    ~Visage();
    
    bool isActive() const { return _active; }
    
    const glm::quat& getHeadRotation() const { return _headRotation; }
    const glm::vec3& getHeadTranslation() const { return _headTranslation; }
    
    float getEstimatedEyePitch() const { return _estimatedEyePitch; }
    float getEstimatedEyeYaw() const { return _estimatedEyeYaw; }
    
    const std::vector<float>& getBlendshapeCoefficients() const { return _blendshapeCoefficients; }
    
    void update();
    void reset();
    
private:

#ifdef HAVE_VISAGE
    VisageSDK::VisageTracker2* _tracker;
    VisageSDK::FaceData* _data;
    QMultiHash<int, QPair<int, float> > _actionUnitIndexMap; 
#endif
    
    bool _active;
    glm::quat _headRotation;
    glm::vec3 _headTranslation;
    
    glm::vec3 _headOrigin;
    
    float _estimatedEyePitch;
    float _estimatedEyeYaw;
    
    std::vector<float> _blendshapeCoefficients;
};

#endif /* defined(__interface__Visage__) */
