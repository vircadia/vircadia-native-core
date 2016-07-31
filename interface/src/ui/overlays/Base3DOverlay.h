//
//  Base3DOverlay.h
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Base3DOverlay_h
#define hifi_Base3DOverlay_h

#include <Transform.h>
#include <SpatiallyNestable.h>

#include "Overlay.h"

class Base3DOverlay : public Overlay, public SpatiallyNestable {
    Q_OBJECT

public:
    Base3DOverlay();
    Base3DOverlay(const Base3DOverlay* base3DOverlay);

    // getters
    virtual bool is3D() const override { return true; }

    // TODO: consider implementing registration points in this class
    glm::vec3 getCenter() const { return getPosition(); }

    float getLineWidth() const { return _lineWidth; }
    bool getIsSolid() const { return _isSolid; }
    bool getIsDashedLine() const { return _isDashedLine; }
    bool getIsSolidLine() const { return !_isDashedLine; }
    bool getIgnoreRayIntersection() const { return _ignoreRayIntersection; }
    bool getDrawInFront() const { return _drawInFront; }

    void setLineWidth(float lineWidth) { _lineWidth = lineWidth; }
    void setIsSolid(bool isSolid) { _isSolid = isSolid; }
    void setIsDashedLine(bool isDashedLine) { _isDashedLine = isDashedLine; }
    void setIgnoreRayIntersection(bool value) { _ignoreRayIntersection = value; }
    void setDrawInFront(bool value) { _drawInFront = value; }

    virtual AABox getBounds() const override = 0;

    void setProperties(const QVariantMap& properties) override;
    QVariant getProperty(const QString& property) override;

    virtual bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance,
                                        BoxFace& face, glm::vec3& surfaceNormal);

    virtual bool findRayIntersectionExtraInfo(const glm::vec3& origin, const glm::vec3& direction,
                                        float& distance, BoxFace& face, glm::vec3& surfaceNormal, QString& extraInfo) {
        return findRayIntersection(origin, direction, distance, face, surfaceNormal);
    }

protected:
    virtual void locationChanged(bool tellPhysics = true) override;

    float _lineWidth;
    bool _isSolid;
    bool _isDashedLine;
    bool _ignoreRayIntersection;
    bool _drawInFront;
};

#endif // hifi_Base3DOverlay_h
