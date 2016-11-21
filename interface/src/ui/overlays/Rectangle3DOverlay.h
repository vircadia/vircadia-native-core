//
//  Rectangle3DOverlay.h
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Rectangle3DOverlay_h
#define hifi_Rectangle3DOverlay_h

#include "Planar3DOverlay.h"

class Rectangle3DOverlay : public Planar3DOverlay {
    Q_OBJECT
    
public:
    static QString const TYPE;
    virtual QString getType() const override { return TYPE; }

    Rectangle3DOverlay();
    Rectangle3DOverlay(const Rectangle3DOverlay* rectangle3DOverlay);
    ~Rectangle3DOverlay();
    virtual void render(RenderArgs* args) override;
    virtual const render::ShapeKey getShapeKey() override;
    void setProperties(const QVariantMap& properties) override;

    virtual Rectangle3DOverlay* createClone() const override;
private:
    int _geometryCacheID;
    std::array<int, 4> _rectGeometryIds;
    glm::vec2 _previousHalfDimensions;
};

 
#endif // hifi_Rectangle3DOverlay_h
