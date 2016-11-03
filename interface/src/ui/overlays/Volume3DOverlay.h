//
//  Volume3DOverlay.h
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Volume3DOverlay_h
#define hifi_Volume3DOverlay_h

#include "Base3DOverlay.h"

class Volume3DOverlay : public Base3DOverlay {
    Q_OBJECT

public:
    Volume3DOverlay() {}
    Volume3DOverlay(const Volume3DOverlay* volume3DOverlay);

    virtual AABox getBounds() const override;

    const glm::vec3& getDimensions() const { return _localBoundingBox.getDimensions(); }
    void setDimensions(float value) { _localBoundingBox.setBox(glm::vec3(-value / 2.0f), value); }
    void setDimensions(const glm::vec3& value) { _localBoundingBox.setBox(-value / 2.0f, value); }

    void setProperties(const QVariantMap& properties) override;
    QVariant getProperty(const QString& property) override;

    virtual bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance,
                                     BoxFace& face, glm::vec3& surfaceNormal) override;

protected:
    // Centered local bounding box
    AABox _localBoundingBox{ vec3(0.0f), 1.0f };
};


#endif // hifi_Volume3DOverlay_h
