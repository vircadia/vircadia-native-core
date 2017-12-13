//
//  Circle3DOverlay.h
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Circle3DOverlay_h
#define hifi_Circle3DOverlay_h

// include this before QGLWidget, which includes an earlier version of OpenGL
#include "Planar3DOverlay.h"

class Circle3DOverlay : public Planar3DOverlay {
    Q_OBJECT
    
public:
    static QString const TYPE;
    virtual QString getType() const override { return TYPE; }

    Circle3DOverlay();
    Circle3DOverlay(const Circle3DOverlay* circle3DOverlay);
    ~Circle3DOverlay();
    
    virtual void render(RenderArgs* args) override;
    virtual const render::ShapeKey getShapeKey() override;
    void setProperties(const QVariantMap& properties) override;
    QVariant getProperty(const QString& property) override;

    float getStartAt() const { return _startAt; }
    float getEndAt() const { return _endAt; }
    float getOuterRadius() const { return _outerRadius; }
    float getInnerRadius() const { return _innerRadius; }
    bool getHasTickMarks() const { return _hasTickMarks; }
    float getMajorTickMarksAngle() const { return _majorTickMarksAngle; }
    float getMinorTickMarksAngle() const { return _minorTickMarksAngle; }
    float getMajorTickMarksLength() const { return _majorTickMarksLength; }
    float getMinorTickMarksLength() const { return _minorTickMarksLength; }
    xColor getMajorTickMarksColor() const { return _majorTickMarksColor; }
    xColor getMinorTickMarksColor() const { return _minorTickMarksColor; }
    
    void setStartAt(float value) { _startAt = value; }
    void setEndAt(float value) { _endAt = value; }
    void setOuterRadius(float value) { _outerRadius = value; }
    void setInnerRadius(float value) { _innerRadius = value; }
    void setHasTickMarks(bool value) { _hasTickMarks = value; }
    void setMajorTickMarksAngle(float value) { _majorTickMarksAngle = value; }
    void setMinorTickMarksAngle(float value) { _minorTickMarksAngle = value; }
    void setMajorTickMarksLength(float value) { _majorTickMarksLength = value; }
    void setMinorTickMarksLength(float value) { _minorTickMarksLength = value; }
    void setMajorTickMarksColor(const xColor& value) { _majorTickMarksColor = value; }
    void setMinorTickMarksColor(const xColor& value) { _minorTickMarksColor = value; }

    virtual bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance, 
                                        BoxFace& face, glm::vec3& surfaceNormal) override;

    virtual Circle3DOverlay* createClone() const override;
    
protected:
    float _startAt { 0 };
    float _endAt { 360 };
    float _outerRadius { 1 };
    float _innerRadius { 0 };

    xColor _innerStartColor { DEFAULT_OVERLAY_COLOR };
    xColor _innerEndColor { DEFAULT_OVERLAY_COLOR };
    xColor _outerStartColor { DEFAULT_OVERLAY_COLOR };
    xColor _outerEndColor { DEFAULT_OVERLAY_COLOR };
    float _innerStartAlpha { DEFAULT_ALPHA };
    float _innerEndAlpha { DEFAULT_ALPHA };
    float _outerStartAlpha { DEFAULT_ALPHA };
    float _outerEndAlpha { DEFAULT_ALPHA };

    bool _hasTickMarks { false };
    float _majorTickMarksAngle { 0 };
    float _minorTickMarksAngle { 0 };
    float _majorTickMarksLength { 0 };
    float _minorTickMarksLength { 0 };
    xColor _majorTickMarksColor { DEFAULT_OVERLAY_COLOR };
    xColor _minorTickMarksColor { DEFAULT_OVERLAY_COLOR };
    gpu::Primitive _solidPrimitive { gpu::TRIANGLE_FAN };
    int _quadVerticesID { 0 };
    int _lineVerticesID { 0 };
    int _majorTicksVerticesID { 0 };
    int _minorTicksVerticesID { 0 };

    bool _dirty { true };
};

 
#endif // hifi_Circle3DOverlay_h
