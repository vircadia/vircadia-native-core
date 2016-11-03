//
//  Cube3DOverlay.h
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Cube3DOverlay_h
#define hifi_Cube3DOverlay_h

#include "Volume3DOverlay.h"

class Cube3DOverlay : public Volume3DOverlay {
    Q_OBJECT
    
public:
    static QString const TYPE;
    virtual QString getType() const override { return TYPE; }

    Cube3DOverlay();
    Cube3DOverlay(const Cube3DOverlay* cube3DOverlay);
    ~Cube3DOverlay();

    virtual void render(RenderArgs* args) override;
    virtual const render::ShapeKey getShapeKey() override;

    virtual Cube3DOverlay* createClone() const override;

    float getBorderSize() const { return _borderSize; }

    void setBorderSize(float value) { _borderSize = value; }

    void setProperties(const QVariantMap& properties) override;
    QVariant getProperty(const QString& property) override;

private:
    float _borderSize;
    // edges on a cube
    std::array<int, 12> _geometryIds;
};

 
#endif // hifi_Cube3DOverlay_h
