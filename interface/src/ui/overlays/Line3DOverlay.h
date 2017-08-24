//
//  Line3DOverlay.h
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Line3DOverlay_h
#define hifi_Line3DOverlay_h

#include "Base3DOverlay.h"

class Line3DOverlay : public Base3DOverlay {
    Q_OBJECT

public:
    static QString const TYPE;
    virtual QString getType() const override { return TYPE; }

    Line3DOverlay();
    Line3DOverlay(const Line3DOverlay* line3DOverlay);
    ~Line3DOverlay();
    virtual void render(RenderArgs* args) override;
    virtual const render::ShapeKey getShapeKey() override;
    virtual AABox getBounds() const override;

    // getters
    glm::vec3 getStart() const;
    glm::vec3 getEnd() const;
    const float& getGlow() const { return _glow; }
    const float& getGlowWidth() const { return _glowWidth; }

    // setters
    void setStart(const glm::vec3& start);
    void setEnd(const glm::vec3& end);

    void setLocalStart(const glm::vec3& localStart) { setLocalPosition(localStart); }
    void setLocalEnd(const glm::vec3& localEnd);

    void setGlow(const float& glow) { _glow = glow; }
    void setGlowWidth(const float& glowWidth) { _glowWidth = glowWidth; }

    void setProperties(const QVariantMap& properties) override;
    QVariant getProperty(const QString& property) override;
    bool isTransparent() override { return Base3DOverlay::isTransparent() || _glow > 0.0f; }

    virtual Line3DOverlay* createClone() const override;

    glm::vec3 getDirection() const { return _direction; }
    float getLength() const { return _length; }
    glm::vec3 getLocalStart() const { return getLocalPosition(); }
    glm::vec3 getLocalEnd() const { return getLocalStart() + _direction * _length; }
    QUuid getEndParentID() const { return _endParentID; }
    quint16 getEndJointIndex() const { return _endParentJointIndex; }

private:
    QUuid _endParentID;
    quint16 _endParentJointIndex { INVALID_JOINT_INDEX };

    // _direction and _length are in the parent's frame.  If _endParentID is set, they are
    // relative to that.  Otherwise, they are relative to the local-start-position (which is the
    // same as localPosition)
    glm::vec3 _direction; // in parent frame
    float _length { 1.0 }; // in parent frame

    float _glow { 0.0 };
    float _glowWidth { 0.0 };
    int _geometryCacheID;
};

#endif // hifi_Line3DOverlay_h
