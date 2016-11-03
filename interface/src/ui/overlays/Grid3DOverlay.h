//
//  Grid3DOverlay.h
//  interface/src/ui/overlays
//
//  Created by Ryan Huffman on 11/06/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Grid3DOverlay_h
#define hifi_Grid3DOverlay_h

#include "Planar3DOverlay.h"

class Grid3DOverlay : public Planar3DOverlay {
    Q_OBJECT

public:
    static QString const TYPE;
    virtual QString getType() const override { return TYPE; }

    Grid3DOverlay();
    Grid3DOverlay(const Grid3DOverlay* grid3DOverlay);
    ~Grid3DOverlay();

    virtual AABox getBounds() const override;

    virtual void render(RenderArgs* args) override;
    virtual const render::ShapeKey getShapeKey() override;
    void setProperties(const QVariantMap& properties) override;
    QVariant getProperty(const QString& property) override;

    virtual Grid3DOverlay* createClone() const override;

    // Grids are UI tools, and may not be intersected (pickable)
    virtual bool findRayIntersection(const glm::vec3&, const glm::vec3&, float&, BoxFace&, glm::vec3&) override { return false; }

private:
    void updateGrid();

    bool _followCamera { true };

    int _majorGridEvery { 5 };
    float _majorGridRowDivisions;
    float _majorGridColDivisions;

    float _minorGridEvery { 1.0f };
    float _minorGridRowDivisions;
    float _minorGridColDivisions;
    int _geometryId { 0 };
};

#endif // hifi_Grid3DOverlay_h
