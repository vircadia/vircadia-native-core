//
//  Visage.h
//  interface/src/devices
//
//  Created by Andrzej Kapolka on 2/11/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Visage_h
#define hifi_Visage_h

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
class Visage : public QObject {
    Q_OBJECT
    
public:
    
    Visage();
    virtual ~Visage();
    
    void init();
    
    bool isActive() const { return _active; }
    
    const glm::quat& getHeadRotation() const { return _headRotation; }
    const glm::vec3& getHeadTranslation() const { return _headTranslation; }
    
    float getEstimatedEyePitch() const { return _estimatedEyePitch; }
    float getEstimatedEyeYaw() const { return _estimatedEyeYaw; }
    
    const QVector<float>& getBlendshapeCoefficients() const { return _blendshapeCoefficients; }
    
    void update();
    void reset();

public slots:

    void updateEnabled();
    
private:

#ifdef HAVE_VISAGE
    VisageSDK::VisageTracker2* _tracker;
    VisageSDK::FaceData* _data;
    QMultiHash<int, QPair<int, float> > _actionUnitIndexMap; 
#endif
    
    void setEnabled(bool enabled);
    
    bool _enabled;
    bool _active;
    glm::quat _headRotation;
    glm::vec3 _headTranslation;
    
    glm::vec3 _headOrigin;
    
    float _estimatedEyePitch;
    float _estimatedEyeYaw;
    
    QVector<float> _blendshapeCoefficients;
};

#endif // hifi_Visage_h
