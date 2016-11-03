//
//  Planar3DOverlay.h
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Planar3DOverlay_h
#define hifi_Planar3DOverlay_h

#include "Base3DOverlay.h"

class Planar3DOverlay : public Base3DOverlay {
    Q_OBJECT
    
public:
    Planar3DOverlay();
    Planar3DOverlay(const Planar3DOverlay* planar3DOverlay);
    
    virtual AABox getBounds() const override;
    
    glm::vec2 getDimensions() const { return _dimensions; }
    void setDimensions(float value) { _dimensions = glm::vec2(value); }
    void setDimensions(const glm::vec2& value) { _dimensions = value; }
    
    void setProperties(const QVariantMap& properties) override;
    QVariant getProperty(const QString& property) override;

    virtual bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance, 
                                        BoxFace& face, glm::vec3& surfaceNormal) override;
    
protected:
    glm::vec2 _dimensions;
};

 
#endif // hifi_Planar3DOverlay_h
